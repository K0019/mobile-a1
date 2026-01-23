@echo off
setlocal enabledelayedexpansion

echo ================================================================================
echo MagicEngine Submodule Setup Script
echo ================================================================================
echo.

:: Get the script directory and navigate to repo root
cd /d "%~dp0.."

:: Check if this is a git repository
if not exist ".git" (
    echo ERROR: This script must be run from within the MagicEngine repository.
    echo Please navigate to the repository root and try again.
    pause
    exit /b 1
)

echo Step 1: Synchronizing submodule URLs...
git submodule sync --recursive
if errorlevel 1 (
    echo ERROR: Failed to sync submodules. Check your git installation.
    pause
    exit /b 1
)
echo Done.
echo.

echo Step 2: Initializing and updating submodules...
echo This may take several minutes on first run...
echo.
git submodule update --init --recursive
if errorlevel 1 (
    echo ERROR: Failed to update submodules.
    echo Try running: git submodule update --init --recursive --force
    pause
    exit /b 1
)
echo Done.
echo.

echo Step 3: Verifying critical submodules...
set MISSING=0
set SUBMODULES=Vulkan-Headers volk VulkanMemoryAllocator glm fmt glslang imgui glfw

for %%s in (%SUBMODULES%) do (
    if not exist "extern\%%s\CMakeLists.txt" (
        echo MISSING: extern\%%s
        set /a MISSING+=1
    ) else (
        echo OK: extern\%%s
    )
)

echo.
if !MISSING! gtr 0 (
    echo WARNING: !MISSING! submodule^(s^) appear to be incomplete.
    echo Try running this script again, or manually run:
    echo   git submodule update --init --recursive --force
    pause
    exit /b 1
)

echo ================================================================================
echo SUCCESS: All submodules initialized!
echo ================================================================================
echo.
echo You can now configure and build the project:
echo   cmake -B build -G "Visual Studio 17 2022"
echo   cmake --build build --config Debug
echo.
pause
