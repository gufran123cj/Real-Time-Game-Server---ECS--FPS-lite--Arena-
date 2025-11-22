# GameServer Run Script (Sadece calistir)
# Kullanim: .\run.ps1

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$ReleaseDir = Join-Path $ProjectRoot "build\Release"
$ExePath = Join-Path $ReleaseDir "GameServer.exe"

Write-Host "GameServer Calistiriliyor" -ForegroundColor Cyan
Write-Host "========================" -ForegroundColor Cyan

if (Test-Path $ExePath) {
    Write-Host "[OK] GameServer.exe bulundu" -ForegroundColor Green
    Write-Host "[*] Calisma dizini: $ProjectRoot" -ForegroundColor Yellow
    # Proje root'undan calistir (assets klasoru burada)
    Set-Location $ProjectRoot
    
    # Exe'yi calistir ve cikis kodunu yakala
    # Stderr ve stdout'u yakalamak icin Start-Process kullan
    Write-Host "[*] GameServer.exe calistiriliyor..." -ForegroundColor Yellow
    Write-Host ""
    
    try {
        $process = Start-Process -FilePath $ExePath -NoNewWindow -Wait -PassThru -RedirectStandardOutput "server_output.txt" -RedirectStandardError "server_error.txt"
        $exitCode = $process.ExitCode
        
        # Cikti dosyalarini oku
        if (Test-Path "server_output.txt") {
            $output = Get-Content "server_output.txt" -Raw
            if ($output) {
                Write-Host $output
            }
        }
        
        if (Test-Path "server_error.txt") {
            $error = Get-Content "server_error.txt" -Raw
            if ($error) {
                Write-Host $error -ForegroundColor Red
            }
        }
        
        if ($exitCode -ne 0) {
            Write-Host "" -ForegroundColor Red
            Write-Host "[HATA] GameServer.exe hata ile kapandi (Exit Code: $exitCode)" -ForegroundColor Red
            
            # Exit code aciklamasi
            if ($exitCode -eq -1073740791) {
                Write-Host "[*] Bu hata genellikle su anlama gelir:" -ForegroundColor Yellow
                Write-Host "    - Eksik DLL dosyasi (SFML veya baska bir kutuphane)" -ForegroundColor Yellow
                Write-Host "    - Bellek erisim hatasi (uninitialized pointer)" -ForegroundColor Yellow
                Write-Host "    - Stack buffer overflow" -ForegroundColor Yellow
                Write-Host ""
                Write-Host "[*] Cozum onerileri:" -ForegroundColor Cyan
                Write-Host "    1. build\Release klasorunde DLL dosyalarini kontrol edin" -ForegroundColor Cyan
                Write-Host "    2. GameServer.exe'yi direkt calistirip Windows hata mesajini gorebilirsiniz" -ForegroundColor Cyan
                Write-Host "    3. Visual Studio ile debug modda calistirin" -ForegroundColor Cyan
                Write-Host "    4. Temiz build yapin: .\rebuild-clean.ps1" -ForegroundColor Cyan
            }
            
            # Gecici dosyalari temizle
            if (Test-Path "server_output.txt") { Remove-Item "server_output.txt" -ErrorAction SilentlyContinue }
            if (Test-Path "server_error.txt") { Remove-Item "server_error.txt" -ErrorAction SilentlyContinue }
            
            exit $exitCode
        }
        
        # Gecici dosyalari temizle
        if (Test-Path "server_output.txt") { Remove-Item "server_output.txt" -ErrorAction SilentlyContinue }
        if (Test-Path "server_error.txt") { Remove-Item "server_error.txt" -ErrorAction SilentlyContinue }
        
    } catch {
        Write-Host "[HATA] GameServer.exe calistirilirken exception olustu:" -ForegroundColor Red
        Write-Host $_.Exception.Message -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "[HATA] GameServer.exe bulunamadi!" -ForegroundColor Red
    Write-Host "[*] Once derleme yapin: .\build.ps1" -ForegroundColor Yellow
    exit 1
}

Set-Location $ProjectRoot

