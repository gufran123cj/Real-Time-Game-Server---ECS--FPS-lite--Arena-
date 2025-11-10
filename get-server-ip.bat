@echo off
setlocal enabledelayedexpansion
echo ========================================
echo Game Server IP Adresi Bulucu
echo ========================================
echo.
echo Server'iniz bu IP adreslerinde dinliyor:
echo.

for /f "tokens=2 delims=:" %%a in ('ipconfig ^| findstr /i "IPv4"') do (
    set ip=%%a
    set ip=!ip:~1!
    echo   - !ip!:7777
)

echo.
echo Client'larda bu IP'yi kullanin:
echo   GameClient.exe [IP_ADRESI] 7777
echo.
echo Ornek:
echo   GameClient.exe 192.168.1.100 7777
echo.
pause

