@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Enable-VR.ps1"
if errorlevel 1 (
 echo.
 echo The operation did not complete. Read the message above.
 pause
 exit /b 1
)
pause
