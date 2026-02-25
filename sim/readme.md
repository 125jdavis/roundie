# roundie – PC Simulator

A CMake-based LVGL9 simulator for the roundie display, built on SDL2.

## Prerequisites

| Tool | Notes |
|------|-------|
| CMake ≥ 3.21 | |
| C/C++ compiler | MSVC (Visual Studio 2022) on Windows; GCC/Clang on Linux/macOS |
| SDL2 | See platform notes below |

## Building on Windows (Visual Studio + vcpkg)

1. **Install vcpkg** (if not already done):
   ```
   git clone https://github.com/microsoft/vcpkg C:\dev\vcpkg
   C:\dev\vcpkg\bootstrap-vcpkg.bat
   ```

2. **Install SDL2** for the x64 triplet:
   ```
   C:\dev\vcpkg\vcpkg install sdl2:x64-windows
   ```

3. **Configure** the simulator build:
   ```
   cmake -S sim -B build/sim ^
         -G "Visual Studio 17 2022" -A x64 ^
         -DCMAKE_TOOLCHAIN_FILE=C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake ^
         -DVCPKG_TARGET_TRIPLET=x64-windows
   ```

4. **Build**:
   ```
   cmake --build build/sim --config Release --target roundie_sim
   ```

> **Note:** `-DVCPKG_TARGET_TRIPLET=x64-windows` is required when you pass
> `-A x64` so CMake picks up the 64-bit SDL2 package from vcpkg.

> **Note:** On Windows/MSVC, LVGL is compiled as a **static library** to avoid
> the `cmake -E __create_def` PreLink failure (MSB3073) that occurs when CMake
> auto-generates DLL export tables.  No extra steps are needed; CMake handles
> this automatically.

## Building on Linux / macOS

Install SDL2 via your package manager, then:

```bash
# Ubuntu / Debian
sudo apt install libsdl2-dev

# macOS (Homebrew)
brew install sdl2
```

```bash
cmake -S sim -B build/sim
cmake --build build/sim --target roundie_sim
```

## No SDL2 installed?

If SDL2 is not found on the system, CMake will automatically download and
build it from source via FetchContent (requires internet access at configure
time).  No extra steps are needed, but the first configure will take longer.
