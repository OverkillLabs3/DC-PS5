#include <algorithm>
#include "prx/libc/include/VerboseLog.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <prx/libc/include/General.hpp>

#include "SDL.h"
#include "SceTypes.hpp"
#include "prx/libkernel/Time/include/Time.hpp"

static constexpr int PORT_TYPE_MAIN = 0;
static constexpr int PORT_TYPE_BGM = 1;
static constexpr int PORT_TYPE_VOICE = 2;
static constexpr int PORT_TYPE_PERSONAL = 3;
static constexpr int PORT_TYPE_PADSPK = 4;
static constexpr int PORT_TYPE_VIBRATION = 10;
static constexpr int PORT_TYPE_AUDIO3D = 126;
static constexpr int PORT_TYPE_AUX = 127;

static constexpr int PORTS_MAX = 32;
static constexpr int DEFAULT_VOLUME = 32768;
static constexpr std::uint32_t FORMAT_MASK = 0xFFu;
static constexpr std::uint64_t TARGET_LATENCY_US = 120000;   // frame-time hitches of the video path must not starve the device
static constexpr std::uint64_t DRAIN_TIMEOUT_US = 600000;
static constexpr std::uint64_t PRIME_US = 60000;   // silence queued after an underrun so the next hitch is absorbed
static constexpr std::uint64_t DRAIN_SLEEP_US = 1000;

enum class Format {
    Unknown,
    S16Mono,
    S16Stereo,
    S16_8Ch,
    F32Mono,
    F32Stereo,
    F32_8Ch,
    S16_8ChStd,
    F32_8ChStd,
};

static bool formatIsFloat(Format f) {
    return f == Format::F32Mono || f == Format::F32Stereo ||
           f == Format::F32_8Ch || f == Format::F32_8ChStd;
}

static bool formatIsStd(Format f) {
    return f == Format::S16_8ChStd || f == Format::F32_8ChStd;
}

static int channelsForFormat(Format f);
static void LogAudio(const char* fmt, std::uint32_t a, std::uint32_t b, const char* c);

static int channelsForFormat(Format f) {
    switch (f) {
        case Format::S16Mono:
        case Format::F32Mono:
            return 1;
        case Format::S16Stereo:
        case Format::F32Stereo:
            return 2;
        case Format::S16_8Ch:
        case Format::F32_8Ch:
        case Format::S16_8ChStd:
        case Format::F32_8ChStd:
            return 8;
        default:
            LogAudio("channelsForFormat: unknown format", static_cast<int>(f), 0, nullptr);
            return 2;
    }
}

static SDL_AudioFormat sdlFormat(Format f) {
    return formatIsFloat(f) ? AUDIO_F32SYS : AUDIO_S16SYS;
}

static std::uint32_t bytesPerSample(Format f) {
    return formatIsFloat(f) ? sizeof(float) : sizeof(std::int16_t);
}

struct Port {
    bool used = false;
    int type = 0;
    std::uint32_t samplesNum = 0;
    std::uint32_t freq = 0;
    Format format = Format::Unknown;
    int channels = 0;
    int volume[8] = {};
    std::uint64_t lastOutputTime = 0;
    SDL_AudioDeviceID device = 0;
    SDL_AudioSpec spec = {};
    // diagnostics
    std::uint64_t statStart = 0, statLast = 0, statCalls = 0, statMaxGapUs = 0, statUnderruns = 0, statSumGapUs = 0, statNonZero = 0;
    std::uint32_t statPeak = 0;
};

static std::mutex g_mutex;
static std::mutex g_portIo[PORTS_MAX];   // serialises output per port; the global lock must never be held while waiting for a drain
static Port g_ports[PORTS_MAX];
static bool g_sdlInitialized = false;

static bool ensureSdlAudio() {
    if (g_sdlInitialized) {
        return true;
    }
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        return false;
    }
    g_sdlInitialized = true;
    return true;
}

static bool openDevice(Port& port) {
    if (!ensureSdlAudio()) {
        return false;
    }
    SDL_AudioSpec desired{};
    desired.freq = static_cast<int>(port.freq);
    desired.format = sdlFormat(port.format);
    desired.channels = static_cast<Uint8>(port.channels);
    desired.samples = static_cast<Uint16>(port.samplesNum);
    desired.callback = nullptr;
    SDL_AudioSpec obtained{};
    port.device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (port.device == 0) {
        return false;
    }
    port.spec = obtained;
    SDL_PauseAudioDevice(port.device, 0);
    return true;
}

