#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "SDL.h"
#include "prx/libSceVideoOut/include/PadInput.hpp"
#include "prx/libSceVideoOut/include/DisplayWindow.hpp"
#include "prx/libScePad/include/PadState.hpp"
#include "prx/libScePad/include/PadInputTypes.hpp"

void PadInput::setMouseMode(bool enabled) {
    if (SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE) != 0) throw std::runtime_error(std::string("Pad: relative mouse mode failed: ") + SDL_GetError());
    int deltaX = 0;
    int deltaY = 0;
    SDL_GetRelativeMouseState(&deltaX, &deltaY);
    mouseEnabled = enabled;
    mouseStick = {128, 128};
    nextMousePoll = std::chrono::steady_clock::now() + std::chrono::milliseconds(Pad::MousePollIntervalMs);
}

void PadInput::openController() {
    if (controller != nullptr) return;
    if ((SDL_WasInit(SDL_INIT_GAMECONTROLLER) & SDL_INIT_GAMECONTROLLER) == 0) {
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1"); // keep reading the pad while the window is unfocused
        if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
            std::fprintf(stderr, "[PAD] SDL game controller init failed: %s\n", SDL_GetError());
            return; // no controller support: keyboard and mouse keep working
        }
    }
    std::fprintf(stderr, "[PAD] SDL reports %d joystick device(s)\n", SDL_NumJoysticks());
    for (int index = 0; index < SDL_NumJoysticks(); ++index) {
        const char* joystickName = SDL_JoystickNameForIndex(index);
        std::fprintf(stderr, "[PAD]   device %d: %s (game controller mapping: %s)\n", index, joystickName != nullptr ? joystickName : "?", SDL_IsGameController(index) == SDL_TRUE ? "yes" : "NO");
        if (SDL_IsGameController(index) == SDL_FALSE) continue;
        controller = SDL_GameControllerOpen(index);
        if (controller != nullptr) {
            std::fprintf(stderr, "[PAD]   opened %s\n", SDL_GameControllerName(controller));
            break;
        }
        std::fprintf(stderr, "[PAD]   open failed: %s\n", SDL_GetError());
    }
}

void PadInput::closeController() {
    if (controller != nullptr) SDL_GameControllerClose(controller);
    controller = nullptr;
    controllerState = {};
}

// SDL reports buttons by physical position (A = bottom face button), which on a PlayStation layout is Cross, so the mapping below is the
// PlayStation one for every pad type; a Switch-style layout is normalised by SDL's own mapping database.
PadInputState PadInput::sampleController() const {
    PadInputState result;
    if (controller == nullptr) return result;
    const auto button = [&](SDL_GameControllerButton which, Pad::PadButton pad) {
        if (SDL_GameControllerGetButton(controller, which) != 0) result.buttons |= static_cast<std::uint32_t>(pad);
    };
    button(SDL_CONTROLLER_BUTTON_A, Pad::PadButton::Cross);
    button(SDL_CONTROLLER_BUTTON_B, Pad::PadButton::Circle);
    button(SDL_CONTROLLER_BUTTON_X, Pad::PadButton::Square);
    button(SDL_CONTROLLER_BUTTON_Y, Pad::PadButton::Triangle);
    button(SDL_CONTROLLER_BUTTON_START, Pad::PadButton::Options);
    button(SDL_CONTROLLER_BUTTON_BACK, Pad::PadButton::TouchPad);
    button(SDL_CONTROLLER_BUTTON_TOUCHPAD, Pad::PadButton::TouchPad);
    button(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, Pad::PadButton::L1);
    button(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, Pad::PadButton::R1);
    button(SDL_CONTROLLER_BUTTON_LEFTSTICK, Pad::PadButton::L3);
    button(SDL_CONTROLLER_BUTTON_RIGHTSTICK, Pad::PadButton::R3);
    button(SDL_CONTROLLER_BUTTON_DPAD_UP, Pad::PadButton::Up);
    button(SDL_CONTROLLER_BUTTON_DPAD_DOWN, Pad::PadButton::Down);
    button(SDL_CONTROLLER_BUTTON_DPAD_LEFT, Pad::PadButton::Left);
    button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, Pad::PadButton::Right);
    constexpr int triggerThreshold = 8000;
    if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > triggerThreshold) result.buttons |= static_cast<std::uint32_t>(Pad::PadButton::L2);
    if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > triggerThreshold) result.buttons |= static_cast<std::uint32_t>(Pad::PadButton::R2);
    const auto axis = [&](SDL_GameControllerAxis which) {
        constexpr int deadZone = 4500; // ~14 %
        int value = SDL_GameControllerGetAxis(controller, which);
        if (value > -deadZone && value < deadZone) value = 0;
        return static_cast<std::uint8_t>(std::clamp((value >> 8) + 128, 0, 255));
    };
    result.sticks = {axis(SDL_CONTROLLER_AXIS_LEFTX), axis(SDL_CONTROLLER_AXIS_LEFTY), axis(SDL_CONTROLLER_AXIS_RIGHTX), axis(SDL_CONTROLLER_AXIS_RIGHTY)};
    return result;
}

