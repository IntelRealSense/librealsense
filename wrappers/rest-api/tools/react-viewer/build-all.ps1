#!/usr/bin/env pwsh
# Complete RealSense Viewer Build Script
# Builds: FastAPI executable + React UI + Tauri bundles
# Output: All artifacts in ./build/ directory

param(
    [switch]$Clean,
    [switch]$Help
)

if ($Help) {
    Write-Host @"
RealSense Viewer - Complete Build Script
========================================

Usage:
  .\build-all.ps1                 # Build everything
  .\build-all.ps1 -Clean          # Clean and rebuild
  .\build-all.ps1 -Help           # Show this help

Output Locations:
    - FastAPI executable:    ./build/rest-api-dist/realsense_api/
  - React build:           ./dist/
  - Tauri bundles:         ./build/tauri/release/bundle/
                          (msi installer)

Requirements:
  - Node.js 18+
  - Python 3.10+
  - Rust 1.56+
  - PyInstaller (pip install pyinstaller)

"@
    exit 0
}

# Colors for output
$SuccessColor = 'Green'
$ErrorColor = 'Red'
$WarningColor = 'Yellow'
$InfoColor = 'Cyan'

function Write-Success { Write-Host -ForegroundColor $SuccessColor "[OK] $args" }
function Write-Error { Write-Host -ForegroundColor $ErrorColor "[ERROR] $args" }
function Write-Warning { Write-Host -ForegroundColor $WarningColor "[WARN] $args" }
function Write-Info { Write-Host -ForegroundColor $InfoColor "[INFO] $args" }

# Track timing
$StartTime = Get-Date

function Measure-Duration {
    param([ScriptBlock]$Block)
    $Start = Get-Date
    & $Block
    $Duration = (Get-Date) - $Start
    return $Duration
}

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  RealSense Viewer - Complete Build" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Resolve project root for shared output locations
$ScriptDir = $PSScriptRoot
$ProjectRoot = Resolve-Path (Join-Path $ScriptDir "..\..\..\..")
$RestApiOutput = Join-Path $ProjectRoot "build\rest-api-dist"
$RestApiWork = Join-Path $ProjectRoot "build\rest-api-work"

# Step 1: Build FastAPI Executable
Write-Info "Step 1/3: Building FastAPI executable with PyInstaller..."
Push-Location (Join-Path $ScriptDir "..\..")

if ($Clean) {
    Write-Warning "Cleaning FastAPI build artifacts..."
    if (Test-Path "build") { Remove-Item "build" -Recurse -Force }
    if (Test-Path "dist") { Remove-Item "dist" -Recurse -Force }
    if (Test-Path "__pycache__") { Remove-Item "__pycache__" -Recurse -Force }
}

$Duration = Measure-Duration {
    if (Test-Path ".\build\build.ps1") {
        & .\build\build.ps1 -OutputDir $RestApiOutput -Clean:$Clean
        if (-not $?) { throw "FastAPI build script failed" }
    }
    else {
        Write-Warning "rest-api\\build\\build.ps1 not found; invoking PyInstaller directly..."
        # Pick a python: $env:PYTHON override, else python on PATH, else py launcher,
        # else common Windows install locations (Jenkins agents park it under C:\Python*_64).
        $PyBin = $env:PYTHON
        if (-not $PyBin) {
            foreach ($cand in @("python", "py", "C:/Python310_64/python.exe", "C:/Python311_64/python.exe", "C:/Python313_64/python.exe")) {
                if (Get-Command $cand -ErrorAction SilentlyContinue) { $PyBin = $cand; break }
            }
        }
        if (-not $PyBin) {
            Write-Error "Python not found. Tried: python, py, C:/Python31*_64/python.exe. Set `$env:PYTHON to override."
            throw "Python missing"
        }
        Write-Info "Using Python: $PyBin"
        & $PyBin -c "import PyInstaller" 2>$null
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "PyInstaller not importable for $PyBin; installing via pip..."
            & $PyBin -m pip install pyinstaller | ForEach-Object { Write-Host $_ }
            if ($LASTEXITCODE -ne 0) { throw "pip install pyinstaller failed" }
        }
        if (-not (Test-Path $RestApiOutput)) { New-Item -ItemType Directory -Path $RestApiOutput | Out-Null }
        if (-not (Test-Path $RestApiWork)) { New-Item -ItemType Directory -Path $RestApiWork | Out-Null }
        & $PyBin -m PyInstaller main.py --name realsense_api --distpath $RestApiOutput --workpath $RestApiWork -y | ForEach-Object { Write-Host $_ }
        # Verify build output rather than relying on $LASTEXITCODE alone
        $builtExe = Get-ChildItem -Path $RestApiOutput -Filter "realsense_api.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $builtExe) { throw "PyInstaller build failed: executable not found under $RestApiOutput" }
    }
}
Write-Success "FastAPI executable built in $($Duration.TotalSeconds)s"

# Copy to staging resources (outside source tree) for bundling
Write-Info "Copying FastAPI bundle to Tauri staging resources..."
$BundleDir = Join-Path $RestApiOutput "realsense_api"
if (-not (Test-Path $BundleDir)) {
    Write-Error "FastAPI bundle directory not found at $BundleDir"; Pop-Location; exit 1
}

