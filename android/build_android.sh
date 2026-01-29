#!/bin/bash
# Android Build Script for MagicEngine
# Usage: ./build_android.sh [--install] [--clean]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
ANDROID_ASSETS="$SCRIPT_DIR/app/src/main/assets"
ASSETS_SOURCE="$REPO_ROOT/Assets"

INSTALL=false
CLEAN=false

for arg in "$@"; do
    case $arg in
        --install|-i) INSTALL=true ;;
        --clean|-c) CLEAN=true ;;
    esac
done

echo "=== MagicEngine Android Build ==="

# Check for JAVA_HOME
if [ -z "$JAVA_HOME" ]; then
    if [ -d "/c/Program Files/Android/Android Studio/jbr" ]; then
        export JAVA_HOME="/c/Program Files/Android/Android Studio/jbr"
        echo "Using Android Studio JBR: $JAVA_HOME"
    elif [ -d "$HOME/Android/android-studio/jbr" ]; then
        export JAVA_HOME="$HOME/Android/android-studio/jbr"
        echo "Using Android Studio JBR: $JAVA_HOME"
    else
        echo "Warning: JAVA_HOME not set, hoping gradlew finds Java..."
    fi
fi

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo -e "\n[1/4] Cleaning previous build..."
    cd "$SCRIPT_DIR" && ./gradlew clean

    for dir in Fonts behaviourtrees compiledassets images models navmeshdata prefabs scenes scripts sounds; do
        if [ -d "$ANDROID_ASSETS/$dir" ]; then
            rm -rf "$ANDROID_ASSETS/$dir"
            echo "  Removed $dir"
        fi
    done
else
    echo -e "\n[1/4] Skipping clean (use --clean to force)"
fi

# Copy assets
echo -e "\n[2/4] Copying assets..."

copy_assets() {
    local src="$1"
    local dst="$2"
    if [ -d "$ASSETS_SOURCE/$src" ]; then
        mkdir -p "$ANDROID_ASSETS/$dst"
        cp -r "$ASSETS_SOURCE/$src/"* "$ANDROID_ASSETS/$dst/"
        local count=$(find "$ANDROID_ASSETS/$dst" -type f | wc -l)
        echo "  $src -> $dst ($count files)"
    else
        echo "  $src not found, skipping"
    fi
}

copy_assets "compiledassets" "compiledassets"
copy_assets "Scenes" "scenes"  # Note: lowercase for Android case-sensitivity
copy_assets "scripts" "scripts"
copy_assets "behaviourtrees" "behaviourtrees"
copy_assets "prefabs" "prefabs"
copy_assets "navmeshdata" "navmeshdata"
copy_assets "Fonts" "Fonts"
copy_assets "sounds" "sounds"
copy_assets "images" "images"

# Generate asset manifest
echo -e "\n[3/4] Generating asset_manifest.txt..."

MANIFEST="$ANDROID_ASSETS/asset_manifest.txt"
find "$ANDROID_ASSETS" -type f \
    ! -name "asset_manifest.txt" \
    ! -name "VkLayer_khronos_validation.json" \
    -printf '%P\n' | sort > "$MANIFEST"

ENTRY_COUNT=$(wc -l < "$MANIFEST")
echo "  Generated manifest with $ENTRY_COUNT entries"

# Build APK
echo -e "\n[4/4] Building APK..."

cd "$SCRIPT_DIR"
./gradlew assembleDebug

APK_PATH="$SCRIPT_DIR/app/build/outputs/apk/debug/app-debug.apk"
if [ -f "$APK_PATH" ]; then
    APK_SIZE=$(du -h "$APK_PATH" | cut -f1)
    echo -e "\nBuild successful! APK: $APK_SIZE"
    echo "  $APK_PATH"
else
    echo "Error: APK not found"
    exit 1
fi

# Install if requested
if [ "$INSTALL" = true ]; then
    echo -e "\nInstalling to device..."
    adb install -r "$APK_PATH"
    if [ $? -eq 0 ]; then
        echo "Launching app..."
        adb shell am start -n com.ryengine.vulkan/com.magicengine.kurorekishi.MainActivity
    fi
fi

echo -e "\n=== Done ==="
