@echo off
echo ==========================================
echo   TeachTiles MIDI Bridge
echo ==========================================
echo.
echo Starting MIDI bridge daemon...
echo Press Ctrl+C to stop.
echo.

SET PYTHON_PATH=C:\Users\3078565\AppData\Local\Programs\Python\Python312\python.exe

cd /d "%~dp0\.."
"%PYTHON_PATH%" scripts\midi_bridge_daemon.py --log-level INFO

pause
