# GameServer Rebuild Script (Temiz derleme)
# Kullanim: .\rebuild.ps1

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ProjectRoot "build"

Write-Host "GameServer Rebuild Script (Temiz Derleme)" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# Build klasorunu temizle
if (Test-Path $BuildDir) {
    Write-Host "[*] Build klasoru temizleniyor..." -ForegroundColor Yellow
    Remove-Item -Path $BuildDir -Recurse -Force
}

# Yeni build klasoru olustur
Write-Host "[*] Yeni build klasoru olusturuluyor..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null

# CMake configure
Write-Host "[*] CMake configure ediliyor..." -ForegroundColor Yellow
Set-Location $BuildDir
cmake .. -G "Visual Studio 17 2022" -A x64

if ($LASTEXITCODE -ne 0) {
    Write-Host "[HATA] CMake configure basarisiz!" -ForegroundColor Red
    exit 1
}

# Build script'ini calistir
Write-Host "[*] Derleme baslatiliyor..." -ForegroundColor Yellow
& (Join-Path $ProjectRoot "build.ps1")

Set-Location $ProjectRoot

