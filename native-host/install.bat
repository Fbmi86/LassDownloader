@echo off
title LassDownloader Native Host Installer
echo ========================================
echo  LassDownloader Native Host Installer
echo ========================================
echo.

set HOST_NAME=com.lassdownloader.native
set INSTALL_DIR=C:\Program Files\LassDownloader
set MANIFEST_FILE=%INSTALL_DIR%\%HOST_NAME%.json

echo 1. Creating installation directory...
mkdir "%INSTALL_DIR%" 2>nul

echo 2. Compiling native host...
if exist "lass_host.c" (
    gcc -O2 -o lass_host.exe lass_host.c -lws2_32
    echo   Compiled successfully!
) else (
    echo   Error: lass_host.c not found!
    pause
    exit /b 1
)

echo 3. Copying files...
copy /Y lass_host.exe "%INSTALL_DIR%\"
copy /Y %HOST_NAME%.json "%INSTALL_DIR%\"

echo 4. Registering for Chrome...
reg add "HKCU\Software\Google\Chrome\NativeMessagingHosts\%HOST_NAME%" /ve /d "%MANIFEST_FILE%" /f

echo 5. Registering for Edge...
reg add "HKCU\Software\Microsoft\Edge\NativeMessagingHosts\%HOST_NAME%" /ve /d "%MANIFEST_FILE%" /f

echo 6. Registering for Firefox...
reg add "HKCU\Software\Mozilla\NativeMessagingHosts\%HOST_NAME%" /ve /d "%MANIFEST_FILE%" /f

echo 7. Registering for Brave...
reg add "HKCU\Software\BraveSoftware\Brave-Browser\NativeMessagingHosts\%HOST_NAME%" /ve /d "%MANIFEST_FILE%" /f 2>nul

echo 8. Registering for Opera...
reg add "HKCU\Software\Opera Software\Opera Stable\NativeMessagingHosts\%HOST_NAME%" /ve /d "%MANIFEST_FILE%" /f 2>nul

echo.
echo ========================================
echo  Installation Complete!
echo ========================================
echo.
echo Installed to: %INSTALL_DIR%
echo.
echo Please restart your browsers.
echo.
pause
