@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup-wroom-ap.ps1" %*
exit /b %ERRORLEVEL%
