@echo off
setlocal
cd /d "%~dp0"
if /i "%VSCMD_ARG_TGT_ARCH%"=="x64" goto build
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto missingvs
for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT goto missingvs
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto failed
:build
where py >nul 2>nul
if errorlevel 1 goto python
py -3 tools\build.py %*
goto result
:python
python tools\build.py %*
:result
if errorlevel 1 goto failed
echo Build finished. See the README for output locations.
pause
exit /b 0
:missingvs
echo Install Visual Studio 2022 with Desktop development with C++ and the Windows SDK.
:failed
echo Build did not complete. Read the error above and README.md.
pause
exit /b 1