static void closeDevice(Port& port) {
    if (port.device != 0 && SDL_WasInit(SDL_INIT_AUDIO) != 0) {
        SDL_ClearQueuedAudio(port.device);
        SDL_CloseAudioDevice(port.device);
    }
    port.device = 0;
    port.spec = {};
}

static constexpr std::uint32_t STD_8CH_MAP[8] = {0, 1, 2, 3, 6, 7, 4, 5};

static const void* prepareBuffer(const Port& port, const void* data, std::vector<std::uint8_t>& buf) {
    const auto frames = port.samplesNum;
    const auto ch = static_cast<std::uint32_t>(port.channels);
    const auto bps = bytesPerSample(port.format);
    const auto size = frames * ch * bps;

    bool volumeChanged = false;
    for (std::uint32_t i = 0; i < ch; i++) {
        if (port.volume[i] != DEFAULT_VOLUME) {
            volumeChanged = true;
            break;
        }
    }

    if (!volumeChanged && !formatIsStd(port.format)) {
        return data;
    }

    buf.resize(size);
    const bool isStd = formatIsStd(port.format) && ch == 8;

    if (formatIsFloat(port.format)) {
        auto* dst = reinterpret_cast<float*>(buf.data());
        const auto* src = static_cast<const float*>(data);
        for (std::uint32_t fr = 0; fr < frames; fr++) {
            for (std::uint32_t c = 0; c < ch; c++) {
                const auto srcCh = isStd ? STD_8CH_MAP[c] : c;
                dst[fr * ch + c] = src[fr * ch + srcCh] *
                    (static_cast<float>(port.volume[c]) / static_cast<float>(DEFAULT_VOLUME));
            }
        }
    } else {
        auto* dst = reinterpret_cast<std::int16_t*>(buf.data());
        const auto* src = static_cast<const std::int16_t*>(data);
        for (std::uint32_t fr = 0; fr < frames; fr++) {
            for (std::uint32_t c = 0; c < ch; c++) {
                const auto srcCh = isStd ? STD_8CH_MAP[c] : c;
                std::int64_t s = static_cast<std::int64_t>(src[fr * ch + srcCh]) *
                    port.volume[c] / DEFAULT_VOLUME;
                s = std::clamp(s,
                    static_cast<std::int64_t>(std::numeric_limits<std::int16_t>::min()),
                    static_cast<std::int64_t>(std::numeric_limits<std::int16_t>::max()));
                dst[fr * ch + c] = static_cast<std::int16_t>(s);
            }
        }
    }
    return buf.data();
}

static constexpr int AUDIO_ERROR_INVALID_PORT = -2144993277;  // 0x80260303
static constexpr int AUDIO_ERROR_INVALID_VALUE = -2144993278; // 0x80260302
static int audioLogs = 0;

static void LogAudio(const char* fmt, std::uint32_t a, std::uint32_t b, const char* c) {
    if (audioLogs < 40) {
        ++audioLogs;
        std::fprintf(stderr, "[AOUT] %s (0x%x, 0x%x, %s)\n", fmt, a, b, c ? c : "");
        std::fflush(stderr);
    }
}

