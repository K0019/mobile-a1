# Android Build Script for MagicEngine
# Usage: .\build_android.ps1 [-Install] [-Clean]

param(
    [switch]$Install,  # Install APK to connected device after build
    [switch]$Clean     # Clean build (removes copied assets and rebuilds)
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$AndroidAssets = "$PSScriptRoot\app\src\main\assets"
$AssetsSource = "$RepoRoot\Assets"

Write-Host "=== MagicEngine Android Build ===" -ForegroundColor Cyan

# Check for JAVA_HOME or Android Studio JBR
if (-not $env:JAVA_HOME) {
    $AndroidStudioJBR = "C:\Program Files\Android\Android Studio\jbr"
    if (Test-Path $AndroidStudioJBR) {
        $env:JAVA_HOME = $AndroidStudioJBR
        Write-Host "Using Android Studio JBR: $AndroidStudioJBR" -ForegroundColor Yellow
    } else {
        Write-Error "JAVA_HOME not set and Android Studio JBR not found"
        exit 1
    }
}

# Clean if requested
if ($Clean) {
    Write-Host "`n[1/4] Cleaning previous build..." -ForegroundColor Green
    & "$PSScriptRoot\gradlew.bat" clean

    # Remove copied assets (keep asset_manifest.txt and VkLayer)
    $AssetDirs = @("Fonts", "behaviourtrees", "compiledassets", "images", "models",
                   "navmeshdata", "prefabs", "scenes", "scripts", "sounds")
    foreach ($dir in $AssetDirs) {
        $path = Join-Path $AndroidAssets $dir
        if (Test-Path $path) {
            Remove-Item -Recurse -Force $path
            Write-Host "  Removed $dir"
        }
    }
} else {
    Write-Host "`n[1/4] Skipping clean (use -Clean to force)" -ForegroundColor Gray
}

# Copy assets
Write-Host "`n[2/4] Copying assets..." -ForegroundColor Green

$AssetMappings = @{
    "compiledassets" = "compiledassets"
    "Scenes"         = "scenes"          # Note: lowercase target for Android case-sensitivity
    "scripts"        = "scripts"
    "behaviourtrees" = "behaviourtrees"
    "prefabs"        = "prefabs"
    "navmeshdata"    = "navmeshdata"
    "Fonts"          = "Fonts"
    "sounds"         = "sounds"
    "images"         = "images"
}

foreach ($mapping in $AssetMappings.GetEnumerator()) {
    $src = Join-Path $AssetsSource $mapping.Key
    $dst = Join-Path $AndroidAssets $mapping.Value

    if (Test-Path $src) {
        if (-not (Test-Path $dst)) {
            New-Item -ItemType Directory -Path $dst -Force | Out-Null
        }
        Copy-Item -Path "$src\*" -Destination $dst -Recurse -Force
        $count = (Get-ChildItem -Path $dst -Recurse -File).Count
        Write-Host "  $($mapping.Key) -> $($mapping.Value) ($count files)"
    } else {
        Write-Host "  $($mapping.Key) not found, skipping" -ForegroundColor Yellow
    }
}

# Generate asset manifest
Write-Host "`n[3/4] Generating asset_manifest.txt..." -ForegroundColor Green

$ManifestPath = Join-Path $AndroidAssets "asset_manifest.txt"
$ManifestEntries = @()

Get-ChildItem -Path $AndroidAssets -Recurse -File |
    Where-Object { $_.Name -ne "asset_manifest.txt" -and $_.Name -ne "VkLayer_khronos_validation.json" } |
    ForEach-Object {
        $relativePath = $_.FullName.Substring($AndroidAssets.Length + 1).Replace('\', '/')
        $ManifestEntries += $relativePath
    }

$ManifestEntries | Sort-Object | Set-Content -Path $ManifestPath -Encoding UTF8
Write-Host "  Generated manifest with $($ManifestEntries.Count) entries"

# Build APK
Write-Host "`n[4/4] Building APK..." -ForegroundColor Green

Push-Location $PSScriptRoot
try {
    & "$PSScriptRoot\gradlew.bat" assembleDebug
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed"
        exit 1
    }
} finally {
    Pop-Location
}

$ApkPath = "$PSScriptRoot\app\build\outputs\apk\debug\app-debug.apk"
if (Test-Path $ApkPath) {
    $ApkSize = [math]::Round((Get-Item $ApkPath).Length / 1MB, 2)
    Write-Host "`nBuild successful! APK: $ApkSize MB" -ForegroundColor Green
    Write-Host "  $ApkPath"
} else {
    Write-Error "APK not found at expected location"
    exit 1
}

# Install if requested
if ($Install) {
    Write-Host "`nInstalling to device..." -ForegroundColor Green
    adb install -r $ApkPath
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Launching app..." -ForegroundColor Green
        adb shell am start -n com.ryengine.vulkan/com.magicengine.kurorekishi.MainActivity
    }
}

Write-Host "`n=== Done ===" -ForegroundColor Cyan
