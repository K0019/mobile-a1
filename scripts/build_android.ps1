<#
.SYNOPSIS
    One-command Android build: asset compile → ASTC recompress → manifest generate → Gradle build.

.PARAMETER BuildType
    Gradle build type: Debug (default) or Release.

.PARAMETER SkipAssetCompile
    Skip the ASTC texture recompression step (useful if textures haven't changed).

.PARAMETER SkipManifest
    Skip manifest regeneration (useful if no assets were added/removed).

.PARAMETER InstallOnly
    Only run Gradle installDebug/installRelease (skip all asset steps).

.PARAMETER AssetCompilerPath
    Path to AssetCompiler.exe. Defaults to Tools/Release/AssetCompiler.exe.
#>
param(
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Debug",

    [switch]$SkipAssetCompile,
    [switch]$SkipManifest,
    [switch]$InstallOnly,
    [string]$AssetCompilerPath = "Tools\Release\AssetCompiler.exe"
)

$ErrorActionPreference = "Stop"
# Script is at scripts/build_android.ps1, so repo root is one level up from scripts/
$RepoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $RepoRoot

try {
    # --- Auto-detect JAVA_HOME ---
    if (-not $env:JAVA_HOME) {
        $jbrCandidates = @(
            "$env:ProgramFiles\Android\Android Studio\jbr",
            "${env:ProgramFiles(x86)}\Android\Android Studio\jbr",
            "$env:LOCALAPPDATA\Programs\Android Studio\jbr"
        )
        foreach ($jbr in $jbrCandidates) {
            if (Test-Path "$jbr\bin\java.exe") {
                $env:JAVA_HOME = $jbr
                Write-Host "Auto-detected JAVA_HOME: $jbr" -ForegroundColor DarkGray
                break
            }
        }
        if (-not $env:JAVA_HOME) {
            Write-Error "JAVA_HOME not set and Android Studio JBR not found. Set JAVA_HOME or install Android Studio."
            exit 1
        }
    }

    # --- Auto-generate local.properties if missing ---
    $localProps = Join-Path $RepoRoot "android\local.properties"
    if (-not (Test-Path $localProps)) {
        $sdkDir = $null
        foreach ($candidate in @($env:ANDROID_HOME, $env:ANDROID_SDK_ROOT, "$env:LOCALAPPDATA\Android\Sdk")) {
            if ($candidate -and (Test-Path $candidate)) {
                $sdkDir = $candidate
                break
            }
        }
        if ($sdkDir) {
            $escaped = $sdkDir.Replace('\', '\\').Replace(':', '\:')
            Set-Content -Path $localProps -Value "sdk.dir=$escaped"
            Write-Host "Generated local.properties (sdk.dir=$sdkDir)" -ForegroundColor DarkGray
        } else {
            Write-Warning "Could not auto-detect Android SDK. Create android/local.properties manually."
        }
    }

    # --- Find Python ---
    $python = $null
    foreach ($candidate in @("python", "py", "python3")) {
        try {
            & $candidate --version 2>&1 | Out-Null
            $python = $candidate
            break
        } catch { }
    }
    if (-not $python) {
        Write-Error "Python not found. Install Python 3 and ensure it's on PATH."
        exit 1
    }
    Write-Host "[1/4] Using Python: $python" -ForegroundColor Cyan

    if (-not $InstallOnly) {
        # --- Step 1: ASTC texture recompression ---
        if (-not $SkipAssetCompile) {
            if (-not (Test-Path $AssetCompilerPath)) {
                Write-Warning "AssetCompiler not found at '$AssetCompilerPath'. Skipping ASTC recompression."
                Write-Warning "Build it first: cmake --build build --target AssetCompiler --config Release"
            } else {
                Write-Host "[2/4] Recompressing textures to ASTC for Android..." -ForegroundColor Cyan
                & $python scripts/rebuild_android_astc_textures.py `
                    --assetcompiler $AssetCompilerPath `
                    --assets-root Assets `
                    --include-non-textures
                if ($LASTEXITCODE -ne 0) {
                    Write-Error "ASTC recompression failed (exit=$LASTEXITCODE)"
                    exit $LASTEXITCODE
                }
            }
        } else {
            Write-Host "[2/4] Skipping ASTC recompression (--SkipAssetCompile)" -ForegroundColor Yellow
        }

        # --- Step 2: Regenerate asset manifest ---
        if (-not $SkipManifest) {
            Write-Host "[3/4] Regenerating asset_manifest.txt..." -ForegroundColor Cyan
            & $python generate_android_assets_manifest.py
            if ($LASTEXITCODE -ne 0) {
                Write-Error "Manifest generation failed (exit=$LASTEXITCODE)"
                exit $LASTEXITCODE
            }
        } else {
            Write-Host "[3/4] Skipping manifest generation (--SkipManifest)" -ForegroundColor Yellow
        }
    } else {
        Write-Host "[2/4] Skipping asset steps (--InstallOnly)" -ForegroundColor Yellow
        Write-Host "[3/4] Skipping asset steps (--InstallOnly)" -ForegroundColor Yellow
    }

    # --- Step 3: Gradle build + install ---
    $gradleTask = if ($BuildType -eq "Release") { "installRelease" } else { "installDebug" }
    Write-Host "[4/4] Running Gradle :app:$gradleTask..." -ForegroundColor Cyan
    Push-Location android
    try {
        & ./gradlew ":app:$gradleTask"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Gradle build failed (exit=$LASTEXITCODE)"
            exit $LASTEXITCODE
        }
    } finally {
        Pop-Location
    }

    Write-Host "Android build complete!" -ForegroundColor Green
} finally {
    Pop-Location
}
