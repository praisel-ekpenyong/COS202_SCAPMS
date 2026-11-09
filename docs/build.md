# Building SCAPMS

Toolchain: **MSYS2 UCRT64** at `C:\msys64`. Everything is driven
from ordinary **PowerShell** in the repo root - no MSYS2 shell required - by putting
`C:\msys64\ucrt64\bin` on `PATH` for the invocation. That one directory holds g++, cmake,
ninja, the Qt DLLs, and the Qt CMake packages; without it, CMake cannot find Ninja or the
compiler, and the app cannot find its Qt DLLs.

## Installed versions (verified 2026-10-17)

| Tool | Version | pacman package |
|---|---|---|
| g++ | 16.1.0 (Rev5, MSYS2) | mingw-w64-ucrt-x86_64-gcc (pre-installed) |
| CMake | 4.4.0 | mingw-w64-ucrt-x86_64-cmake |
| Ninja | 1.13.2 | mingw-w64-ucrt-x86_64-ninja |
| Qt Widgets | 6.11.1 | mingw-w64-ucrt-x86_64-qt6-base |
| Qt moc/uic/rcc | 6.11.1 | mingw-w64-ucrt-x86_64-qt6-tools (**required** - not in qt6-base) |

To (re)install everything into a fresh MSYS2:

```
C:\msys64\usr\bin\pacman.exe -S --noconfirm mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-tools mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

## Exact commands (fresh shell, repo root)

Configure (only needed once, or after CMakeLists changes):

```
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"; cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

Build:

```
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"; cmake --build build
```

Run the app (must have ucrt64 on PATH at run time too, for the Qt DLLs):

```
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"; .\build\gui\SCAPMS.exe
```

Clean rebuild: delete `build\` and re-run configure + build.

## Layout

- `build/gui/SCAPMS.exe` - the application (GUI subsystem; no console window).
- Targets: static libs `scapms_utils`, `scapms_model`, `scapms_controller`,
  `scapms_gui`, executable `SCAPMS`.
- Namespaces mirror the directories: `scapms::{model,controller,gui,utils}`.
- Each library's `CMakeLists.txt` adds the **repo root** to its include path, so always
  include as `#include "gui/MainWindow.h"`, `#include "utils/AppInfo.h"`, etc.
- New GUI classes with `Q_OBJECT` headers need no extra wiring - `CMAKE_AUTOMOC` is on.

## Common build issues

1. **PATH**: invoking `C:\msys64\ucrt64\bin\cmake.exe` directly *without* the PATH prefix
   still fails - CMake then cannot locate `ninja` or `g++`. Always set the PATH prefix.
2. **qt6-tools**: MSYS2 splits Qt tools out of `qt6-base`. Without `qt6-tools`, configure
   fails at AUTOMOC because `moc.exe` is absent (it installs to
   `C:\msys64\ucrt64\share\qt6\bin\moc.exe`; the Qt6 CMake config finds it there by
   itself once the package is present).
3. `.exe` files are gitignored; source `.cpp/.h` and CMakeLists are what get committed,
   never build output.
