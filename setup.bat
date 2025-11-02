@echo off
setlocal enabledelayedexpansion

set "VCPKG_ROOT=extern\vcpkg"
set "PATH=%VCPKG_ROOT%;%PATH%"

echo VCPKG_ROOT is set to: %VCPKG_ROOT%

REM Configuration - set defaults
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release
if "%ANDROID_PLATFORM%"=="" set ANDROID_PLATFORM=30
if "%ANDROID_ABIS%"=="" set ANDROID_ABIS=arm64-v8a,x86_64

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
if /i "%TARGET%"=="android" echo Android ABIs: %ANDROID_ABIS%
echo ===============================================

call :check_requirements
if errorlevel 1 (
    if "%SHOW_MENU%"=="true" pause
    exit /b 1
)

if /i "%TARGET%"=="android" (
    call :setup_android_ndk
    if errorlevel 1 (
        if "%SHOW_MENU%"=="true" pause
        exit /b 1
    )
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
    REM Need Ninja for Android build
	call :check_ninja
    if errorlevel 1 (
        if "%SHOW_MENU%"=="true" pause
        exit /b 1
    )
    if "%VCPKG_ROOT%"=="" (
        echo [ERROR] VCPKG_ROOT environment variable is not set
        echo [ERROR] Please set it to your vcpkg installation directory
        if "%SHOW_MENU%"=="true" pause
        exit /b 1
    )
    if not exist "%VCPKG_ROOT%" (
        echo [ERROR] VCPKG_ROOT directory does not exist: %VCPKG_ROOT%
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

cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_TOOLCHAIN_FILE="%CD%\..\extern\vcpkg\scripts\buildsystems\vcpkg.cmake"
if errorlevel 1 (
    echo [ERROR] CMake configuration failed
    cd ..
    if "%SHOW_MENU%"=="true" pause
    exit /b 1
)

cd ..
echo [INFO] Windows build complete. You can now open the solution in build/ or run the Editor.
exit /b 0

:setup_android_ndk
echo [INFO] Setting up Android NDK...

if not "%ANDROID_NDK_HOME%"=="" (
    if exist "%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake" (
        echo [INFO] Using existing ANDROID_NDK_HOME: %ANDROID_NDK_HOME%
        exit /b 0
    )
)

echo [INFO] Searching for Android NDK installations...
set BASE_PATHS[0]=%USERPROFILE%\AppData\Local\Android\Sdk\ndk
set BASE_PATHS[1]=%ANDROID_HOME%\ndk
set BASE_PATHS[2]=%ANDROID_SDK_ROOT%\ndk

for /l %%i in (0,1,2) do (
    call set "base_path=%%BASE_PATHS[%%i]%%"
    if exist "!base_path!" (
        for /d %%d in ("!base_path!\*") do (
            if exist "%%d\build\cmake\android.toolchain.cmake" (
                set ANDROID_NDK_HOME=%%d
                echo [INFO] Found Android NDK at: !ANDROID_NDK_HOME!
                exit /b 0
            )
        )
    )
)

echo [ERROR] Android NDK not found. Please set ANDROID_NDK_HOME manually.
if "%SHOW_MENU%"=="true" pause
exit /b 1

:abi_to_triplet
set "input_abi=%~1"
if "%input_abi%"=="arm64-v8a" (
    set "vcpkg_triplet=arm64-android"
) else if "%input_abi%"=="armeabi-v7a" (
    set "vcpkg_triplet=arm-neon-android"
) else if "%input_abi%"=="x86_64" (
    set "vcpkg_triplet=x64-android"
) else if "%input_abi%"=="x86" (
    set "vcpkg_triplet=x86-android"
) else (
    echo [ERROR] Unsupported ABI: %input_abi%
    if "%SHOW_MENU%"=="true" pause
    exit /b 1
)
exit /b 0

:build_android
echo [INFO] Building engine for Android (%BUILD_TYPE%)...

echo [INFO] Running Rocky's python script
@echo off
py --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Python is not installed.
    pause
    exit /b 1
)
py generate_android_assets_manifest.py

if exist "android\install" rmdir /s /q android\install
if exist "build-android" rmdir /s /q build-android
mkdir android\install

set "abi_list=%ANDROID_ABIS%"
set "abi_count=0"

:parse_abis
for /f "tokens=1* delims=," %%a in ("%abi_list%") do (
    set "current_abi=%%a"
    set "abi_list=%%b"
    
    for /f "tokens=* delims= " %%c in ("!current_abi!") do set "current_abi=%%c"
    
    echo [INFO] Building for ABI: !current_abi! [%BUILD_TYPE%]
    
    call :abi_to_triplet "!current_abi!"
    if errorlevel 1 (
        if "%SHOW_MENU%"=="true" pause
        exit /b 1
    )
    
    set "build_dir=build-android-!current_abi!"
    mkdir "!build_dir!"
    cd "!build_dir!"
    
    REM Set compiler flags based on build type
    set "EXTRA_CXX_FLAGS="
    if /i "%BUILD_TYPE%"=="Debug" (
        REM Debug: Full symbols, no optimization, no inlining
        set "EXTRA_CXX_FLAGS=-g3 -O0 -fno-inline -fno-omit-frame-pointer -DDEBUG -D_DEBUG"
    ) else if /i "%BUILD_TYPE%"=="RelWithDebInfo" (
        REM RelWithDebInfo: Symbols + some optimization, allow inlining but keep frame pointers
        set "EXTRA_CXX_FLAGS=-g -O2 -fno-omit-frame-pointer -DNDEBUG"
    ) else (
        REM Release: Full optimization
        set "EXTRA_CXX_FLAGS=-O3 -DNDEBUG"
    )
    
    cmake .. ^
        -G "Ninja" ^
        -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
        -DCMAKE_INSTALL_PREFIX=../android/install/!current_abi! ^
        -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ^
        -DVCPKG_TARGET_TRIPLET=!vcpkg_triplet! ^
        -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake ^
        -DANDROID_ABI=!current_abi! ^
        -DANDROID_PLATFORM=%ANDROID_PLATFORM% ^
        -DANDROID=ON ^
        -DCMAKE_CXX_FLAGS="!EXTRA_CXX_FLAGS! -Wno-error" ^
        -DCMAKE_C_FLAGS="!EXTRA_CXX_FLAGS! -Wno-error"
    
    if errorlevel 1 (
        echo [ERROR] CMake configuration failed for !current_abi!
        cd ..
        if "%SHOW_MENU%"=="true" pause
        exit /b 1
    )
    
    cmake --build . --target install
    if errorlevel 1 (
        echo [ERROR] Build failed for !current_abi!
        cd ..
        if "%SHOW_MENU%"=="true" pause
        exit /b 1
    )
    
	REM Copy engine headers
	REM if exist "..\android\install\!current_abi!\include\" (
	REM	echo engine headers already installed
	REM) else (
	REM	echo Installing engine headers...
	REM	mkdir "..\android\install\!current_abi!\include"
	REM	xcopy /E /I /Y "..\engine-core\core\engine\engine.h" "..\android\install\!current_abi!\include\" >nul
	REM)

    REM Copy vcpkg dependencies
    set "vcpkg_install_dir=.\vcpkg_installed\!vcpkg_triplet!"
    set "abi_install_dir=..\android\install\!current_abi!"
    
    if exist "!vcpkg_install_dir!" (
        REM Copy debug libraries
        if exist "!vcpkg_install_dir!\debug\lib\" (
            xcopy /E /I /Y "!vcpkg_install_dir!\debug\lib\*" "!abi_install_dir!\debug\lib\" >nul
        )
        REM Copy release libraries  
        if exist "!vcpkg_install_dir!\lib\" (
            xcopy /E /I /Y "!vcpkg_install_dir!\lib\*" "!abi_install_dir!\lib\" >nul
        )
        REM Copy includes and share
        if exist "!vcpkg_install_dir!\include\" (
            xcopy /E /I /Y "!vcpkg_install_dir!\include\*" "!abi_install_dir!\include\" >nul
        )
        if exist "!vcpkg_install_dir!\share\" (
            xcopy /E /I /Y "!vcpkg_install_dir!\share\*" "!abi_install_dir!\share\" >nul
        )
    )
    
    cd ..
    set /a abi_count+=1
)

