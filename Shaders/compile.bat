@echo off
setlocal enabledelayedexpansion

if "%~1" == "" (
    rem Compile all shaders
    for %%f in (*.vert *.frag *.comp) do (
        call :CompileShader "%%f"
    )
) else (
    rem Compile single shader
    call :CompileShader "%~1"
)
exit /b 0

:CompileShader
set "inputFile=%~1"
set "outputFile=%~dpn1%~x1.spv"
echo Compiling shader: %inputFile%

rem Configure common compilation flags
set "COMMON_FLAGS=--target-env=vulkan1.3"
set "COMMON_FLAGS=%COMMON_FLAGS% -O"

rem Extension-specific flags for compute shaders
if /I "%~x1"==".comp" (
    set "SHADER_FLAGS=%COMMON_FLAGS%"
    set "SHADER_FLAGS=%SHADER_FLAGS% --target-spv=spv1.3"
) else (
    set "SHADER_FLAGS=%COMMON_FLAGS%"
)

glslc.exe %SHADER_FLAGS% "%inputFile%" -o "%outputFile%"
if errorlevel 1 (
    echo Shader compilation failed: %inputFile%
    exit /b 1
) else (
    echo Shader compiled successfully: %outputFile%
)
exit /b 0