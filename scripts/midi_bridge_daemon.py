#!/usr/bin/env python3
"""
MIDI to ESP32 Bridge Daemon
Automatically detects and bridges MIDI input to ESP32 when devices are connected.
Runs continuously in the background, reconnecting as needed.

Install as a launch agent for automatic startup.
"""

import mido
import serial
import serial.tools.list_ports
import time
import sys
import os
import logging
from datetime import datetime

# Configure logging
LOG_DIR = os.path.expanduser("~/Library/Logs/TeachTiles")
os.makedirs(LOG_DIR, exist_ok=True)
LOG_FILE = os.path.join(LOG_DIR, "midi_bridge.log")

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler(LOG_FILE),
        logging.StreamHandler(sys.stdout)
    ]
)
logger = logging.getLogger(__name__)

# Device detection patterns
MIDI_KEYWORDS = ["keyboard", "piano", "midi", "digital"]
ESP32_KEYWORDS = ["usbserial", "cp210", "ch340", "ftdi", "usb"]

# Baud rate for ESP32 communication
ESP32_BAUD = 115200

# Polling intervals
DEVICE_CHECK_INTERVAL = 2.0  # Check for new devices every 2 seconds
RECONNECT_DELAY = 3.0  # Wait before trying to reconnect


def find_midi_port():
    """Find a MIDI input port that looks like a keyboard/piano."""
    try:
        ports = mido.get_input_names()
        for port in ports:
            port_lower = port.lower()
            for keyword in MIDI_KEYWORDS:
                if keyword in port_lower:
                    return port
        # If no keyword match, return first available port
        if ports:
            return ports[0]
    except Exception as e:
        logger.debug(f"Error finding MIDI port: {e}")
    return None


def find_esp32_port():
    """Find a serial port that looks like an ESP32."""
    try:
        for port in serial.tools.list_ports.comports():
            port_lower = (port.device + " " + str(port.description)).lower()
            for keyword in ESP32_KEYWORDS:
                if keyword in port_lower:
                    return port.device
    except Exception as e:
        logger.debug(f"Error finding ESP32 port: {e}")
    return None


def run_bridge(midi_port_name, serial_port_name):
    """Run the MIDI to ESP32 bridge. Returns when connection is lost."""
    logger.info(f"Starting bridge: {midi_port_name} -> {serial_port_name}")
    
    esp32 = None
    midi_in = None
    
    try:
        # Open serial connection to ESP32
        esp32 = serial.Serial(serial_port_name, ESP32_BAUD, timeout=1)
        time.sleep(2)  # Wait for ESP32 to reset
        
        # Drain startup messages
        while esp32.in_waiting:
            esp32.read(esp32.in_waiting)
        
        # Open MIDI input
        midi_in = mido.open_input(midi_port_name)
        
        logger.info("Bridge active - forwarding MIDI to ESP32")
        
        note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
        
        while True:
            # Check for incoming MIDI messages
            for msg in midi_in.iter_pending():
                if msg.type == 'note_on' and msg.velocity > 0:
                    # Send raw MIDI bytes to ESP32
                    midi_bytes = bytes([0x90 | msg.channel, msg.note, msg.velocity])
                    esp32.write(midi_bytes)
                    
                    note_name = note_names[msg.note % 12]
                    octave = (msg.note // 12) - 1
                    logger.debug(f"Note On: {note_name}{octave} (MIDI {msg.note})")
                
                elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
                    midi_bytes = bytes([0x80 | msg.channel, msg.note, 0])
                    esp32.write(midi_bytes)
            
            # Check if devices are still connected
            if not os.path.exists(serial_port_name):
                logger.warning("ESP32 disconnected")
                return
            
            time.sleep(0.001)
            
    except serial.SerialException as e:
        logger.warning(f"Serial error: {e}")
    except Exception as e:
        logger.warning(f"Bridge error: {e}")
    finally:
        try:
            if esp32:
                esp32.close()
            if midi_in:
                midi_in.close()
        except:
            pass


def main():
    logger.info("=" * 50)
    logger.info("TeachTiles MIDI Bridge Daemon starting")
    logger.info("=" * 50)
    
    last_midi_port = None
    last_esp32_port = None
    
    while True:
        try:
            # Look for devices
            midi_port = find_midi_port()
            esp32_port = find_esp32_port()
            
            # Log when devices appear/disappear
            if midi_port != last_midi_port:
                if midi_port:
                    logger.info(f"MIDI device found: {midi_port}")
                elif last_midi_port:
                    logger.info("MIDI device disconnected")
                last_midi_port = midi_port
            
            if esp32_port != last_esp32_port:
                if esp32_port:
                    logger.info(f"ESP32 found: {esp32_port}")
                elif last_esp32_port:
                    logger.info("ESP32 disconnected")
                last_esp32_port = esp32_port
            
            # If both devices are connected, run the bridge
            if midi_port and esp32_port:
                run_bridge(midi_port, esp32_port)
                logger.info("Bridge stopped, waiting for reconnection...")
                time.sleep(RECONNECT_DELAY)
            else:
                # Wait and check again
                time.sleep(DEVICE_CHECK_INTERVAL)
                
        except KeyboardInterrupt:
            logger.info("Daemon stopped by user")
            break
        except Exception as e:
            logger.error(f"Unexpected error: {e}")
            time.sleep(RECONNECT_DELAY)


if __name__ == "__main__":
    main()
