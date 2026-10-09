#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <cstdarg>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <system_error>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/Socket/include/SocketRuntime.hpp"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "prx/libkernel/File/include/FileFlags.hpp"
#include "prx/libkernel/File/include/NativeStat.hpp"

#include <cerrno>
#include <map>

extern "C" {
// Directory descriptors on Windows: a directory cannot be opened with _wopen, so opening one returns a real descriptor for NUL that is
// tracked here with its path and a cursor for getdents. See the definitions at the end of Stdio.cpp.
int FileOpenDirectoryFd(const char* utf8Path);
int FileIsDirectoryFd(int fd);
int FileDirectoryFdPath(int fd, char* out, unsigned size);
void FileForgetDirectoryFd(int fd);
int FileReadDirectoryFd(int fd, char* buf, int nbytes);
}

extern "C" {
void FileRecordGuestFlags(int fd, int guestFlags);
int FileGetGuestFlags(int fd);
void FileForgetGuestFlags(int fd);
}

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <direct.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

namespace {

// Guest file descriptors alias the host CRT descriptor table: sceKernelOpen
// returns native fds, so every raw syscall wrapper here operates on them
// directly.

#ifdef _WIN32

using NativeStatT = struct __stat64;

int NativeFstat(int fd, NativeStatT* st) { return ::_fstat64(fd, st); }
int NativeStatPath(const std::filesystem::path& p, NativeStatT* st) { return ::_wstat64(p.wstring().c_str(), st); }
int NativeOpenPath(const std::filesystem::path& p, int flags, int mode) { return ::_wopen(p.wstring().c_str(), flags, mode); }
std::int64_t NativeLseek(int fd, std::int64_t off, int whence) { return ::_lseeki64(fd, off, whence); }
int NativeTruncate(int fd, std::int64_t len) { return ::_chsize_s(fd, static_cast<long long>(len)); }
int NativeMkdirPath(const std::filesystem::path& p) { return ::_wmkdir(p.wstring().c_str()); }
int NativeAccess(const std::filesystem::path& p, int mode) { return ::_waccess(p.wstring().c_str(), mode); }

constexpr int kSeekSet = 0, kSeekCur = 1, kSeekEnd = 2;

#else

using NativeStatT = struct stat;

int NativeFstat(int fd, NativeStatT* st) { return ::fstat(fd, st); }
int NativeStatPath(const std::filesystem::path& p, NativeStatT* st) { return ::stat(p.c_str(), st); }
int NativeOpenPath(const std::filesystem::path& p, int flags, int mode) { return ::open(p.c_str(), flags, mode); }
std::int64_t NativeLseek(int fd, std::int64_t off, int whence) { return ::lseek(fd, static_cast<off_t>(off), whence); }
int NativeTruncate(int fd, std::int64_t len) { return ::ftruncate(fd, static_cast<off_t>(len)) == 0 ? 0 : errno; }
int NativeMkdirPath(const std::filesystem::path& p) { return ::mkdir(p.c_str(), 0777); }
int NativeAccess(const std::filesystem::path& p, int mode) { return ::access(p.c_str(), mode); }

constexpr int kSeekSet = SEEK_SET, kSeekCur = SEEK_CUR, kSeekEnd = SEEK_END;

#endif

int NativeReadOnce(int fd, void* buf, std::size_t n) {
#ifdef _WIN32
    if (n > 0x7fffffffu) n = 0x7fffffffu;
    return ::_read(fd, buf, static_cast<unsigned int>(n));
#else
    return static_cast<int>(::read(fd, buf, n));
#endif
}

int NativeWriteOnce(int fd, const void* buf, std::size_t n) {
#ifdef _WIN32
    if (n > 0x7fffffffu) n = 0x7fffffffu;
    return ::_write(fd, buf, static_cast<unsigned int>(n));
#else
    return static_cast<int>(::write(fd, buf, n));
#endif
}

// UCRT has no pread/pwrite; emulate with an atomic seek+transfer.
std::mutex g_offsetLock;

std::int64_t NativePread(int fd, void* buf, std::size_t nbytes, std::int64_t offset) {
    std::lock_guard lock(g_offsetLock);
    const std::int64_t saved = NativeLseek(fd, 0, kSeekCur);
    if (NativeLseek(fd, offset, kSeekSet) < 0) return -1;
    std::size_t done = 0;
    auto* out = static_cast<std::byte*>(buf);
    while (done < nbytes) {
        const int n = NativeReadOnce(fd, out + done, nbytes - done);
        if (n < 0) { NativeLseek(fd, saved, kSeekSet); return -1; }
        if (n == 0) break;
        done += static_cast<std::size_t>(n);
    }
    NativeLseek(fd, saved, kSeekSet);
    return static_cast<std::int64_t>(done);
}

std::int64_t NativePwrite(int fd, const void* buf, std::size_t nbytes, std::int64_t offset) {
    std::lock_guard lock(g_offsetLock);
    const std::int64_t saved = NativeLseek(fd, 0, kSeekCur);
    if (NativeLseek(fd, offset, kSeekSet) < 0) return -1;
    std::size_t done = 0;
    const auto* in = static_cast<const std::byte*>(buf);
    while (done < nbytes) {
        const int n = NativeWriteOnce(fd, in + done, nbytes - done);
        if (n <= 0) { NativeLseek(fd, saved, kSeekSet); return -1; }
        done += static_cast<std::size_t>(n);
    }
    NativeLseek(fd, saved, kSeekSet);
    return static_cast<std::int64_t>(done);
}

std::int64_t NativeReadFull(int fd, void* buf, std::size_t nbytes) {
    std::size_t done = 0;
    auto* out = static_cast<std::byte*>(buf);
    while (done < nbytes) {
        const int n = NativeReadOnce(fd, out + done, nbytes - done);
        if (n < 0) return -1;
        if (n == 0) break;
        done += static_cast<std::size_t>(n);
    }
    return static_cast<std::int64_t>(done);
}

std::int64_t NativeWriteFull(int fd, const void* buf, std::size_t nbytes) {
    std::size_t done = 0;
    const auto* in = static_cast<const std::byte*>(buf);
    while (done < nbytes) {
        const int n = NativeWriteOnce(fd, in + done, nbytes - done);
        if (n <= 0) return -1;
        done += static_cast<std::size_t>(n);
    }
    return static_cast<std::int64_t>(done);
}

void FillFromNative(const NativeStatT& st, FileStat* sb) {
    *sb = FileStat{};
    sb->st_mode = static_cast<std::uint16_t>(st.st_mode);
    sb->st_size = static_cast<std::int64_t>(st.st_size);
    sb->st_dev = static_cast<std::uint32_t>(st.st_dev);
    sb->st_ino = static_cast<std::uint32_t>(st.st_ino);
    sb->st_nlink = static_cast<std::uint16_t>(st.st_nlink ? st.st_nlink : 1);
    sb->st_blksize = 512;
    sb->st_blocks = (sb->st_size + 511LL) / 512LL;
    sb->st_atim.tv_sec = static_cast<std::int64_t>(st.st_atime);
    sb->st_mtim.tv_sec = static_cast<std::int64_t>(st.st_mtime);
    sb->st_ctim.tv_sec = static_cast<std::int64_t>(st.st_ctime);
    sb->st_birthtim.tv_sec = static_cast<std::int64_t>(st.st_ctime);
}

// TEMPORARY MEASUREMENT: trace of guest file-system calls outside the read-only game image (find where the game keeps its saves).
void FileTrace(const char* op, const char* path, long long result) {
    static std::atomic<int> traced{0};
    if (path == nullptr || std::strncmp(path, "/app0", 5) == 0 || traced.fetch_add(1) >= 600) return;
    std::fprintf(stderr, "[FS] %s %s -> %lld (errno %d)\n", op, path, result, result < 0 ? errno : 0);
    std::fflush(stderr);
}

int DoStatPath(const char* path, FileStat* sb) {
    if (!path || !sb) { errno = EINVAL; return -1; }
    NativeStatT st{};
    if (NativeStatPath(ResolvePath_nid_no_patch(path), &st) != 0) {
        FileTrace("stat", path, -1);
        return -1;
    }
    FileTrace("stat", path, 0);
    FillFromNative(st, sb);
    return 0;
}

int DoFstat(int fd, FileStat* sb) {
    if (!sb) { errno = EINVAL; return -1; }
    NativeStatT st{};
#ifdef _WIN32
    if (char directory[1024]; FileDirectoryFdPath(fd, directory, sizeof(directory)) != 0) {
        if (NativeStatPath(std::filesystem::path(directory), &st) != 0) return -1;
        FillFromNative(st, sb);
        return 0;
    }
#endif
    if (NativeFstat(fd, &st) != 0) return -1;
    FillFromNative(st, sb);
    return 0;
}

int MapGuestFlags(int guestFlags) {
    int f = 0;
    const int acc = guestFlags & SCE_KERNEL_O_ACCMODE;
    if (acc == SCE_KERNEL_O_RDONLY) f |= _O_RDONLY;
    else if (acc == SCE_KERNEL_O_WRONLY) f |= _O_WRONLY;
    else if (acc == SCE_KERNEL_O_RDWR) f |= _O_RDWR;
    else { errno = EINVAL; return -1; }
    if (guestFlags & SCE_KERNEL_O_APPEND) f |= _O_APPEND;
    if (guestFlags & SCE_KERNEL_O_CREAT) f |= _O_CREAT;
    if (guestFlags & SCE_KERNEL_O_TRUNC) f |= _O_TRUNC;
    if (guestFlags & SCE_KERNEL_O_EXCL) f |= _O_EXCL;
    f |= _O_BINARY;
    return f;
}

int DoOpen(const char* path, int flags, int mode) {
    if (!path) { errno = EINVAL; return -1; }
    const int nativeFlags = MapGuestFlags(flags);
    if (nativeFlags < 0) return -1;
#ifdef _WIN32
    (void)mode;
    int fd = NativeOpenPath(ResolvePath_nid_no_patch(path), nativeFlags, 0644);
    if (fd < 0) {
        const auto native = ResolvePath_nid_no_patch(path);
        std::error_code directoryError;
        if (std::filesystem::is_directory(native, directoryError)) {
            fd = FileOpenDirectoryFd(native.string().c_str());
            if (fd >= 0) FileRecordGuestFlags(fd, flags);
        }
    }
#else
    const int fd = NativeOpenPath(ResolvePath_nid_no_patch(path), nativeFlags, static_cast<mode_t>(mode));
#endif
    {
        // TEMPORARY MEASUREMENT: every open outside the read-only game image, with its result (find where the game keeps its saves).
        static std::atomic<int> logged{0};
        if (std::strncmp(path, "/app0", 5) != 0 && logged.fetch_add(1) < 400) {
            std::fprintf(stderr, "[OPEN2] %s flags=0x%x -> %s (%d)\n", path, static_cast<unsigned>(flags), fd >= 0 ? "ok" : "FAIL", fd >= 0 ? fd : errno);
            std::fflush(stderr);
        }
    }
    if (fd >= 0) {
        FileRecordGuestFlags(fd, flags);
    } else {
        // Failed opens are the usual reason a title loops on a loading screen: report the first ones with the reason.
        static std::atomic<int> failedOpens{0};
        if (failedOpens.fetch_add(1) < 100) {
            std::fprintf(stderr, "[FILE] open failed: path=%s flags=0x%x errno=%d\n", path, static_cast<unsigned>(flags), errno);
            std::fflush(stderr);
        }
    }
    return fd;
}

}

