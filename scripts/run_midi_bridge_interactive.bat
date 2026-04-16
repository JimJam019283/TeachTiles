@echo off
echo ==========================================
echo   TeachTiles MIDI Bridge (Interactive)
echo ==========================================
echo.

SET PYTHON_PATH=C:\Users\3078565\AppData\Local\Programs\Python\Python312\python.exe

cd /d "%~dp0\.."
"%PYTHON_PATH%" scripts\midi_to_esp32.py

pause
