@echo off
setlocal
set "CX_EDITOR=C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%CX_EDITOR%" (
    echo Unreal Engine 5.5 was not found. Update CX_EDITOR in PlayMultiplayer.cmd.
    exit /b 1
)
if not exist "%~dp0Binaries\Win64\UnrealEditor-CX.dll" (
    call "%~dp0Build.cmd"
    if errorlevel 1 exit /b 1
)
start "CX Host" "%CX_EDITOR%" "%~dp0CX.uproject" "/Game/Maps/CXMiningArena?listen" -game -windowed -ResX=960 -ResY=600 -WinX=0 -WinY=20 -port=7777 -NoSplash "-abslog=%~dp0Saved\Logs\HostPlay.log" %*
start "CX Client" "%CX_EDITOR%" "%~dp0CX.uproject" 127.0.0.1:7777 -game -windowed -ResX=960 -ResY=600 -WinX=960 -WinY=20 -NoSplash "-abslog=%~dp0Saved\Logs\ClientPlay.log" %*
exit /b 0