extern "C" {

int APS5_VABI rename_nid_postfix(const char* from, const char* to) {
 if (!from || !to) return -1;
 std::error_code ec;
 std::filesystem::rename(ResolvePath_nid_no_patch(from), ResolvePath_nid_no_patch(to), ec);
 return ec ? -1 : 0;
}

int APS5_VABI unlink_nid_postfix(const char* path) {
 if (!path) return -1;
#ifdef _WIN32
 return ::_wunlink(ResolvePath_nid_no_patch(path).wstring().c_str());
#else
 return ::unlink(ResolvePath_nid_no_patch(path).c_str());
#endif
}

int APS5_VABI rmdir_nid_postfix(const char* path) {
 if (!path) return -1;
#ifdef _WIN32
 return ::_wrmdir(ResolvePath_nid_no_patch(path).wstring().c_str());
#else
 return ::rmdir(ResolvePath_nid_no_patch(path).c_str());
#endif
}

int APS5_VABI fcntl_nid_postfix(int d, int cmd, ...) {
 // FreeBSD cmds: 0 DUP, 1 GETFD, 2 SETFD, 3 GETFL, 4 SETFL, 7/8 locks, 10 GETOWN...
#ifdef _WIN32
 __builtin_sysv_va_list args;
 __builtin_sysv_va_start(args, cmd);
 const long arg = __builtin_va_arg(args, long);
 __builtin_sysv_va_end(args);
#else
 va_list args;
 va_start(args, cmd);
 const long arg = va_arg(args, long);
 va_end(args);
#endif
 if (d >= GuestSockets::FirstDescriptor) return GuestSockets::Fcntl(d, cmd, static_cast<int>(arg));
 switch (cmd) {
  case 3: { // F_GETFL: report at least the access mode so callers can branch on it.
   const int recorded = FileGetGuestFlags(d);
   if (recorded < 0) return 0;
   return recorded & ~(SCE_KERNEL_O_CREAT | SCE_KERNEL_O_EXCL | SCE_KERNEL_O_TRUNC);
  }
  case 4: return 0; // F_SETFL: append/nonblock are tracked by the CRT itself for our regular files.
  case 1: return 0; // F_GETFD
  case 2: return 0; // F_SETFD
  case 7: case 8: case 9: case 11: case 12: case 13: return 0; // locking hints: succeed, host files are exclusive enough
  case 0: { // F_DUPFD
#ifdef _WIN32
   const int fd = ::_dup(d);
#else
   const int fd = ::fcntl(d, F_DUPFD, static_cast<int>(arg));
#endif
   return fd;
  }
  default: return 0;
 }
}

int APS5_VABI chmod_nid_postfix(const char* path, int mode) {
 if (!path) return -1;
#ifdef _WIN32
 return ::_wchmod(ResolvePath_nid_no_patch(path).wstring().c_str(), mode);
#else
 return ::chmod(ResolvePath_nid_no_patch(path).c_str(), static_cast<mode_t>(mode));
#endif
}

int APS5_VABI close_nid_postfix(int d) {
    if (d >= GuestSockets::FirstDescriptor) return GuestSockets::Close(d);
    FileForgetGuestFlags(d);
    FileForgetDirectoryFd(d);
#ifdef _WIN32
    return ::_close(d);
#else
    return ::close(d);
#endif
}

int APS5_VABI flock_nid_postfix(int d, int operation) {
 (void)d;
 (void)operation;
 return 0; // advisory locks on private host files: nothing to enforce
}

int64_t APS5_VABI fstat_nid_disambig1_nid_postfix(int d, FileStat* sb) {
 return DoFstat(d, sb);
}

int APS5_VABI ftruncate_nid_postfix(int d, int64_t length) {
 if (length < 0) { errno = EINVAL; return -1; }
 return NativeTruncate(d, length);
}

int64_t APS5_VABI lseek_nid_postfix(int d, int64_t offset, int whence) {
 if (whence < 0 || whence > 2) { errno = EINVAL; return -1; }
 return NativeLseek(d, offset, whence);
}

int APS5_VABI mkdir_nid_postfix(const char* path, uint16_t mode) {
 (void)mode;
 if (!path) return -1;
 const int result = NativeMkdirPath(ResolvePath_nid_no_patch(path));
 FileTrace("mkdir", path, result);
 return result;
}

int APS5_VABI open_nid_postfix(const char* path, int flags, int mode) {
 return DoOpen(path, flags, mode);
}

int64_t APS5_VABI pread_nid_postfix(int d, void* buf, size_t nbytes, int64_t offset) {
 if (!buf && nbytes) { errno = EFAULT; return -1; }
 return NativePread(d, buf, nbytes, offset);
}

int64_t APS5_VABI pwrite_nid_disambig1_nid_postfix(int d, const void* buf, size_t nbytes, int64_t offset) {
 if (!buf && nbytes) { errno = EFAULT; return -1; }
 return NativePwrite(d, buf, nbytes, offset);
}

int64_t APS5_VABI read_nid_postfix(int d, void* buf, uint64_t nbytes) {
 if (!buf && nbytes) { errno = EFAULT; return -1; }
 return NativeReadFull(d, buf, static_cast<std::size_t>(nbytes));
}

int64_t APS5_VABI write_nid_postfix(int d, const char* str, int64_t size) {
 if (!str && size) { errno = EFAULT; return -1; }
 return NativeWriteFull(d, str, static_cast<std::size_t>(size));
}

int APS5_VABI stat_nid_postfix(const char* path, FileStat* sb) {
 return DoStatPath(path, sb);
}

int APS5_VABI sceKernelCheckReachability(const char* path) {
 FileStat sb{};
 return DoStatPath(path, &sb) == 0 ? 0 : -1; // FreeBSD semantics: 0 exists, -1/ENOENT otherwise
}

int APS5_VABI sceKernelFstat(int d, FileStat* sb) {
 return DoFstat(d, sb);
}

int APS5_VABI sceKernelFsync(int fd) {
#ifdef _WIN32
 return ::_commit(fd);
#else
 return ::fsync(fd);
#endif
}

int APS5_VABI sceKernelGetdents(int fd, char* buf, int nbytes) {
 // FreeBSD dirent: [u32 reclen][u32 reserved][u8 type][u8 namelen][name...] aligned to 8.
#ifdef _WIN32
 const int listed = FileReadDirectoryFd(fd, buf, nbytes);
 if (listed == -2) errno = ENOTDIR;
 return listed >= 0 ? listed : -1;
#else
 DIR* dir = ::fdopendir(fd);
 if (!dir) return -1;
 std::size_t used = 0;
 dirent* entry;
 while ((entry = ::readdir(dir)) != nullptr) {
  const std::size_t nameLen = std::strlen(entry->d_name) + 1;
  const std::size_t fixed = 16;
  std::size_t reclen = (fixed + nameLen + 7) & ~std::size_t(7);
  if (used + reclen > static_cast<std::size_t>(nbytes)) break;
  auto* out = reinterpret_cast<std::uint8_t*>(buf) + used;
  std::memset(out, 0, reclen);
  *reinterpret_cast<std::uint32_t*>(out) = static_cast<std::uint32_t>(reclen);
  out[8] = entry->d_type;
  out[9] = static_cast<std::uint8_t>(nameLen);
  std::memcpy(out + fixed, entry->d_name, nameLen);
  used += reclen;
 }
 return static_cast<int>(used);
#endif
}

int APS5_VABI sceKernelGetdirentries(int fd, char* buf, int nbytes, int64_t* basep) {
 const int res = sceKernelGetdents(fd, buf, nbytes);
 if (basep) *basep = 0;
 return res;
}

int APS5_VABI sceKernelMkdir(const char* path, uint16_t mode) {
 return mkdir_nid_postfix(path, mode);
}

int64_t APS5_VABI sceKernelPread(int d, void* buf, size_t nbytes, int64_t offset) {
 return pread_nid_postfix(d, buf, nbytes, offset);
}

int64_t APS5_VABI sceKernelPwrite(int d, const void* buf, size_t nbytes, int64_t offset) {
 return pwrite_nid_disambig1_nid_postfix(d, buf, nbytes, offset);
}

int APS5_VABI sceKernelRename(const char* from, const char* to) {
 return rename_nid_postfix(from, to);
}

int APS5_VABI sceKernelRmdir(const char* path) {
 return rmdir_nid_postfix(path);
}

int APS5_VABI access_nid_postfix(const char* path, int mode) {
 if (!path) { errno = EINVAL; return -1; }
 const int result = NativeAccess(ResolvePath_nid_no_patch(path), mode);
 FileTrace("access", path, result);
 return result;
}

}

