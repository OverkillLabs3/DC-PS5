#ifndef CORE_LIBS_PRX_LIBSCEVIDEOOUT_PADINPUT_HPP
#define CORE_LIBS_PRX_LIBSCEVIDEOOUT_PADINPUT_HPP

#include "SDL_events.h"
#include "SDL_gamecontroller.h"
#include "prx/libScePad/include/InputMapping.hpp"
#include "prx/libScePad/include/PadState.hpp"
#include <array>
#include <chrono>

class DisplayWindow;

class PadInput {
public:
    void HandleEvent(const SDL_Event& event, DisplayWindow& window);
    void Update();

private:
    void publish();
    void setMouseMode(bool enabled);
    void openController();
    void closeController();
    PadInputState sampleController() const;
    std::array<bool, Pad::InputMapping.size()> pressed{};
    std::array<std::chrono::steady_clock::time_point, Pad::InputMapping.size()> wheelReleaseTimes{};
    std::array<std::uint8_t, 2> mouseStick{128, 128};
    std::chrono::steady_clock::time_point nextMousePoll{};
    bool mouseEnabled = false;
    // Host gamepad (Xbox / DualShock / DualSense / Switch Pro ... through SDL's GameController mapping database), merged with keyboard and mouse.
    SDL_GameController* controller = nullptr;
    bool controllerProbed = false;
    PadInputState controllerState{};
};

#endif
