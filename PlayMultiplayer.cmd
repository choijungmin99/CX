@echo off
"C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\ThirdParty\Python3\Win64\python.exe" "%~dp0Scripts\cx.py" play %*
exit /b %errorlevel%