namespace {
std::mutex g_flagsMutex;
std::map<int, int> g_guestFlagsByFd;
}

extern "C" {

void FileRecordGuestFlags(int fd, int guestFlags) {
 std::lock_guard lock(g_flagsMutex);
 g_guestFlagsByFd[fd] = guestFlags;
}

int FileGetGuestFlags(int fd) {
 std::lock_guard lock(g_flagsMutex);
 const auto it = g_guestFlagsByFd.find(fd);
 return it == g_guestFlagsByFd.end() ? -1 : it->second;
}

void FileForgetGuestFlags(int fd) {
 std::lock_guard lock(g_flagsMutex);
 g_guestFlagsByFd.erase(fd);
}

}

// ---------------------------------------------------------------------------------------------------------------------------------------
// Windows directory descriptors (open on a directory, fstat, getdents, close).
#ifdef _WIN32
#include <fcntl.h>
#include <vector>
#include <string>

namespace {
struct DirectoryState {
    std::string path;
    std::vector<std::pair<std::string, std::uint8_t>> entries;
    std::size_t cursor = 0;
    bool loaded = false;
};
std::mutex g_directoryMutex;
std::map<int, DirectoryState>& Directories() {
    static std::map<int, DirectoryState> directories;
    return directories;
}
}

extern "C" int FileOpenDirectoryFd(const char* utf8Path) {
    const int fd = ::_open("NUL", _O_RDONLY | _O_BINARY);
    if (fd < 0) return -1;
    std::lock_guard lock(g_directoryMutex);
    Directories()[fd] = DirectoryState{utf8Path, {}, 0, false};
    return fd;
}

