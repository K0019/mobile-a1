# Batch recompile all model assets
param(
    [switch]$DryRun = $false
)

$ErrorActionPreference = "Continue"

$projectRoot = "C:\Users\ryang\Documents\GitHub\gam300-magic"
$compiler = "$projectRoot\Tools\Release\AssetCompiler.exe"
$assetsRoot = "$projectRoot\Assets"
$outputRoot = "$projectRoot\Assets\compiledassets"

# Find all GLB and FBX files
$glbFiles = Get-ChildItem -Path "$assetsRoot\models" -Filter "*.glb" -Recurse -ErrorAction SilentlyContinue
$fbxFiles = Get-ChildItem -Path "$assetsRoot\models" -Filter "*.fbx" -Recurse -ErrorAction SilentlyContinue
$carFiles = Get-ChildItem -Path "$assetsRoot\fbxcars" -Filter "*.fbx" -Recurse -ErrorAction SilentlyContinue

$allFiles = @()
if ($glbFiles) { $allFiles += $glbFiles }
if ($fbxFiles) { $allFiles += $fbxFiles }
if ($carFiles) { $allFiles += $carFiles }

Write-Host "Found $($allFiles.Count) model files to compile" -ForegroundColor Cyan
Write-Host ""

$successCount = 0
$failCount = 0
$mixedCount = 0

foreach ($file in $allFiles) {
    $relativePath = $file.FullName.Substring($assetsRoot.Length + 1)
    $outputDir = Join-Path $outputRoot ($file.Directory.FullName.Substring($assetsRoot.Length + 1))

    # Create output directory if it doesn't exist
    if (-not (Test-Path $outputDir)) {
        if (-not $DryRun) {
            New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
        }
    }

    Write-Host "Compiling: $relativePath" -ForegroundColor Yellow

    if ($DryRun) {
        Write-Host "  [DRY RUN] Would run: $compiler -root `"$assetsRoot`" -input `"$($file.FullName)`" -output `"$outputDir`"" -ForegroundColor Gray
        continue
    }

    $output = & $compiler -root "$assetsRoot" -input "$($file.FullName)" -output "$outputDir" 2>&1
    $exitCode = $LASTEXITCODE

    # Check for MIXED winding in output
    $hasMixed = $output -match "MIXED"

    if ($exitCode -eq 0) {
        if ($hasMixed) {
            Write-Host "  SUCCESS (has MIXED winding meshes)" -ForegroundColor DarkYellow
            $mixedCount++
        } else {
            Write-Host "  SUCCESS" -ForegroundColor Green
        }
        $successCount++
    } else {
        Write-Host "  FAILED (exit code $exitCode)" -ForegroundColor Red
        Write-Host "  Output: $output" -ForegroundColor Red
        $failCount++
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Compilation Complete" -ForegroundColor Cyan
Write-Host "  Success: $successCount" -ForegroundColor Green
Write-Host "  With MIXED winding: $mixedCount" -ForegroundColor DarkYellow
Write-Host "  Failed: $failCount" -ForegroundColor Red
Write-Host "========================================" -ForegroundColor Cyan
