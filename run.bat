@echo off
echo ========================================
echo Game Server Baslatiliyor...
echo ========================================
echo.

if not exist "GameServer.exe" (
    echo HATA: GameServer.exe bulunamadi!
    echo Once build.bat ile derleyin.
    pause
    exit /b 1
)

REM Port ve tick rate parametreleri
set PORT=7777
set TICK_RATE=60

if not "%1"=="" set PORT=%1
if not "%2"=="" set TICK_RATE=%2

echo Port: %PORT%
echo Tick Rate: %TICK_RATE%
echo.
echo Sunucu calisiyor... (Ctrl+C ile durdurun)
echo.

GameServer.exe %PORT% %TICK_RATE%

pause