extern "C" int FileIsDirectoryFd(int fd) {
    std::lock_guard lock(g_directoryMutex);
    return Directories().count(fd) != 0 ? 1 : 0;
}

extern "C" int FileDirectoryFdPath(int fd, char* out, unsigned size) {
    std::lock_guard lock(g_directoryMutex);
    const auto found = Directories().find(fd);
    if (found == Directories().end() || out == nullptr || size == 0) return 0;
    std::snprintf(out, size, "%s", found->second.path.c_str());
    return 1;
}

extern "C" void FileForgetDirectoryFd(int fd) {
    std::lock_guard lock(g_directoryMutex);
    Directories().erase(fd);
}

// FreeBSD 9 dirent as used by the console's getdents: { u32 d_fileno; u16 d_reclen; u8 d_type; u8 d_namlen; char d_name[]; }, 4-byte aligned.
extern "C" int FileReadDirectoryFd(int fd, char* buf, int nbytes) {
    std::lock_guard lock(g_directoryMutex);
    const auto found = Directories().find(fd);
    if (found == Directories().end()) return -2;
    auto& state = found->second;
    if (!state.loaded) {
        state.loaded = true;
        state.entries.emplace_back(".", static_cast<std::uint8_t>(4));
        state.entries.emplace_back("..", static_cast<std::uint8_t>(4));
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(state.path), error)) {
            std::error_code typeError;
            state.entries.emplace_back(entry.path().filename().string(), static_cast<std::uint8_t>(entry.is_directory(typeError) ? 4 : 8));
        }
    }
    std::size_t used = 0;
    while (state.cursor < state.entries.size()) {
        const auto& [name, type] = state.entries[state.cursor];
        const std::size_t reclen = (8 + name.size() + 1 + 3) & ~static_cast<std::size_t>(3);
        if (used + reclen > static_cast<std::size_t>(nbytes)) break;
        auto* out = reinterpret_cast<std::uint8_t*>(buf) + used;
        std::memset(out, 0, reclen);
        const std::uint32_t fileno = static_cast<std::uint32_t>(state.cursor + 1);
        const std::uint16_t reclen16 = static_cast<std::uint16_t>(reclen);
        std::memcpy(out, &fileno, sizeof(fileno));
        std::memcpy(out + 4, &reclen16, sizeof(reclen16));
        out[6] = type;
        out[7] = static_cast<std::uint8_t>(name.size());
        std::memcpy(out + 8, name.c_str(), name.size() + 1);
        used += reclen;
        ++state.cursor;
    }
    return static_cast<int>(used);
}
#else
extern "C" int FileOpenDirectoryFd(const char*) { return -1; }
extern "C" int FileIsDirectoryFd(int) { return 0; }
extern "C" int FileDirectoryFdPath(int, char*, unsigned) { return 0; }
extern "C" void FileForgetDirectoryFd(int) {}
extern "C" int FileReadDirectoryFd(int, char*, int) { return -2; }
#endif
