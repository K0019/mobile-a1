# MagicEngine

## Prerequisites

| Tool | Notes |
|------|-------|
| **CMake 3.22+** | If vcpkg extraction fails on international Windows, try CMake 3.30.5 |
| **Visual Studio 2022** (v17) | With C++ desktop development workload |
| **.NET Framework 4.7.2** | For the C# editor project |
| **Python 3** | On `PATH` (used by asset pipeline scripts) |
| **Latest GPU drivers** | Required for Vulkan renderer |

For Android builds, also install:

| Tool | Notes |
|------|-------|
| **Android Studio** | Provides SDK, NDK, and bundled Java (JBR) |
| **Android SDK** API 30+ | Via Android Studio SDK Manager |
| **Android NDK** r29+ | Via Android Studio SDK Manager |

## Quick Start (Desktop)

```bat
:: 1. Clone with submodules
git clone --recursive <repo-url>
cd gam300-magic

:: 2. Run setup (initializes submodules, bootstraps vcpkg, generates VS solution)
setup.bat

:: 3. Open and build
::    The solution is in the build/ folder.
::    Debug/x64   = Editor + Engine
::    Release/x64 = Game only
```

If you already cloned without `--recursive`:
```bat
scripts\setup_submodules.bat
setup.bat
```

## Quick Start (Android)

### Prerequisites

Android builds require a **desktop build first** to create the AssetCompiler tool,
which converts textures to mobile-compatible ASTC format.

### From Fresh Clone

```bat
:: 1. Clone with submodules
git clone --recursive <repo-url>
cd gam300-magic

:: 2. Build AssetCompiler (desktop CMake build)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_ASSET_COMPILER=ON ^
  -DCMAKE_TOOLCHAIN_FILE="../extern/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build . --target AssetCompiler --config Release
cd ..

:: 3. Build Android (Gradle handles ASTC conversion + manifest automatically)
cd android
gradlew assembleDebug
```

### Using Android Studio

Once AssetCompiler is built, you can open `android/` in Android Studio and build
directly. Gradle automatically runs:
1. **generateInitialManifest** - Creates asset manifest
2. **recompressAstcTextures** - Converts textures to ASTC (if AssetCompiler exists)
3. **generateAssetManifest** - Updates manifest with android textures

If AssetCompiler doesn't exist, ASTC conversion is skipped and textures will fail
to load on mobile GPUs (they use BC7 format which isn't supported).

### Command-Line Build

```powershell
# One command does everything (after AssetCompiler is built):
powershell -ExecutionPolicy Bypass -File scripts\build_android.ps1
```

This auto-detects Java and the Android SDK, generates `local.properties` if
missing, runs the asset pipeline, builds the APK, and installs it on a connected device.

### Build Options

```powershell
# Skip asset steps (code-only change):
scripts\build_android.ps1 -InstallOnly

# Skip ASTC recompression (textures unchanged):
scripts\build_android.ps1 -SkipAssetCompile

# Release build:
scripts\build_android.ps1 -BuildType Release
```

### Interactive Build Menu

`setup.bat` also has an interactive menu for both Windows and Android targets:
```bat
setup.bat              :: interactive menu
setup.bat windows debug --no-menu
setup.bat android debug --no-menu
```

## Project Structure

```
gam300-magic/
  CMakeLists.txt        Main build configuration
  setup.bat             Interactive build script (Windows + Android)
  vcpkg.json            Desktop dependencies (assimp, freeimage, ktx, etc.)
  MagicEngine/          Core engine library
  AssetCompiler/        Asset compilation tool
  editor/               Desktop editor application
  game/                 Standalone game application
  Assets/               Game assets (scenes, models, textures, scripts)
  extern/               Git submodules (26 libraries)
  scripts/
    setup_submodules.bat   Submodule initialization + verification
    build_android.ps1      One-command Android build pipeline
  android/
    BUILD.md               Detailed Android build documentation
```

## Known Issues

- Newer CMake versions may fail at the vcpkg extraction step on international
  versions of Windows. Use CMake 3.30.5 if this occurs.
- Programmers may need to update their graphics driver to the latest version
  due to Vulkan renderer API usage.
