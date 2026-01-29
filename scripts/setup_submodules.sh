#!/bin/bash
set -e

echo "================================================================================"
echo "MagicEngine Submodule Setup Script"
echo "================================================================================"
echo

# Navigate to repo root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

# Check if this is a git repository
if [ ! -d ".git" ]; then
    echo "ERROR: This script must be run from within the MagicEngine repository."
    echo "Please navigate to the repository root and try again."
    exit 1
fi

echo "Step 1: Synchronizing submodule URLs..."
git submodule sync --recursive
echo "Done."
echo

echo "Step 2: Initializing and updating submodules..."
echo "This may take several minutes on first run..."
echo
git submodule update --init --recursive
echo "Done."
echo

echo "Step 3: Verifying critical submodules..."
MISSING=0
SUBMODULES="Vulkan-Headers volk VulkanMemoryAllocator glm fmt glslang imgui glfw"

for submod in $SUBMODULES; do
    if [ ! -f "extern/$submod/CMakeLists.txt" ]; then
        echo "MISSING: extern/$submod"
        MISSING=$((MISSING + 1))
    else
        echo "OK: extern/$submod"
    fi
done

echo
if [ $MISSING -gt 0 ]; then
    echo "WARNING: $MISSING submodule(s) appear to be incomplete."
    echo "Try running this script again, or manually run:"
    echo "  git submodule update --init --recursive --force"
    exit 1
fi

echo "================================================================================"
echo "SUCCESS: All submodules initialized!"
echo "================================================================================"
echo
echo "You can now configure and build the project:"
echo "  cmake -B build"
echo "  cmake --build build"
echo
