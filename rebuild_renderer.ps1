# Force rebuild Renderer.cpp to apply changes
Set-ExecutionPolicy -Scope Process Bypass
$ErrorActionPreference = 'Stop'

Write-Host "=== FORCE REBUILD RENDERER ===" -ForegroundColor Green

# Kill running app
Write-Host "Stopping any running AI_Movie_Studio..." -ForegroundColor Yellow
Get-Process -Name "AI_Movie_Studio" -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500

# Delete cached object file
$objPath = "build\CMakeFiles\AI_Movie_Studio.dir\Debug\Renderer.obj"
if (Test-Path $objPath) {
    Write-Host "Deleting cached $objPath" -ForegroundColor Yellow
    Remove-Item $objPath -Force
    Write-Host "Deleted!" -ForegroundColor Green
} else {
    Write-Host "Object file not found (will be built fresh)" -ForegroundColor Yellow
}

# Clean build
Write-Host "`nCleaning build..." -ForegroundColor Cyan
cmake --build build --config Debug --clean-first

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed!"
    exit 1
}

Write-Host "`n=== BUILD SUCCESSFUL ===" -ForegroundColor Green
Write-Host "Run the app with: .\build\bin\Debug\AI_Movie_Studio.exe" -ForegroundColor Yellow
