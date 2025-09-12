@echo off
setlocal

set "BUILD_DIR=build"

echo.
echo Configuring project with CMake...

:: Create the build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

:: Navigate into the build directory and run CMake
cd "%BUILD_DIR%"
cmake -G "Visual Studio 17 2022" ..

if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed. Something went horribly wrong. Please seek help.
    exit /b 1
)

echo.
echo CMake configuration complete.
echo You can now open the .sln file in the '%BUILD_DIR%' directory.

endlocal
