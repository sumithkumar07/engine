# Requires: Ollama installed (listens on 11434) and a model pulled (e.g., llama3:latest)

# Set execution policy for the current process only
Set-ExecutionPolicy -Scope Process Bypass

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

function Wait-Port($port, $timeoutSec=30) {
  $sw = [Diagnostics.Stopwatch]::StartNew()
  while ($sw.Elapsed.TotalSeconds -lt $timeoutSec) {
    $ok = (Test-NetConnection -ComputerName localhost -Port $port -ErrorAction SilentlyContinue).TcpTestSucceeded
    if ($ok) { return $true }
    Start-Sleep -Milliseconds 300
  }
  return $false
}

Write-Host "=== AI MOVIE STUDIO STARTUP SCRIPT ===" -ForegroundColor Green
Write-Host "Root directory: $root" -ForegroundColor Yellow

Write-Host "`n1. Checking Ollama on :11434..." -ForegroundColor Cyan
if (-not (Wait-Port 11434 10)) {
  Write-Error "Ollama not reachable on 11434. Start Ollama and rerun."
  Write-Host "To start Ollama:" -ForegroundColor Yellow
  Write-Host "  - Download from https://ollama.ai" -ForegroundColor Yellow
  Write-Host "  - Run: ollama serve" -ForegroundColor Yellow
  Write-Host "  - Pull a model: ollama pull llama3:latest" -ForegroundColor Yellow
  exit 1
}
Write-Host "✓ Ollama is running" -ForegroundColor Green

Write-Host "`n2. Setting up Python environment..." -ForegroundColor Cyan
$aisvc = Join-Path $root "ai_service"
Write-Host "AI service directory: $aisvc" -ForegroundColor Yellow
Set-Location $aisvc

if (-not (Test-Path ".\.venv\Scripts\Activate.ps1")) {
  Write-Host "Creating Python virtual environment..." -ForegroundColor Yellow
  python -m venv .venv
  if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to create virtual environment. Make sure Python is installed."
    exit 1
  }
}

Write-Host "Activating virtual environment..." -ForegroundColor Yellow
.\.venv\Scripts\Activate.ps1

Write-Host "Upgrading pip..." -ForegroundColor Yellow
python -m pip install --upgrade pip

Write-Host "Installing requirements..." -ForegroundColor Yellow
pip install -r requirements.txt
if ($LASTEXITCODE -ne 0) {
  Write-Error "Failed to install Python requirements"
  exit 1
}
Write-Host "✓ Python environment ready" -ForegroundColor Green

Write-Host "`n3. Starting AI service..." -ForegroundColor Cyan
$env:OLLAMA_HOST = "http://localhost:11434"
$env:OLLAMA_MODEL = "llama3:latest"
$env:AI_SERVICE_HOST = "127.0.0.1"
$env:AI_SERVICE_PORT = "8080"

Write-Host "Environment variables:" -ForegroundColor Yellow
Write-Host "  OLLAMA_HOST = $env:OLLAMA_HOST"
Write-Host "  OLLAMA_MODEL = $env:OLLAMA_MODEL"
Write-Host "  AI_SERVICE_HOST = $env:AI_SERVICE_HOST"
Write-Host "  AI_SERVICE_PORT = $env:AI_SERVICE_PORT"

Write-Host "`nStarting AI service in foreground (press Ctrl+C to stop)..." -ForegroundColor Yellow
Write-Host "If this fails, try manually:" -ForegroundColor Yellow
Write-Host "  cd ai_service" -ForegroundColor Yellow
Write-Host "  .\.venv\Scripts\Activate.ps1" -ForegroundColor Yellow
Write-Host "  python main.py" -ForegroundColor Yellow

# Start AI service in foreground
try {
  python main.py
} catch {
  Write-Error "AI service failed to start: $_"
  Write-Host "`nTrying alternative startup method..." -ForegroundColor Yellow
  try {
    python -m uvicorn main:app --host 127.0.0.1 --port 8080 --log-level debug
  } catch {
    Write-Error "Alternative startup also failed: $_"
    Write-Host "`nManual troubleshooting steps:" -ForegroundColor Red
    Write-Host "1. Check if Ollama is running: ollama list" -ForegroundColor Yellow
    Write-Host "2. Pull a model: ollama pull llama3:latest" -ForegroundColor Yellow
    Write-Host "3. Test Ollama: ollama run llama3:latest 'Hello'" -ForegroundColor Yellow
    Write-Host "4. Check AI service manually in ai_service directory" -ForegroundColor Yellow
    exit 1
  }
}