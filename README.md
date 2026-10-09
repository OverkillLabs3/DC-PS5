# DeadCells-PS5 Native

**Dead Cells for PS5, running as a fully native Windows program: relinked, not emulated.**

To our knowledge, the first PS5 game running as a fully native Windows program.

*Dead Cells: Return to Castlevania Edition* (PPSA15552) runs from **your own** copy of the PS5 game. The game's
original x86-64 code is relinked into a standalone Windows executable and runs directly: no emulator, no CPU emulation
and no separate runtime process. Its GPU shaders are recompiled to Vulkan and the console libraries it needs are
provided as ordinary DLLs.

Made possible by the [AnyPS5](https://github.com/boykopovar/AnyPS5) project.

* Dead Cells as a real native Windows program, playable from the logo through gameplay, with music and sound effects.
* About 55-60 FPS in indoor scenes and 30-35 FPS in outdoor scenes (see [Performance](#performance)).
* A download with nothing to install: extract, add your game files, double-click.

> This project does not contain the game, firmware, keys or any Sony software. You must supply your own, legally
> obtained, **decrypted** copy of the game.

> **Tested with:** *Dead Cells: Return to Castlevania Edition*, title ID `PPSA15552`, content ID
> `UP4016-PPSA15552_00-DEADCELLS0000000`, version `01.000.000` (master `01.01`). Other versions, regions, patches or
> repacked dumps have **not** been tested and may not work.

## Play

### Requirements

* Windows 10 or 11, 64-bit.
* A GPU and driver with Vulkan 1.1 or newer, supporting `VK_EXT_depth_clip_control`, BC (DXT) texture compression and
  buffer device address. Tested on an NVIDIA RTX 5070 Ti.
* 16 GB of RAM or more recommended.

### Setup

1. Download the latest DeadCells-PS5 Native release and extract it anywhere.
2. Copy your own game files **into the same folder as `deadcells.exe`**:
   * `eboot.elf` (the decrypted executable)
   * `sce_sys`, `sce_module`, `res.pak` and the rest of the game's files
3. Double-click `deadcells.exe`.

The first start takes a few seconds while the game is prepared from your files; later starts are immediate. There is no
console window. Problems are shown in a message box and details are written to the `logs` folder. On first start the
launcher also creates a `Dead Cells` shortcut and uses the icon from your own game files.

Windows SmartScreen may warn about an unrecognised app the first time; choose *More info*, then *Run anyway*.

### Controls

Keyboard and mouse work out of the box, and any gamepad supported by SDL (Xbox, DualShock, DualSense, Switch Pro, ...) can
be used at the same time. Keyboard, mouse and gamepad inputs are merged.

| PS5 button | Keyboard | Mouse | Gamepad (Xbox layout) | In Dead Cells |
|---|---|---|---|---|
| Cross | Enter / Space | | A | Jump, confirm |
| Circle | C | | B | Roll, back |
| Square | | **Left click** | X | Attack (primary weapon) |
| Triangle | I | | Y | Secondary weapon |
| L1 | Q | | LB | Health flask |
| R1 | E / Alt | | RB | |
| L2 | | | Left trigger | Left skill |
| R2 | | **Right click** | Right trigger | Right skill |
| L3 / R3 | Shift / Ctrl | | Stick clicks | |
| D-pad | Arrow keys | Mouse wheel (up / down) | D-pad | Menu navigation |
| Left stick | W A S D | | Left stick | Move |
| Right stick | T F G H | Mouse movement (mouse-aim mode) | Right stick | |
| Options | Esc | | Start | Pause |
| Touchpad | Backspace / Tab | | Back | |

Extras:

* **F11**: toggle fullscreen.
* **Middle mouse button**: turn mouse-aim mode on or off. While it is on, the mouse is captured and its movement drives
  the right stick; press the middle button again (or switch windows) to release it.

Rumble, light bar and motion controls are not implemented.

### Save data

Saves are stored in the `_sd` folder next to `deadcells.exe`. Copy it to back up your progress; delete it to reset.

### Performance

Indoor scenes run at about 55-60 FPS and outdoor scenes at 30-35 FPS. Overlay and capture software (OBS hook, ReShade, RivaTuner,
launcher overlays) can slow the game down; create an empty file named `no_vk_layers.txt` next to `deadcells.exe` to disable
implicit Vulkan layers.

### Optional switches

Empty files next to `deadcells.exe`, read at startup:

| File | Effect |
|---|---|
| `no_vk_layers.txt` | disable implicit Vulkan layers |
| `verbose_log.txt` | write periodic performance and audio statistics to the logs |
| `padspk_rate.txt` | controller-speaker sample rate (a single number, default 16000) |

### Troubleshooting

* *"The game files were not found"*: the files must be next to `deadcells.exe`, not in a subfolder.
* *"The game executable is not decrypted"*: `eboot.elf` must be the decrypted executable.
* *The game closes right after starting*: open `logs\game.err`; the last lines say what failed.
* *Very low FPS*: close overlay and capture software, or add `no_vk_layers.txt`.

## About the project

AnyPS5 is a tool for porting executables automatically. It includes a [relinker](core/relinker) that converts an
executable to the target system's native format and implementations of [system prx libraries](core/libs/prx) suitable
for dynamic linking.

### Status

Unsupported or unexpected states strictly throw `std::runtime_error`; `what()` is printed to stderr and the process
terminates. The [shader recompiler](core/shader/recompiler/Recompiler.cpp) produces SPIR-V validated with
[SPIRV-Tools](3rdparty/SPIRV-Tools).

[Technical debt of the project](docs/TechnicalDebt.md), [code style conventions](docs/CONVENTIONS.md)

## Building from source

The relinker uses only the C++20 standard library and should build with any conforming compiler.
[libc.prx](core/libs/prx/libc) contains compiler-specific code: Linux builds work with GCC; on Windows,
MinGW-w64 GCC 15.2.0 (`winlibs-gcc15`, `x86_64-ucrt-posix-seh`) is currently required.

```
git clone --recursive <repository-url>
cd <repository>
cmake -S . -B build -G Ninja
cmake --build build --target libs relinker deadcells -j 16
powershell -ExecutionPolicy Bypass -File tools\package_release.ps1
```

You need `g++` (MinGW-w64, see above), `cmake` and `ninja` on your `PATH`. The result is
`release\DeadCells-PS5-Native-<version>-win64.zip` containing `deadcells.exe`, the `libs` folder and `tools\relinker.exe`, with no
extra dependencies. To sign `deadcells.exe` during packaging, see the comments in `tools\package_release.ps1`.

## Legal

Dead Cells is a trademark and copyright of its respective owners; this project is not affiliated with or endorsed by
them. This project is intended for interoperability, research, preservation, and compatibility purposes. It does not
include, distribute, or require copyrighted software, firmware, cryptographic keys, or proprietary libraries. Users are
responsible for ensuring that any binaries used with this project are obtained and used in accordance with applicable
laws and their respective license terms.

## License

This project is licensed under the GNU General Public License version 2 only.
