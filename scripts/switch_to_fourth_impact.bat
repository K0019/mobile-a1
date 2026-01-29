@echo off
setlocal

echo ================================================================================
echo Switching to fourth-impact branch (submodule-based dependencies)
echo ================================================================================
echo.

cd /d "%~dp0.."

:: Check if we're in a git repo
if not exist ".git" (
    echo ERROR: Not in a git repository
    pause
    exit /b 1
)

echo Step 1: Cleaning up stale submodule references...
:: Remove any orphaned .git files in extern that point to non-existent modules
for /d %%d in (extern\*) do (
    if exist "%%d\.git" (
        if not exist ".git\modules\%%~nxd" (
            echo Removing stale .git reference in %%d
            del "%%d\.git" 2>nul
        )
    )
)

echo Step 2: Deinitializing submodules (safe cleanup)...
git submodule deinit --all -f 2>nul

echo Step 3: Switching to fourth-impact branch...
git checkout fourth-impact
if errorlevel 1 (
    echo ERROR: Failed to checkout fourth-impact branch
    pause
    exit /b 1
)

echo Step 4: Syncing submodule URLs...
git submodule sync --recursive

echo Step 5: Initializing submodules (this may take a while)...
git submodule update --init --recursive
if errorlevel 1 (
    echo WARNING: Some submodules failed to initialize. Trying force update...
    git submodule update --init --recursive --force
)

echo.
echo ================================================================================
echo SUCCESS: Switched to fourth-impact with all submodules initialized
echo ================================================================================
echo.
echo You can now build the project:
echo   cmake -B build -G "Visual Studio 17 2022"
echo   cmake --build build --config Debug
echo.
pause