$TauriResources = Join-Path $ScriptDir "src-tauri\resources"
if (-not (Test-Path $TauriResources)) { New-Item -ItemType Directory -Path $TauriResources | Out-Null }

# Remove previous staged bundle and copy fresh (exe + _internal/ with DLLs)
$TargetBundle = Join-Path $TauriResources "realsense_api"
if (Test-Path $TargetBundle) { Remove-Item $TargetBundle -Recurse -Force }
Copy-Item $BundleDir -Destination $TauriResources -Recurse -Force
Write-Success "FastAPI bundle staged (including _internal/ directory)"

Pop-Location

# Step 2: Build React UI
Write-Info "Step 2/3: Building React UI..."
Push-Location $ScriptDir

if ($Clean) {
    Write-Warning "Cleaning Node modules cache..."
    if (Test-Path "dist") { Remove-Item "dist" -Recurse -Force }
}

if (-not (Test-Path "node_modules")) {
    Write-Info "node_modules missing; running npm ci..."
    & npm ci | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -ne 0) {
        Write-Error "npm ci failed"
        Pop-Location
        exit 1
    }
}

$Duration = Measure-Duration {
    Write-Host "Running npm build..." -ForegroundColor Gray
    & npm run build | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -ne 0) {
        Write-Error "React build failed!"
        Pop-Location
        exit 1
    }
}
Write-Success "React UI built in $($Duration.TotalSeconds)s"

# Step 3: Build Tauri Bundles
Write-Info "Step 3/3: Building Tauri production bundles..."

# Ensure cargo is available. rustup installs to %USERPROFILE%\.cargo\bin but
# fresh, non-login shells (e.g. Jenkins agents) may not have it on PATH. If
# rustup isn't installed at all, download rustup-init.exe directly and run it
# non-interactively - one-time ~2 min per agent, then ~/.cargo/bin sticks
# around for every subsequent run. win.rustup.rs is the official redirector.
$CargoBin = Join-Path $env:USERPROFILE ".cargo\bin"
if (-not (Get-Command cargo -ErrorAction SilentlyContinue)) {
    if (Test-Path (Join-Path $CargoBin "cargo.exe")) {
        Write-Warning "cargo not on PATH; adding $CargoBin"
        $env:PATH = $env:PATH.TrimEnd(';') + ';' + $CargoBin
    }
    else {
        Write-Warning "cargo not found; downloading rustup-init (one-time per agent, ~2 min)..."
        $rustupInit = Join-Path $env:TEMP "rustup-init.exe"
        try {
            [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
            Invoke-WebRequest -UseBasicParsing -Uri "https://win.rustup.rs/x86_64" -OutFile $rustupInit
            & $rustupInit -y --default-toolchain stable --default-host x86_64-pc-windows-msvc --profile minimal | ForEach-Object { Write-Host $_ }
        } catch {
            Write-Warning "rustup-init download/run failed: $_"
        }
        if (Test-Path (Join-Path $CargoBin "cargo.exe")) {
            $env:PATH = $env:PATH.TrimEnd(';') + ';' + $CargoBin
        }
    }
}
if (-not (Get-Command cargo -ErrorAction SilentlyContinue)) {
    Write-Error "cargo not found and could not auto-install. Install Rust from https://rustup.rs/ and retry."
    throw "cargo missing"
}

# Ensure Cargo outputs to project-level build/tauri-target
$CargoTarget = Join-Path $ProjectRoot "build\tauri-target"
Write-Info "Setting CARGO_TARGET_DIR to: $CargoTarget"
if (-not (Test-Path $CargoTarget)) { New-Item -ItemType Directory -Path $CargoTarget | Out-Null }
$env:CARGO_TARGET_DIR = $CargoTarget

# Clean legacy src-tauri/target if present
if (Test-Path "src-tauri\target") {
    Write-Warning "Removing legacy src-tauri\\target directory..."
    Remove-Item "src-tauri\target" -Recurse -Force
}

$Duration = Measure-Duration {
    Write-Host "This will compile Rust and create installers (this may take 2-5 minutes)..." -ForegroundColor Gray
    & npm run tauri:build -- --bundles msi | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Tauri build failed!"
        Pop-Location
        exit 1
    }
}
Write-Success "Tauri bundles created in $($Duration.TotalSeconds)s"

Pop-Location

# Summary
Write-Host ""
Write-Host "============================================================" -ForegroundColor Green
Write-Host "  BUILD COMPLETE!" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Green

$TotalDuration = (Get-Date) - $StartTime
Write-Success "Total build time: $($TotalDuration.TotalSeconds)s"

Write-Host ""
Write-Host "Output Artifacts:" -ForegroundColor Cyan
Write-Host "  MSI Installer:  " -NoNewline
Write-Host "build/tauri-target/release/bundle/msi/" -ForegroundColor Yellow
Write-Host "  Portable EXE:   " -NoNewline
Write-Host "build/tauri-target/release/" -ForegroundColor Yellow

Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Cyan
Write-Host "  1. Install one of the generated installers"
Write-Host "  2. Launch 'RealSense Viewer' from Start Menu"
Write-Host "  3. Open DevTools (F12) to verify API connection"
Write-Host "  4. Check Device Panel for connected RealSense cameras"

Write-Host ""
