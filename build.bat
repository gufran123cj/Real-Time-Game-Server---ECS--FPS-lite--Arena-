@echo off
echo ========================================
echo Game Server Build Script
echo ========================================
echo.

REM MinGW path kontrol
set MINGW_PATH=D:\MinGW\bin
if not exist "%MINGW_PATH%\g++.exe" (
    echo HATA: g++ bulunamadi!
    echo MinGW path'ini kontrol edin: %MINGW_PATH%
    pause
    exit /b 1
)

echo Derleme basladi...
echo.

REM Tüm kaynak dosyalarını derle
REM MinGW header uyumluluk flag'leri
"%MINGW_PATH%\g++.exe" ^
    -std=c++20 ^
    -O3 ^
    -Wall ^
    -Wextra ^
    -D_MINGW_EXTENSION=__extension__ ^
    -D__USE_MINGW_ANSI_STDIO=0 ^
    -I. ^
    -Iinclude ^
    -Iecs ^
    -Inet ^
    -Iphysics ^
    -Imatchmaker ^
    -Ianti-cheat-lite ^
    src\main.cpp ^
    src\Server.cpp ^
    ecs\Component.cpp ^
    ecs\World.cpp ^
    net\Socket.cpp ^
    net\Snapshot.cpp ^
    physics\Physics.cpp ^
    matchmaker\Matchmaker.cpp ^
    anti-cheat-lite\AntiCheat.cpp ^
    -o GameServer.exe ^
    -lws2_32

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Derleme BASARILI!
    echo ========================================
    echo.
    echo Calistirmak icin: GameServer.exe
    echo Veya: GameServer.exe 7777 60
    echo.
) else (
    echo.
    echo ========================================
    echo Derleme BASARISIZ!
    echo ========================================
    echo.
)

pause