// Returns 0 on success, negative SCE error on failure. Audio must never
// terminate the game: an unopenable device means silent, timed drop.
static int queueAudio(Port& port, const void* data) {
    if (port.type == PORT_TYPE_VIBRATION) {
        return 0;  // gamepad rumble, intentionally silent here
    }
    if (data == nullptr) {
        LogAudio("queueAudio: null data", port.type, port.samplesNum, nullptr);
        return AUDIO_ERROR_INVALID_VALUE;
    }
    if (port.device == 0) {
        if (!openDevice(port)) {
            const std::uint64_t blockUs = port.freq != 0
                ? (1000000ULL * port.samplesNum) / port.freq : 20000;
            struct timespec req{};
            req.tv_sec = static_cast<time_t>(blockUs / 1000000ULL);
            req.tv_nsec = static_cast<long>((blockUs % 1000000ULL) * 1000ULL);
            nanosleep(&req, nullptr);
            LogAudio("queueAudio: no audio device, dropped frame",
                port.samplesNum, port.freq, SDL_GetError());
            return 0;
        }
    }

    {
        const auto nowUs = sceKernelGetProcessTime();
        if (port.statStart == 0) port.statStart = port.statLast = nowUs;
        const auto gap = nowUs >= port.statLast ? nowUs - port.statLast : 0;
        port.statLast = nowUs;
        ++port.statCalls;
        port.statSumGapUs += gap;
        if (gap > port.statMaxGapUs) port.statMaxGapUs = gap;
        if (SDL_GetQueuedAudioSize(port.device) == 0) ++port.statUnderruns;
        std::uint32_t peak = 0;
        if (VerboseLog()) {
        const auto sampleCount = port.samplesNum * static_cast<std::uint32_t>(port.channels);
        if (formatIsFloat(port.format)) {
            const auto* f = static_cast<const float*>(data);
            for (std::uint32_t i = 0; i < sampleCount; ++i) peak = std::max(peak, static_cast<std::uint32_t>(std::min(1.0f, std::abs(f[i])) * 32767.0f));
        } else {
            const auto* v = static_cast<const std::int16_t*>(data);
            for (std::uint32_t i = 0; i < sampleCount; ++i) peak = std::max(peak, static_cast<std::uint32_t>(std::abs(static_cast<int>(v[i]))));
        }
        }
        port.statPeak = std::max(port.statPeak, peak);
        if (peak != 0) ++port.statNonZero;
        if (nowUs - port.statStart >= 5000000ULL) {
            if (VerboseLog()) std::fprintf(stderr, "[AOUTSTAT] type=%d fmt=%d ch=%d frames=%u freq=%u calls=%llu avgGap=%.1fms maxGap=%.1fms underruns=%llu nonSilentBuffers=%llu peak=%u queued=%u dev=%d\n",
                port.type, static_cast<int>(port.format), port.channels, port.samplesNum, port.freq, static_cast<unsigned long long>(port.statCalls),
                port.statCalls ? port.statSumGapUs / 1000.0 / port.statCalls : 0.0, port.statMaxGapUs / 1000.0, static_cast<unsigned long long>(port.statUnderruns),
                static_cast<unsigned long long>(port.statNonZero), port.statPeak, SDL_GetQueuedAudioSize(port.device), static_cast<int>(port.device));
            std::fflush(stderr);
            port.statStart = nowUs; port.statCalls = port.statSumGapUs = port.statMaxGapUs = port.statUnderruns = port.statNonZero; port.statPeak = 0;
        }
    }
    std::vector<std::uint8_t> prepareBuf;
    const void* prepared = prepareBuffer(port, data, prepareBuf);
    std::uint32_t preparedSize = port.samplesNum *
        static_cast<std::uint32_t>(port.channels) * bytesPerSample(port.format);

    // The DualSense speaker port is fed 256-frame grains every ~16 ms (measured from a raw dump: contiguous waveform, median gap 16.3 ms),
    // i.e. ~16 kHz of content, although the port is opened as 48 kHz. Playing it as 48 kHz turns every grain into 5.3 ms of sound plus ~10 ms of
    // silence. padspk_rate.txt (a single integer) overrides the rate for experiments.
    std::uint32_t sourceFreq = port.freq;
    if (port.type == 4) {
        static const std::uint32_t padRate = [] {
            std::uint32_t rate = 16000;
            if (std::FILE* f = std::fopen("padspk_rate.txt", "rb")) {
                unsigned value = 0;
                if (std::fscanf(f, "%u", &value) == 1 && value >= 4000 && value <= 96000) rate = value;
                std::fclose(f);
            }
            std::fprintf(stderr, "[AOUT] pad-speaker source rate %u Hz\n", rate);
            return rate;
        }();
        sourceFreq = padRate;
    }
    std::uint32_t bufferFrames = port.samplesNum;
    std::vector<std::int16_t> padUp;
    if (port.type == 4 && !formatIsFloat(port.format) && port.channels == 1 && sourceFreq != 0 && port.spec.freq > static_cast<int>(sourceFreq) && port.spec.freq % sourceFreq == 0) {
        // SDL's converter leaves imaging noise on a 3x upsample; interpolate with a Catmull-Rom cubic, carrying the last samples across grains.
        const std::uint32_t ratio = static_cast<std::uint32_t>(port.spec.freq) / sourceFreq;
        static float h[4] = {0, 0, 0, 0};
        const auto* in = static_cast<const std::int16_t*>(prepared);
        padUp.resize(static_cast<std::size_t>(port.samplesNum) * ratio);
        std::size_t o = 0;
        for (std::uint32_t i = 0; i < port.samplesNum; ++i) {
            h[0] = h[1]; h[1] = h[2]; h[2] = h[3]; h[3] = static_cast<float>(in[i]);
            for (std::uint32_t k = 0; k < ratio; ++k) {
                const float t = static_cast<float>(k) / static_cast<float>(ratio);
                const float v = 0.5f * (2.0f * h[1] + (-h[0] + h[2]) * t + (2.0f * h[0] - 5.0f * h[1] + 4.0f * h[2] - h[3]) * t * t + (-h[0] + 3.0f * h[1] - 3.0f * h[2] + h[3]) * t * t * t);
                padUp[o++] = static_cast<std::int16_t>(std::clamp(v, -32768.0f, 32767.0f));
            }
        }
        prepared = padUp.data();
        preparedSize = static_cast<std::uint32_t>(padUp.size() * sizeof(std::int16_t));
        bufferFrames = static_cast<std::uint32_t>(padUp.size());
        sourceFreq = static_cast<std::uint32_t>(port.spec.freq);
    }
    SDL_AudioCVT cvt{};
    const int cvtResult = SDL_BuildAudioCVT(
        &cvt,
        sdlFormat(port.format), static_cast<Uint8>(port.channels), static_cast<int>(sourceFreq),
        port.spec.format, port.spec.channels, port.spec.freq);

    if (cvtResult < 0) {
        LogAudio("SDL_BuildAudioCVT failed", port.samplesNum, port.freq, SDL_GetError());
        return AUDIO_ERROR_INVALID_VALUE;
    }

    const void* queueData = prepared;
    std::uint32_t queueSize = preparedSize;
    std::vector<std::uint8_t> convertBuf;

    if (cvtResult > 0) {
        convertBuf.resize(static_cast<std::size_t>(preparedSize) * cvt.len_mult);
        std::memcpy(convertBuf.data(), prepared, preparedSize);
        cvt.buf = convertBuf.data();
        cvt.len = static_cast<int>(preparedSize);
        if (SDL_ConvertAudio(&cvt) < 0) {
            LogAudio("SDL_ConvertAudio failed", port.samplesNum, port.freq, SDL_GetError());
            return AUDIO_ERROR_INVALID_VALUE;
        }
        queueData = cvt.buf;
        queueSize = static_cast<std::uint32_t>(cvt.len_cvt);
    }

    const std::uint64_t bufferUs = sourceFreq != 0
        ? (1000000ULL * bufferFrames) / sourceFreq
        : 0;
    const std::uint32_t buffers = bufferUs != 0
        ? static_cast<std::uint32_t>((TARGET_LATENCY_US + bufferUs - 1) / bufferUs)
        : 2u;
    const std::uint32_t minQueued = queueSize * std::clamp(buffers, 2u, 16u);
    const std::uint64_t waitStart = sceKernelGetProcessTime();

    // Underrun (or first buffer): queue a little silence so scheduling hitches on the game side do not become audible gaps.
    if (SDL_GetQueuedAudioSize(port.device) == 0) {
        const auto bytesPerFrame = static_cast<std::uint32_t>(port.spec.channels) * (SDL_AUDIO_BITSIZE(port.spec.format) / 8u);
        const auto primeBytes = static_cast<std::uint32_t>((static_cast<std::uint64_t>(port.spec.freq) * PRIME_US / 1000000ULL)) * bytesPerFrame;
        if (primeBytes != 0) {
            std::vector<std::uint8_t> silence(primeBytes, static_cast<std::uint8_t>(port.spec.silence));
            SDL_QueueAudio(port.device, silence.data(), primeBytes);
        }
    }

    while (SDL_GetQueuedAudioSize(port.device) > minQueued) {
        if (sceKernelGetProcessTime() - waitStart > DRAIN_TIMEOUT_US) {
            SDL_ClearQueuedAudio(port.device);
            break;
        }
        SDL_Delay(static_cast<Uint32>(DRAIN_SLEEP_US / 1000ULL));
    }

    if (SDL_QueueAudio(port.device, queueData, queueSize) < 0) {
        LogAudio("SDL_QueueAudio failed", port.samplesNum, port.freq, SDL_GetError());
        return AUDIO_ERROR_INVALID_PORT;
    }
    // APS5_LOG_OUT("device=%u type=%d bytes=%u queued=%u", port.device, port.type, queueSize, SDL_GetQueuedAudioSize(port.device));
    return 0;
}

