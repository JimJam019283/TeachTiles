@echo off
echo ==========================================
echo   TeachTiles MIDI Bridge Installer (Windows)
echo ==========================================
echo.

SET PYTHON_PATH=C:\Users\3078565\AppData\Local\Programs\Python\Python312\python.exe

REM Check for Python at known location
if not exist "%PYTHON_PATH%" (
    echo Python not found at %PYTHON_PATH%
    echo.
    echo Please install Python 3.12 from python.org or Microsoft Store
    pause
    exit /b 1
)

echo Found Python:
"%PYTHON_PATH%" --version
echo.

echo Installing required packages...
"%PYTHON_PATH%" -m pip install --upgrade pip
"%PYTHON_PATH%" -m pip install mido python-rtmidi pyserial

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Failed to install packages.
    echo You may need to install Visual C++ Build Tools for python-rtmidi.
    echo Download from: https://visualstudio.microsoft.com/visual-cpp-build-tools/
    pause
    exit /b 1
)

echo.
echo ==========================================
echo   Installation Complete!
echo ==========================================
echo.
echo To run the MIDI bridge, double-click:
echo   scripts\run_midi_bridge.bat
echo.
echo Or for interactive mode:
echo   scripts\run_midi_bridge_interactive.bat
echo.
pause
