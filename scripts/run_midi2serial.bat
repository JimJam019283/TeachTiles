@echo off
echo ==========================================
echo   TeachTiles MIDI to Serial Bridge
echo   (No WiFi Required!)
echo ==========================================
echo.

SET PYTHON_PATH=C:\Users\3078565\AppData\Local\Programs\Python\Python312\python.exe

cd /d "%~dp0\.."

echo Connect your piano via USB MIDI dongle
echo Connect TeachTiles ESP32 via USB cable
echo.

"%PYTHON_PATH%" scripts\midi2serial.py %*

pause
