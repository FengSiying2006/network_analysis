@echo off
if not exist "..\build" mkdir "..\build"
echo Building Core Engine...
gcc ..\src\*.c -I ..\include -o ..\build\program.exe
if %errorlevel% neq 0 (
    echo Build Failed!
    exit /b %errorlevel%
)
echo Build Success: build\program.exe