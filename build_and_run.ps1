# Simple build and run script for the C++ app
# Use this when AI service is already running

Set-ExecutionPolicy -Scope Process Bypass
$ErrorActionPreference = 'Stop'

Write-Host "=== BUILDING AND RUNNING AI MOVIE STUDIO ===" -ForegroundColor Green

# Build the project
Write-Host "`n1. Building C++ project..." -ForegroundColor Cyan
if (-not (Test-Path ".\build")) { 
  Write-Host "Creating build directory..." -ForegroundColor Yellow
  cmake -S . -B build 
}

Write-Host "Compiling..." -ForegroundColor Yellow
cmake --build build --config Debug
if ($LASTEXITCODE -ne 0) {
  Write-Error "Build failed!"
  exit 1
}
Write-Host "✓ Build successful" -ForegroundColor Green

# Run the app
Write-Host "`n2. Launching AI Movie Studio..." -ForegroundColor Cyan
Write-Host "Make sure AI service is running on port 8080" -ForegroundColor Yellow
Write-Host "If not, run: .\run.ps1" -ForegroundColor Yellow

# Kill any stale instances
Write-Host "Stopping any existing AI_Movie_Studio processes..." -ForegroundColor Yellow
Get-Process -Name "AI_Movie_Studio" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

# Run in foreground to surface errors, and wait for exit if it crashes
$exe = Join-Path $PSScriptRoot "build\bin\Debug\AI_Movie_Studio.exe"
if (-not (Test-Path $exe)) {
  Write-Error "Executable not found: $exe"
  exit 1
}

Write-Host "Starting: $exe" -ForegroundColor Yellow
& $exe
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
  Write-Error "AI_Movie_Studio exited with code $exitCode"
  Write-Host "Tip: Check Windows Event Viewer -> Windows Logs -> Application for crash details." -ForegroundColor Yellow
  exit $exitCode
}

Write-Host "✓ App launched (process is running)" -ForegroundColor Green