void PadInput::HandleEvent(const SDL_Event& event, DisplayWindow& window) {
    if (event.type == SDL_CONTROLLERDEVICEADDED) {
        openController();
        return;
    }
    if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        if (controller != nullptr && event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller))) {
            closeController();
            openController(); // fall back to another connected pad, if any
            publish();
        }
        return;
    }
    if (event.type == SDL_WINDOWEVENT && (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST || event.window.event == SDL_WINDOWEVENT_CLOSE)) {
        pressed.fill(false);
        wheelReleaseTimes.fill({});
        if (mouseEnabled) setMouseMode(false);
        publish();
        return;
    }
    if (event.type == SDL_MOUSEWHEEL) {
        int direction = (event.wheel.y > 0) - (event.wheel.y < 0);
        if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) direction = -direction;
        if (direction == 0) return;
        const auto releaseTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(Pad::WheelPressDurationMs);
        for (std::size_t index = 0; index < Pad::InputMapping.size(); ++index) {
            const auto& binding = Pad::InputMapping[index];
            if (binding.wheelDirection == 0) continue;
            pressed[index] = binding.wheelDirection == direction;
            wheelReleaseTimes[index] = pressed[index] ? releaseTime : std::chrono::steady_clock::time_point{};
        }
        publish();
        return;
    }
    const bool keyboard = event.type == SDL_KEYDOWN || event.type == SDL_KEYUP;
    const bool mouse = event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP;
    if (!keyboard && !mouse) return;
    if (keyboard && event.key.repeat != 0) return;
    const bool down = event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN;
    for (std::size_t index = 0; index < Pad::InputMapping.size(); ++index) {
        const auto& binding = Pad::InputMapping[index];
        const bool keyMatches = binding.key != SDL_SCANCODE_UNKNOWN && binding.key == event.key.keysym.scancode;
        const bool mouseMatches = binding.mouseButton != Pad::MouseButton::None && binding.mouseButton == static_cast<Pad::MouseButton>(event.button.button);
        const bool matches = keyboard ? keyMatches : mouseMatches;

        if (!matches) continue;
        if (binding.control == Pad::InputControl::ToggleFullscreen) {
            if (keyboard && down && !pressed[index] && window.Handle() != nullptr && event.key.windowID == SDL_GetWindowID(window.Handle())) window.ToggleFullscreen();
        }
        if (binding.control == Pad::InputControl::ToggleMouse && down && !pressed[index]) setMouseMode(!mouseEnabled);
        pressed[index] = down;
    }
    publish();
}

void PadInput::Update() {
    if (!controllerProbed) {
        controllerProbed = true;
        openController();
    }
    if (controller != nullptr) {
        const auto sampled = sampleController();
        if (sampled.buttons != controllerState.buttons || sampled.sticks != controllerState.sticks) {
            controllerState = sampled;
            publish();
        }
    }
    const auto now = std::chrono::steady_clock::now();
    bool released = false;
    for (std::size_t index = 0; index < Pad::InputMapping.size(); ++index) {
        if (Pad::InputMapping[index].wheelDirection == 0 || !pressed[index] || now < wheelReleaseTimes[index]) continue;
        pressed[index] = false;
        wheelReleaseTimes[index] = {};
        released = true;
    }
    if (released) publish();
    if (!mouseEnabled) return;
    if (SDL_GetKeyboardFocus() == nullptr) {
        pressed.fill(false);
        wheelReleaseTimes.fill({});
        setMouseMode(false);
        publish();
        return;
    }
    if (now < nextMousePoll) return;
    nextMousePoll = now + std::chrono::milliseconds(Pad::MousePollIntervalMs);
    int deltaX = 0;
    int deltaY = 0;
    SDL_GetRelativeMouseState(&deltaX, &deltaY);
    mouseStick = {128, 128};
    if (deltaX != 0 || deltaY != 0) {
        const double distance = std::hypot(deltaX, deltaY);
        const double scale = std::clamp(distance * Pad::MouseSensitivity + 16.0, 64.0, 128.0) / distance;
        const auto mapAxis = [scale](int delta) { return static_cast<std::uint8_t>(std::clamp(128L + std::lround(delta * scale), 0L, 255L)); };
        mouseStick = {mapAxis(deltaX), mapAxis(deltaY)};
    }
    publish();
}

void PadInput::publish() {
    PadInputState state;
    std::array<bool, 4> negative{};
    std::array<bool, 4> positive{};
    for (std::size_t index = 0; index < Pad::InputMapping.size(); ++index) {
        if (!pressed[index]) continue;
        const auto& binding = Pad::InputMapping[index];
        switch (binding.control) {
            case Pad::InputControl::Button: state.buttons |= static_cast<std::uint32_t>(binding.button); break;
            case Pad::InputControl::LeftStickLeft: negative[0] = true; break;
            case Pad::InputControl::LeftStickRight: positive[0] = true; break;
            case Pad::InputControl::LeftStickUp: negative[1] = true; break;
            case Pad::InputControl::LeftStickDown: positive[1] = true; break;
            case Pad::InputControl::RightStickLeft: negative[2] = true; break;
            case Pad::InputControl::RightStickRight: positive[2] = true; break;
            case Pad::InputControl::RightStickUp: negative[3] = true; break;
            case Pad::InputControl::RightStickDown: positive[3] = true; break;
            case Pad::InputControl::TouchLeft: state.touchLeft = true; break;
            case Pad::InputControl::TouchRight: state.touchRight = true; break;
            case Pad::InputControl::ToggleMouse: break;
            case Pad::InputControl::ToggleFullscreen: break;
        }
    }
    for (std::size_t axis = 0; axis < state.sticks.size(); ++axis) {
        state.sticks[axis] = negative[axis] == positive[axis] ? 128 : negative[axis] ? 0 : 255;
    }
    if (mouseEnabled) {
        state.sticks[2] = mouseStick[0];
        state.sticks[3] = mouseStick[1];
    }
    // Gamepad merges with keyboard and mouse: buttons OR together, a deflected stick overrides the keyboard-derived axis.
    state.buttons |= controllerState.buttons;
    for (std::size_t axis = 0; axis < state.sticks.size(); ++axis) {
        if (controllerState.sticks[axis] != 128) state.sticks[axis] = controllerState.sticks[axis];
    }
    PadPublishInput_nid_postfix(state);
}