if not "%abi_list%"=="" goto :parse_abis

echo [INFO] Android build complete (%BUILD_TYPE%). Libraries installed to android/install/
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
echo   ANDROID_ABIS    Android ABIs to build (default: arm64-v8a,x86_64)
echo   ANDROID_PLATFORM Android API level (default: 30)
echo   VCPKG_ROOT      Path to vcpkg (required for Android)
echo.
echo Examples:
echo   %0                              # Interactive menu
echo   %0 windows debug                # Build Windows in Debug
echo   %0 android debug --no-menu      # Build Android Debug without menu
echo   %0 android relwithdebinfo       # Build Android with symbols and optimization
echo   set BUILD_TYPE=Debug ^&^& %0 windows
if "%SHOW_MENU%"=="true" pause
exit /b 0


@REM @echo off
@REM setlocal

@REM set "BUILD_DIR=build"

@REM echo.
@REM echo Configuring project with CMake...

@REM :: Create the build directory if it doesn't exist
@REM if not exist "%BUILD_DIR%" (
@REM     mkdir "%BUILD_DIR%"
@REM )

@REM :: Navigate into the build directory and run CMake
@REM cd "%BUILD_DIR%"
@REM cmake ..

@REM if %errorlevel% neq 0 (
@REM     echo.
@REM     echo ERROR: CMake configuration failed.
@REM     pause
@REM     exit /b %errorlevel%
@REM )

@REM echo.
@REM echo CMake configuration complete.
@REM echo You can now open the .sln file in the '%BUILD_DIR%' directory.
@REM pause

@REM endlocal

:check_ninja
REM Check if already installed in our custom location
set "NINJA_DIR=%USERPROFILE%\.ninja"
set "NINJA_EXE=%NINJA_DIR%\ninja.exe"

if exist "%NINJA_EXE%" (
    echo [INFO] Ninja found at %NINJA_EXE%
    set "PATH=%NINJA_DIR%;%PATH%"
    exit /b 0
)

REM Check if ninja is in system PATH
where ninja >nul 2>&1
if %errorlevel%==0 (
    echo [INFO] Ninja already installed in PATH
    exit /b 0
)

echo [INFO] Ninja not found, installing...
if not exist "%NINJA_DIR%" mkdir "%NINJA_DIR%"

curl.exe -L -o "%NINJA_DIR%\ninja.zip" https://github.com/ninja-build/ninja/releases/latest/download/ninja-win.zip
if errorlevel 1 (
    echo [ERROR] Failed to download Ninja
    exit /b 1
)

tar -xf "%NINJA_DIR%\ninja.zip" -C "%NINJA_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to extract Ninja
    exit /b 1
)

del "%NINJA_DIR%\ninja.zip"

set "PATH=%NINJA_DIR%;%PATH%"

if not exist "%NINJA_EXE%" (
    echo [ERROR] Ninja installation failed
    exit /b 1
)

echo [INFO] Ninja installed to %NINJA_DIR%
exit /b 0