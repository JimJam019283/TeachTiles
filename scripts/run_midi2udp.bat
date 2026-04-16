@echo off
echo ==========================================
echo   TeachTiles MIDI to UDP Bridge
echo ==========================================
echo.

SET PYTHON_PATH=C:\Users\3078565\AppData\Local\Programs\Python\Python312\python.exe

cd /d "%~dp0\.."

echo Starting MIDI to UDP bridge...
echo This sends piano notes over WiFi to TeachTiles.
echo.
echo Press Ctrl+C to stop.
echo.

"%PYTHON_PATH%" scripts\midi2udp.py %*

pause
