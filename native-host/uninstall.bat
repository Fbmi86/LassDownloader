@echo off
title LassDownloader Native Host Uninstaller
echo Removing LassDownloader Native Host...

set HOST_NAME=com.lassdownloader.native

reg delete "HKCU\Software\Google\Chrome\NativeMessagingHosts\%HOST_NAME%" /f 2>nul
reg delete "HKCU\Software\Microsoft\Edge\NativeMessagingHosts\%HOST_NAME%" /f 2>nul
reg delete "HKCU\Software\Mozilla\NativeMessagingHosts\%HOST_NAME%" /f 2>nul
reg delete "HKCU\Software\BraveSoftware\Brave-Browser\NativeMessagingHosts\%HOST_NAME%" /f 2>nul
reg delete "HKCU\Software\Opera Software\Opera Stable\NativeMessagingHosts\%HOST_NAME%" /f 2>nul

echo Uninstall complete!
pause