static bool portTypeValid(int type) {
    return (type >= PORT_TYPE_MAIN && type <= PORT_TYPE_PADSPK) ||
           type == PORT_TYPE_VIBRATION ||
           type == PORT_TYPE_AUDIO3D ||
           type == PORT_TYPE_AUX;
}

static Port* getPort(int handle) {
    const int idx = handle - 1;
    if (idx < 0 || idx >= PORTS_MAX || !g_ports[idx].used) {
        return nullptr;
    }
    return &g_ports[idx];
}

extern "C" {

int APS5_VABI sceAudioOutInit() {
    return 0;
}

int APS5_VABI sceAudioOutOpen(int userId, int type, int index, std::uint32_t len,
    std::uint32_t freq, std::uint32_t param) {
    (void)userId;
    if (!portTypeValid(type)) {
        return -2144993270;
    }
    if (index != 0) {
        LogAudio("sceAudioOutOpen: unsupported index", type, index, nullptr);
        return AUDIO_ERROR_INVALID_VALUE;
    }

    Format format = Format::Unknown;
    switch (param & FORMAT_MASK) {
        case 0: format = Format::S16Mono; break;
        case 1: format = Format::S16Stereo; break;
        case 2: format = Format::S16_8Ch; break;
        case 3: format = Format::F32Mono; break;
        case 4: format = Format::F32Stereo; break;
        case 5: format = Format::F32_8Ch; break;
        case 6: format = Format::S16_8ChStd; break;
        case 7: format = Format::F32_8ChStd; break;
        default:
            LogAudio("sceAudioOutOpen: unknown format param", type, param, nullptr);
            return AUDIO_ERROR_INVALID_VALUE;
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    for (int i = 0; i < PORTS_MAX; i++) {
        if (!g_ports[i].used) {
            Port& port = g_ports[i];
            port.used = true;
            port.type = type;
            port.samplesNum = len;
            port.freq = freq;
            port.format = format;
            port.channels = channelsForFormat(format);
            port.lastOutputTime = 0;
            port.statStart = 0;
            std::fprintf(stderr, "[AOUT] open handle=%d type=%d frames=%u freq=%u fmt=%d ch=%d\n", i + 1, type, len, freq, static_cast<int>(format), port.channels);
            std::fflush(stderr);
            for (int c = 0; c < port.channels; c++) {
                port.volume[c] = DEFAULT_VOLUME;
            }
            if (type != PORT_TYPE_VIBRATION) {
                openDevice(port);
            }
            return i + 1;
        }
    }
    return -2144993275;
}

int APS5_VABI sceAudioOutClose(int handle) {
    if (handle < 1 || handle > PORTS_MAX) {
        return -2144993277;
    }
    std::lock_guard<std::mutex> io(g_portIo[handle - 1]);
    std::lock_guard<std::mutex> lock(g_mutex);
    Port* port = getPort(handle);
    if (port == nullptr) {
        return -2144993277;
    }
    closeDevice(*port);
    *port = Port{};
    return 0;
}

int APS5_VABI sceAudioOutOutput(int handle, const void* ptr) {
    Port* port = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        port = getPort(handle);
    }
    if (port == nullptr) {
        return -2144993277;
    }
    std::lock_guard<std::mutex> io(g_portIo[handle - 1]);

    const std::uint64_t blockUs = (1000000ULL * port->samplesNum) / port->freq;
    const std::uint64_t now = sceKernelGetProcessTime();
    const std::uint64_t next = port->lastOutputTime + blockUs;
    if (next > now && port->device == 0) {
        const std::uint64_t waitUs = next - now;
        struct timespec req{};
        req.tv_sec = static_cast<time_t>(waitUs / 1000000ULL);
        req.tv_nsec = static_cast<long>((waitUs % 1000000ULL) * 1000ULL);
        nanosleep(&req, nullptr);
    }

    queueAudio(*port, ptr);
    port->lastOutputTime = sceKernelGetProcessTime();
    return static_cast<int>(port->samplesNum);
}

int APS5_VABI sceAudioOutOutputs(AudioOutOutputParam* param, std::uint32_t num) {
    if (param == nullptr || num == 0) {
        return -2144993276;
    }

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        for (std::uint32_t i = 0; i < num; i++) {
            if (getPort(param[i].handle) == nullptr) {
                return -2144993277;
            }
        }
    }
    std::vector<std::unique_lock<std::mutex>> ioLocks;
    {
        std::vector<int> handles;
        for (std::uint32_t i = 0; i < num; i++) handles.push_back(param[i].handle);
        std::sort(handles.begin(), handles.end());
        handles.erase(std::unique(handles.begin(), handles.end()), handles.end());
        for (const int h : handles) ioLocks.emplace_back(g_portIo[h - 1]);
    }

    Port& first = *getPort(param[0].handle);
    const std::uint64_t blockUs = (1000000ULL * first.samplesNum) / first.freq;
    const std::uint64_t now = sceKernelGetProcessTime();

    std::uint64_t maxWait = 0;
    for (std::uint32_t i = 0; i < num; i++) {
        Port& p = *getPort(param[i].handle);
        const std::uint64_t next = p.lastOutputTime + blockUs;
        const std::uint64_t wait = next > now ? next - now : 0;
        if (wait > maxWait) {
            maxWait = wait;
        }
    }

    bool anyDevice = false;
    for (std::uint32_t i = 0; i < num; i++) {
        if (getPort(param[i].handle)->device != 0) {
            anyDevice = true;
            break;
        }
    }

    if (maxWait != 0 && !anyDevice) {
        struct timespec req{};
        req.tv_sec = static_cast<time_t>(maxWait / 1000000ULL);
        req.tv_nsec = static_cast<long>((maxWait % 1000000ULL) * 1000ULL);
        nanosleep(&req, nullptr);
    }

    for (std::uint32_t i = 0; i < num; i++) {
        queueAudio(*getPort(param[i].handle), param[i].ptr);
    }

    const std::uint64_t done = sceKernelGetProcessTime();
    for (std::uint32_t i = 0; i < num; i++) {
        getPort(param[i].handle)->lastOutputTime = done;
    }

    return static_cast<int>(first.samplesNum);
}

