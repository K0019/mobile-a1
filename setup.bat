@echo off
setlocal enabledelayedexpansion

set "VCPKG_ROOT=extern\vcpkg"
set "PATH=%VCPKG_ROOT%;%PATH%"

echo VCPKG_ROOT is set to: %VCPKG_ROOT%

REM Configuration - set defaults
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release
set "COMPILE_ASSETS=ON"

REM Parse command line arguments
set "TARGET="
set "SHOW_MENU=true"

:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="windows" (
    set "TARGET=windows"
    shift
    goto :parse_args
)
if /i "%~1"=="android" (
    set "TARGET=android"
    shift
    goto :parse_args
)
if /i "%~1"=="debug" (
    set "BUILD_TYPE=Debug"
    shift
    goto :parse_args
)
if /i "%~1"=="release" (
    set "BUILD_TYPE=Release"
    shift
    goto :parse_args
)
if /i "%~1"=="relwithdebinfo" (
    set "BUILD_TYPE=RelWithDebInfo"
    shift
    goto :parse_args
)
if /i "%~1"=="--no-menu" (
    set "SHOW_MENU=false"
    shift
    goto :parse_args
)
if /i "%~1"=="--help" goto :show_help
if /i "%~1"=="-h" goto :show_help
if /i "%~1"=="/?" goto :show_help
echo Unknown argument: %~1
goto :show_help

:args_done

REM Show interactive menu if no target specified and menu not disabled
if "%TARGET%"=="" (
    if "%SHOW_MENU%"=="true" (
        call :show_menu
    ) else (
        echo [ERROR] No target specified. Use 'windows' or 'android' argument.
        echo Use --help for more information.
        if "%SHOW_MENU%"=="true" pause
        exit /b 1
    )
)

echo.
echo ===============================================
echo  engine Build Script
echo ===============================================
echo Target: %TARGET%
echo Build Type: %BUILD_TYPE%
echo ===============================================

call :check_requirements
if errorlevel 1 (
    if "%SHOW_MENU%"=="true" pause
    exit /b 1
)

if /i "%TARGET%"=="android" (
    call :build_android
    if errorlevel 1 (
        if "%SHOW_MENU%"=="true" pause
        exit /b 1
    )
) else (
    call :build_windows
    if errorlevel 1 (
        if "%SHOW_MENU%"=="true" pause
        exit /b 1
    )
)

echo [INFO] Build completed successfully!
if "%SHOW_MENU%"=="true" pause
exit /b 0

:show_menu
echo.
echo ===============================================
echo  engine Build System
echo ===============================================
echo.
echo Select build target:
echo   1) Windows (Desktop) - Debug
echo   2) Windows (Desktop) - Release
echo   3) Windows (Desktop) - RelWithDebInfo
echo   4) Android - Debug
echo   5) Android - Release
echo   6) Android - RelWithDebInfo
echo   8) School PC Lmao (Windows Debug - No Asset Compiler)
echo   7) Exit
echo.
set /p "choice=Enter choice (1-7): "

if "%choice%"=="1" (
    set "TARGET=windows"
    set "BUILD_TYPE=Debug"
    goto :eof
)
if "%choice%"=="2" (
    set "TARGET=windows"
    set "BUILD_TYPE=Release"
    goto :eof
)
if "%choice%"=="3" (
    set "TARGET=windows"
    set "BUILD_TYPE=RelWithDebInfo"
    goto :eof
)
if "%choice%"=="4" (
    set "TARGET=android"
    set "BUILD_TYPE=Debug"
    goto :eof
)
if "%choice%"=="5" (
    set "TARGET=android"
    set "BUILD_TYPE=Release"
    goto :eof
)
if "%choice%"=="6" (
    set "TARGET=android"
    set "BUILD_TYPE=RelWithDebInfo"
    goto :eof
)
if "%choice%"=="7" (
    echo Exiting...
    exit /b 0
)
if "%choice%"=="8" (
    set "TARGET=windows"
    set "BUILD_TYPE=Debug"
    set "COMPILE_ASSETS=OFF"
    goto :eof
)
echo Invalid choice. Please enter 1-7.
goto :show_menu

:check_requirements
echo [INFO] Checking requirements...

if not exist "vcpkg.json" (
    echo [ERROR] vcpkg.json not found
    if "%SHOW_MENU%"=="true" pause
    exit /b 1
)

cmake --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake is required but not installed
    if "%SHOW_MENU%"=="true" pause
    exit /b 1
)

if /i "%TARGET%"=="android" (
    powershell -NoProfile -Command "Get-Command powershell" >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] PowerShell is required for Android builds
        if "%SHOW_MENU%"=="true" pause
        exit /b 1
    )
)

echo [INFO] Requirements check passed
exit /b 0

:build_windows
echo [INFO] Building engine for Windows (%BUILD_TYPE%)...

if not exist "build" mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_ASSET_COMPILER=%COMPILE_ASSETS% -DCMAKE_TOOLCHAIN_FILE="%CD%\..\extern\vcpkg\scripts\buildsystems\vcpkg.cmake"
if errorlevel 1 (
    echo [ERROR] CMake configuration failed
    cd ..
    if "%SHOW_MENU%"=="true" pause
    exit /b 1
)

cd ..
echo [INFO] Windows build complete. You can now open the solution in build/ or run the Editor.
exit /b 0


:build_android
echo [INFO] Building engine for Android (%BUILD_TYPE%)...

REM Map BUILD_TYPE to what scripts\build_android.ps1 accepts (Debug or Release)
set "PS_BUILD_TYPE=%BUILD_TYPE%"
if /i "%BUILD_TYPE%"=="RelWithDebInfo" set "PS_BUILD_TYPE=Release"

powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_android.ps1 -BuildType %PS_BUILD_TYPE%
if errorlevel 1 (
    echo [ERROR] Android build failed.
    exit /b 1
)

echo [INFO] Android build complete (%BUILD_TYPE%).
exit /b 0

:show_help
echo Usage: %0 [target] [build-type] [options]
echo.
echo Targets:
echo   windows         Build for Windows desktop
echo   android         Build for Android
echo.
echo Build Types:
echo   debug           Debug build (no optimization, full symbols, no inlining)
echo   release         Release build (full optimization, no symbols)
echo   relwithdebinfo  Release with debug info (optimized but with symbols)
echo.
echo Options:
echo   --no-menu       Skip interactive menu (for CI/scripts)
echo   --help          Show this help
echo.
echo Environment Variables:
echo   BUILD_TYPE      Debug, Release, or RelWithDebInfo (default: Release)
echo.
echo Examples:
echo   %0                              ^# Interactive menu
echo   %0 windows debug                ^# Build Windows in Debug
echo   %0 android debug --no-menu      ^# Build Android Debug without menu
echo   %0 android relwithdebinfo       ^# Build Android with symbols and optimization
echo   set BUILD_TYPE=Debug ^&^& %0 windows
if "%SHOW_MENU%"=="true" pause
exit /b 0

