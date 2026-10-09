DEADCELLS-PS5 NATIVE
====================

Dead Cells for PS5, running as a fully native Windows program: relinked, not
emulated.

To our knowledge, the first PS5 game running as a fully native Windows
program.

Dead Cells: Return to Castlevania Edition (PPSA15552) runs from your own copy
of the PS5 game. The game's original code is relinked into a standalone Windows
executable and runs directly: no emulator, no CPU emulation and no separate
runtime process. Nothing needs to be installed.

This package does NOT contain the game, firmware, keys or any Sony software.
You need your own, legally obtained, DECRYPTED copy of the game.

TESTED WITH: Dead Cells: Return to Castlevania Edition
  Title ID    PPSA15552
  Content ID  UP4016-PPSA15552_00-DEADCELLS0000000
  Version     01.000.000 (master 01.01)
Other versions, regions, patches or repacked dumps have NOT been tested and may
not work.


HOW TO PLAY
-----------
1. Keep this folder as it is (deadcells.exe, libs, tools).
2. Copy your own game files INTO THIS FOLDER, next to deadcells.exe:
     eboot.elf   (the decrypted executable)
     sce_sys
     sce_module
     res.pak
     ... and the rest of the game's files
3. Double-click deadcells.exe.

The first start takes a few seconds while the game is prepared from your files.
Later starts are immediate. There is no console window. If something goes wrong
a message box explains it, and details are written to the "logs" folder.

On the first start a "Dead Cells" shortcut is created and the icon is taken from
your own game files.

Windows SmartScreen may warn about an unrecognised app the first time. Choose
"More info", then "Run anyway".


REQUIREMENTS
------------
- Windows 10 or 11, 64-bit
- A GPU and driver with Vulkan 1.1 or newer, supporting VK_EXT_depth_clip_control,
  BC (DXT) texture compression and buffer device address
- 16 GB of RAM or more recommended


CONTROLS
--------
Keyboard and mouse work out of the box, and any gamepad supported by SDL (Xbox,
DualShock, DualSense, Switch Pro, ...) can be used at the same time. Keyboard,
mouse and gamepad inputs are merged.

  PS5 button   Keyboard         Mouse                  Gamepad (Xbox)  In Dead Cells
  ----------   --------         -----                  --------------  -------------
  Cross        Enter / Space                            A               Jump, confirm
  Circle       C                                        B               Roll, back
  Square                        LEFT CLICK              X               Attack (primary weapon)
  Triangle     I                                        Y               Secondary weapon
  L1           Q                                        LB              Health flask
  R1           E / Alt                                  RB
  L2                                                    Left trigger    Left skill
  R2                            RIGHT CLICK             Right trigger   Right skill
  L3 / R3      Shift / Ctrl                             Stick clicks
  D-pad        Arrow keys       Wheel (up / down)       D-pad           Menu navigation
  Left stick   W A S D                                  Left stick      Move
  Right stick  T F G H          Mouse movement (aim)    Right stick
  Options      Esc                                      Start           Pause
  Touchpad     Backspace / Tab                          Back

  F11                 Toggle fullscreen
  Middle mouse button Turn mouse-aim mode on/off. While on, the mouse is captured
                      and its movement drives the right stick; press the middle
                      button again (or switch windows) to release it.

Rumble, light bar and motion controls are not implemented.


SAVE DATA
---------
Saves are stored in the "_sd" folder next to deadcells.exe. Copy it to back up
your progress; delete it to reset.


PERFORMANCE
-----------
About 55-60 FPS in indoor scenes and 30-35 FPS in outdoor scenes. Overlay and capture software
(OBS hook, ReShade, RivaTuner, launcher overlays) can slow the game down: create
an empty file named no_vk_layers.txt next to deadcells.exe to disable implicit
Vulkan layers.


OPTIONAL SWITCHES (empty files next to deadcells.exe)
-----------------------------------------------------
  no_vk_layers.txt   disable implicit Vulkan layers
  verbose_log.txt    write periodic performance and audio statistics to logs
  padspk_rate.txt    controller-speaker sample rate (a single number, default 16000)


TROUBLESHOOTING
---------------
"The game files were not found"
    The files must be next to deadcells.exe, not in a subfolder.
"The game executable is not decrypted"
    eboot.elf must be the decrypted executable.
The game closes right after starting
    Open logs\game.err; the last lines say what failed.
Very low FPS
    Close overlay and capture software, or add no_vk_layers.txt.


LEGAL
-----
Dead Cells is a trademark and copyright of its respective owners. This project is
not affiliated with or endorsed by them. It is intended for interoperability,
research, preservation and compatibility purposes and does not include,
distribute or require copyrighted software, firmware, cryptographic keys or
proprietary libraries. You are responsible for ensuring that the files you use
are obtained and used in accordance with applicable laws and license terms.

Made possible by the AnyPS5 project: https://github.com/boykopovar/AnyPS5
Licensed under the GNU General Public License version 2 only (see LICENSE).
The complete source code is available from the project's GitHub page.
