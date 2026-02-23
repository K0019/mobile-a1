# Android Tech Stack & Integration Overview

This document summarizes the tech stack and integration model used to run the MagicEngine game engine on Android, using the existing multi‑platform C++ engine and the Android project under `android/`.

---

## 1. High-Level Architecture

The Android build is a thin Java/Gradle wrapper around a cross‑platform C++ engine:

- **Android app layer**
  - Java `NativeActivity` (`MainActivity`) packaged by **Gradle / Android Studio**.
  - Uses the **Android NDK** to load a native shared library `libMagicEngineAndroid.so`.
- **Native C++ engine layer**
  - The same **MagicEngine** C++ codebase used on desktop (Vulkan renderer, physics, scripting, etc.).
  - Built via **CMake** inside the Android Gradle project.
  - Exposed to Android through `android_main.cpp` (NDK `android_native_app_glue` entry).

Android is effectively “just another platform” for the engine: Java/Android code provides lifecycle, window, sensors, and packaging; the C++ engine implements rendering, audio, physics, and gameplay logic.

---

## 2. Android Project & Build Tooling

### 2.1 Gradle / Android Studio Project

Location: `android/`

- Standard Android/Gradle project:
  - `android/app/` contains `build.gradle`, `src/main/java`, `src/main/cpp`, `AndroidManifest.xml`, etc.
  - The project can be opened directly in **Android Studio**.

Responsibilities of the Android/Gradle layer:

- Select **ABI**s (e.g., `arm64-v8a`), SDK version, and NDK.
- Invoke **CMake** to build the native C++ library (`MagicEngineAndroid`).
- Package:
  - Native `.so` libraries: engine (`libMagicEngineAndroid.so`), FMOD, optional Vulkan validation layer.
  - Game assets under `app/src/main/assets`.
- Produce and sign debug/release APKs (`assembleDebug`, `assembleRelease`).

Relevant scripts:

- `scripts/build_android.ps1` (root): one‑command Android pipeline:
  - Asset manifest generation.
  - ASTC texture recompression (via `AssetCompiler.exe`).
  - Gradle build + install (`installDebug` / `installRelease`).
- `android/build_android.ps1` and `android/build_android.sh`:
  - Copy assets into the Android project.
  - Generate `asset_manifest.txt`.
  - Run `gradlew assembleDebug`.

Supporting docs:

- `android/BUILD.md` and the Android section of the root `README.md` describe:
  - Prerequisites (Android Studio, SDK, NDK, Python, ADB).
  - How to run the build scripts.
  - Debugging tips (e.g., `adb logcat` filters).

---

## 3. Native C++ Build Stack (NDK + CMake)

### 3.1 Android CMake Configuration

File: `android/app/src/main/cpp/CMakeLists.txt`

Responsibilities:

- Define the Android native project and library:

  - `project("MagicEngineAndroid")`
  - Builds `libMagicEngineAndroid.so` with:
    - `android_main.cpp` (platform entry).
    - The entire engine via:

      ```cmake
      get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../" ABSOLUTE)
      add_subdirectory("${PROJECT_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/magicengine-build")
      ```

- Set the C++ standard for Android builds:
