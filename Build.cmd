@echo off
setlocal
set "CX_BUILD=C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat"
if not exist "%CX_BUILD%" (
    echo Unreal Engine 5.5 was not found. Update CX_BUILD in Build.cmd.
    exit /b 1
)
call "%CX_BUILD%" CXEditor Win64 Development "-Project=%~dp0CX.uproject" -WaitMutex -NoHotReloadFromIDE %*
exit /b %errorlevel%
