# Android Build Guide

## Prerequisites

1. **Android Studio** (for SDK, NDK, and JBR)
2. **Android SDK** (API 29+)
3. **Android NDK** (r25+ recommended)
4. **ADB** in PATH for deployment

## Build Steps

### 1. Clone with submodules
```bash
git clone --recursive <repo-url>
# Or if already cloned:
git submodule update --init --recursive
```

### 2. Copy assets to Android folder
Assets must be copied from `Assets/` to `android/app/src/main/assets/`:
```bash
# From repo root:
cp -r Assets/compiledassets android/app/src/main/assets/
cp -r Assets/Scenes android/app/src/main/assets/scenes  # Note: lowercase 'scenes'
cp -r Assets/scripts android/app/src/main/assets/
cp -r Assets/behaviourtrees android/app/src/main/assets/
cp -r Assets/prefabs android/app/src/main/assets/
cp -r Assets/navmeshdata android/app/src/main/assets/
cp -r Assets/Fonts android/app/src/main/assets/
cp -r Assets/sounds android/app/src/main/assets/
```

**Important**: Android's AAssetManager is case-sensitive. Scene folder must be lowercase `scenes/`.

### 3. Update asset_manifest.txt
The `android/app/src/main/assets/asset_manifest.txt` must list all asset files.
Use lowercase paths for scene files (e.g., `scenes/mainmenu.scene`).

### 4. Build APK
```bash
cd android

# Set JAVA_HOME if not set (use Android Studio's bundled JBR)
export JAVA_HOME="C:/Program Files/Android/Android Studio/jbr"

# Build debug APK
./gradlew assembleDebug
```

APK output: `android/app/build/outputs/apk/debug/app-debug.apk`

### 5. Install and run
```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n com.ryengine.vulkan/com.magicengine.kurorekishi.MainActivity
```

## Known Issues

### Case sensitivity
Android filesystem is case-sensitive. The VFS normalizes paths to lowercase on Windows but preserves case on Android. Ensure:
- Scene folder is `scenes/` (lowercase)
- `openscenes.json` is lowercase (APK builder may lowercase it)

### ETC2 textures
Mobile textures should be compiled to ETC2 format for Mali GPUs. Desktop textures (BC7) won't work on Android.

### Validation layers
The APK includes `VkLayer_khronos_validation.json`. To see validation errors:
```bash
adb logcat | grep -i validation
```

## Logs
```bash
# All engine logs
adb logcat | grep -i "ryEngine\|MagicEngine\|VFS"

# Touch input
adb logcat | grep -i "TOUCH"
```
