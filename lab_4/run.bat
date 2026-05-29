@echo off
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting administrator privileges...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit
)
echo Running as administrator...
cd /d "%~dp0"

if not exist "temp" mkdir "temp"

echo Starting Writers...
start "Writer 1" Writer.exe
start "Writer 2" Writer.exe
start "Writer 3" Writer.exe
start "Writer 4" Writer.exe
start "Writer 5" Writer.exe

echo Starting Readers...
start "Reader 0" Reader.exe 0
start "Reader 1" Reader.exe 1
start "Reader 2" Reader.exe 2
start "Reader 3" Reader.exe 3
start "Reader 4" Reader.exe 4

echo.
echo All processes started. Check 'temp\' folder for logs.
pause