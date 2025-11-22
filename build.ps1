# GameServer Build Script
# Kullanim: .\build.ps1

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ProjectRoot "build"
$ReleaseDir = Join-Path $BuildDir "Release"

Write-Host "GameServer Build Script" -ForegroundColor Cyan
Write-Host "=======================" -ForegroundColor Cyan
Write-Host "[*] NOT: Bu script sadece derleme yapar, build/_deps degisikliklerini korur" -ForegroundColor Gray

# CMake generator uyumluluk kontrolu
$CMakeCachePath = Join-Path $BuildDir "CMakeCache.txt"
if (Test-Path $CMakeCachePath) {
    $cacheContent = Get-Content $CMakeCachePath -Raw
    # Generator kontrolu - Visual Studio olmayan generator'lar
    $isMinGW = $cacheContent -match "CMAKE_GENERATOR:INTERNAL=MinGW Makefiles"
    $isVisualStudio = $cacheContent -match "CMAKE_GENERATOR:INTERNAL=Visual Studio"
    
    if ($isMinGW -or (-not $isVisualStudio)) {
        Write-Host "[UYARI] Build klasoru farkli generator ile olusturulmus!" -ForegroundColor Yellow
        if ($isMinGW) {
            Write-Host "[*] Tespit edilen: MinGW Makefiles" -ForegroundColor Yellow
        } else {
            Write-Host "[*] Visual Studio generator bulunamadi" -ForegroundColor Yellow
        }
        Write-Host "[*] Visual Studio generator ile uyumsuz." -ForegroundColor Yellow
        Write-Host "[!] ONEMLI: Temizleme yapilirsa build/_deps klasorundeki degisiklikler kaybolur!" -ForegroundColor Red
        Write-Host "[*] Cozum: .\rebuild.ps1 calistirin (temiz derleme)" -ForegroundColor Cyan
        Write-Host ""
        $choice = Read-Host "Simdi otomatik temizleyip yeniden configure edilsin mi? (E/H)"
        if ($choice -eq "E" -or $choice -eq "e") {
            Write-Host "[!] UYARI: build/_deps klasorundeki tum degisiklikler kaybolacak!" -ForegroundColor Red
            $confirm = Read-Host "Emin misiniz? (E/H)"
            if ($confirm -eq "E" -or $confirm -eq "e") {
                Write-Host "[*] Build klasoru temizleniyor..." -ForegroundColor Yellow
                Remove-Item -Path $BuildDir -Recurse -Force -ErrorAction SilentlyContinue
                New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
                
                Write-Host "[*] CMake yeniden configure ediliyor..." -ForegroundColor Yellow
                Set-Location $BuildDir
                cmake .. -G "Visual Studio 17 2022" -A x64
                
                if ($LASTEXITCODE -ne 0) {
                    Write-Host "[HATA] CMake configure basarisiz!" -ForegroundColor Red
                    exit 1
                }
                Write-Host "[OK] CMake configure basarili!" -ForegroundColor Green
            } else {
                Write-Host "[*] Temizleme iptal edildi. Sadece derleme yapiliyor..." -ForegroundColor Yellow
            }
        } else {
            Write-Host "[*] Temizleme atlandi. Sadece derleme yapiliyor..." -ForegroundColor Yellow
        }
    }
}

# Visual Studio kontrolu (CMake configure icin gerekli)
$VSWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$generator = "Visual Studio 17 2022"
if (Test-Path $VSWhere) {
    $vsVersion = & $VSWhere -latest -property catalog_productLineVersion
    if ($vsVersion -eq "2019") {
        $generator = "Visual Studio 16 2019"
    } elseif ($vsVersion -eq "2017") {
        $generator = "Visual Studio 15 2017"
    } elseif ($vsVersion -eq "2022") {
        $generator = "Visual Studio 17 2022"
    }
    Write-Host "[*] Tespit edilen Visual Studio: $vsVersion" -ForegroundColor Cyan
}

# Build klasoru kontrolu ve olusturma
if (-not (Test-Path $BuildDir)) {
    Write-Host "[*] Build klasoru bulunamadi, olusturuluyor..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
    Write-Host "[OK] Build klasoru olusturuldu" -ForegroundColor Green
}

# Build klasorune git
Set-Location $BuildDir

# Solution dosyasi kontrolu
$SolutionPath = Join-Path $BuildDir "GameServer.sln"
if (-not (Test-Path $SolutionPath)) {
    Write-Host "[UYARI] Solution dosyasi bulunamadi!" -ForegroundColor Yellow
    Write-Host "[*] CMake configure yapiliyor..." -ForegroundColor Yellow
    
    Write-Host "[*] CMake configure ediliyor (Generator: $generator)..." -ForegroundColor Yellow
    cmake .. -G $generator -A x64
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[HATA] CMake configure basarisiz!" -ForegroundColor Red
        Write-Host "[*] Manuel olarak CMake configure yapmayi deneyin:" -ForegroundColor Yellow
        Write-Host "    cd build" -ForegroundColor Gray
        Write-Host "    cmake .. -G `"$generator`" -A x64" -ForegroundColor Gray
        exit 1
    }
    Write-Host "[OK] CMake configure basarili!" -ForegroundColor Green
}

# Visual Studio Developer Command Prompt'u bul (MSBuild icin)
if (Test-Path $VSWhere) {
    $VSInstallPath = & $VSWhere -latest -property installationPath
    $MSBuildPath = Join-Path $VSInstallPath "MSBuild\Current\Bin\MSBuild.exe"
    
    if (Test-Path $MSBuildPath) {
        Write-Host "[OK] MSBuild bulundu: $MSBuildPath" -ForegroundColor Green
        Write-Host "[*] Derleme baslatiliyor..." -ForegroundColor Yellow
        
        & $MSBuildPath "GameServer.sln" /p:Configuration=Release /m /v:minimal
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[OK] Derleme basarili!" -ForegroundColor Green
            Write-Host "[*] Executable: $ReleaseDir\GameServer.exe" -ForegroundColor Cyan
            
            # Calistirmak ister misiniz?
            $run = Read-Host "Programi calistirmak ister misiniz? (E/H)"
            if ($run -eq "E" -or $run -eq "e") {
                Set-Location $ReleaseDir
                & ".\GameServer.exe"
            }
        } else {
            Write-Host "[HATA] Derleme basarisiz!" -ForegroundColor Red
            exit 1
        }
    } else {
        Write-Host "[HATA] MSBuild bulunamadi!" -ForegroundColor Red
        Write-Host "[*] Visual Studio'yu yukleyin veya Developer Command Prompt kullanin" -ForegroundColor Yellow
        exit 1
    }
} else {
    Write-Host "[HATA] Visual Studio bulunamadi!" -ForegroundColor Red
    Write-Host "[*] Visual Studio'yu yukleyin" -ForegroundColor Yellow
    exit 1
}

Set-Location $ProjectRoot

