@echo off
setlocal enabledelayedexpansion
echo ========================================
echo Game Server Baslatiliyor...
echo ========================================
echo.

REM Server IP'sini goster
echo Server IP Adresleri:
for /f "tokens=2 delims=:" %%a in ('ipconfig ^| findstr /i "IPv4"') do (
    set ip=%%a
    set ip=!ip:~1!
    echo   - !ip!:7777
)

echo.
echo Client'larda bu IP'yi kullanin!
echo.
echo Server baslatiliyor...
echo.

REM Server'i baslat
GameServer.exe

pause