int APS5_VABI sceAudioOutSetVolume(int handle, std::uint32_t flag, int* vol) {
    if (vol == nullptr) {
        return -2144993276;
    }
    std::lock_guard<std::mutex> lock(g_mutex);
    Port* port = getPort(handle);
    if (port == nullptr) {
        return -2144993277;
    }
    const bool isStd = formatIsStd(port->format);
    for (int i = 0; i < port->channels; i++, flag >>= 1u) {
        if ((flag & 1u) == 0) {
            continue;
        }
        int srcIdx = i;
        if (isStd) {
            if (i == 4) srcIdx = 6;
            else if (i == 5) srcIdx = 7;
            else if (i == 6) srcIdx = 4;
            else if (i == 7) srcIdx = 5;
        }
        port->volume[i] = vol[srcIdx];
    }
    return 0;
}

int APS5_VABI sceAudioOutGetPortState(int handle, AudioOutPortState* state) {
    if (state == nullptr) {
        return -2144993276;
    }
    std::lock_guard<std::mutex> lock(g_mutex);
    Port* port = getPort(handle);
    if (port == nullptr) {
        return -2144993277;
    }
    state->rerouteCounter = 0;
    state->volume = 127;
    state->flag = 0;
    state->activeState = 0;
    state->reserved[0] = 0;
    switch (port->type) {
        case PORT_TYPE_MAIN:
        case PORT_TYPE_BGM:
        case PORT_TYPE_AUDIO3D:
            state->output = 1;
            state->channel = static_cast<std::uint8_t>(port->channels > 2 ? 2 : port->channels);
            break;
        case PORT_TYPE_VOICE:
        case PORT_TYPE_PERSONAL:
            state->output = 0x40;
            state->channel = 1;
            break;
        case PORT_TYPE_PADSPK:
        case PORT_TYPE_VIBRATION:
            state->output = 4;
            state->channel = 1;
            break;
        case PORT_TYPE_AUX:
            state->output = 0x80;
            state->channel = 0;
            break;
        default:
            LogAudio("GetPortState: unknown port type", port->type, 0, nullptr);
            state->output = 0x40;
            state->channel = 1;
            break;
    }
    return 0;
}

}
