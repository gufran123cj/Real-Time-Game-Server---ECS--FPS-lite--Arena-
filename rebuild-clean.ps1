# Clean Build Script - Build klasorunu temizleyip yeniden derler
# Kullanim: .\rebuild-clean.ps1

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ProjectRoot "build"

Write-Host "Clean Build Baslatiliyor" -ForegroundColor Cyan
Write-Host "========================" -ForegroundColor Cyan

# Build klasorunu temizle
if (Test-Path $BuildDir) {
    Write-Host "[*] Build klasoru temizleniyor..." -ForegroundColor Yellow
    Remove-Item -Path $BuildDir -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "[OK] Build klasoru temizlendi" -ForegroundColor Green
} else {
    Write-Host "[OK] Build klasoru zaten temiz" -ForegroundColor Green
}

# Yeni build yap
Write-Host ""
Write-Host "[*] Yeni build baslatiliyor..." -ForegroundColor Yellow
& "$ProjectRoot\build.ps1"

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "[OK] Clean build basarili!" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "[HATA] Build basarisiz!" -ForegroundColor Red
    exit 1
}

