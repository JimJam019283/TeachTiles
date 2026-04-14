#!/usr/bin/env python3
"""
MIDI to ESP32 Bridge Daemon
Automatically detects and bridges MIDI input to ESP32 when devices are connected.
Runs continuously in the background, reconnecting as needed.

Works on macOS and Linux/Raspberry Pi.
"""

import argparse
import logging
import os
import sys
import time

import mido
import serial
import serial.tools.list_ports


def default_log_dir():
    if sys.platform == "darwin":
        return os.path.expanduser("~/Library/Logs/TeachTiles")

    state_home = os.environ.get("XDG_STATE_HOME")
    if state_home:
        return os.path.join(state_home, "TeachTiles")
    return os.path.expanduser("~/.local/state/TeachTiles")


def configure_logging(level_name):
    log_dir = default_log_dir()
    os.makedirs(log_dir, exist_ok=True)
    log_file = os.path.join(log_dir, "midi_bridge.log")

    level = getattr(logging, level_name.upper(), logging.INFO)
    logging.basicConfig(
        level=level,
        format="%(asctime)s - %(levelname)s - %(message)s",
        handlers=[
            logging.FileHandler(log_file),
            logging.StreamHandler(sys.stdout),
        ],
    )
    return logging.getLogger(__name__)

# Device detection patterns
MIDI_KEYWORDS = ["keyboard", "piano", "midi", "digital", "yamaha", "roland", "casio"]
ESP32_KEYWORDS = ["usbserial", "cp210", "cp210x", "ch340", "ftdi", "ttyusb", "ttyacm", "esp32", "usb"]

# Baud rate for ESP32 communication
ESP32_BAUD = 115200

# Polling intervals
DEVICE_CHECK_INTERVAL = 2.0  # Check for new devices every 2 seconds
RECONNECT_DELAY = 3.0  # Wait before trying to reconnect

logger = logging.getLogger(__name__)


def parse_args():
    parser = argparse.ArgumentParser(description="TeachTiles MIDI bridge daemon")
    parser.add_argument("--midi-port", help="Exact MIDI input port name to use")
    parser.add_argument("--serial-port", help="Exact ESP32 serial device to use")
    parser.add_argument("--baud", type=int, default=ESP32_BAUD, help="ESP32 serial baud rate")
    parser.add_argument("--log-level", default="INFO", help="Logging level (DEBUG, INFO, WARNING)")
    parser.add_argument("--once", action="store_true", help="Run a single bridge session and exit")
    return parser.parse_args()


def find_keyword_match(entries, keywords):
    for entry in entries:
        entry_lower = entry.lower()
        for keyword in keywords:
            if keyword in entry_lower:
                return entry
    return None


def find_midi_port(preferred_name=None):
    """Find a MIDI input port that looks like a keyboard/piano."""
    try:
        ports = mido.get_input_names()
        if preferred_name:
            for port in ports:
                if port == preferred_name:
                    return port
        match = find_keyword_match(ports, MIDI_KEYWORDS)
        if match:
            return match
        # If no keyword match, return first available port
        if ports:
            return ports[0]
    except Exception as e:
        logger.debug(f"Error finding MIDI port: {e}")
    return None


def find_esp32_port(preferred_name=None):
    """Find a serial port that looks like an ESP32."""
    try:
        ports = list(serial.tools.list_ports.comports())
        if preferred_name:
            for port in ports:
                if port.device == preferred_name:
                    return port.device
        for port in ports:
            port_lower = (port.device + " " + str(port.description)).lower()
            for keyword in ESP32_KEYWORDS:
                if keyword in port_lower:
                    return port.device
        if ports:
            return ports[0].device
    except Exception as e:
        logger.debug(f"Error finding ESP32 port: {e}")
    return None


def forward_midi_message(msg, esp32):
    if msg.type == 'note_on' and msg.velocity > 0:
        midi_bytes = bytes([0x90 | msg.channel, msg.note, msg.velocity])
        esp32.write(midi_bytes)
        logger.debug("Note On forwarded: channel=%d note=%d velocity=%d", msg.channel, msg.note, msg.velocity)
    elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
        midi_bytes = bytes([0x80 | msg.channel, msg.note, 0])
        esp32.write(midi_bytes)
        logger.debug("Note Off forwarded: channel=%d note=%d", msg.channel, msg.note)


def drain_esp32_output(esp32):
    while esp32.in_waiting:
        line = esp32.readline().decode('utf-8', errors='ignore').strip()
        if line:
            logger.info("[ESP32] %s", line)


def run_bridge(midi_port_name, serial_port_name, baud_rate):
    """Run the MIDI to ESP32 bridge. Returns when connection is lost."""
    logger.info("Starting bridge: %s -> %s @ %d", midi_port_name, serial_port_name, baud_rate)
    
    esp32 = None
    midi_in = None
    
    try:
        # Open serial connection to ESP32
        esp32 = serial.Serial(serial_port_name, baud_rate, timeout=0.25)
        time.sleep(2)  # Wait for ESP32 to reset
        
        # Drain startup messages
        drain_esp32_output(esp32)
        
        # Open MIDI input
        midi_in = mido.open_input(midi_port_name)
        
        logger.info("Bridge active - forwarding MIDI to ESP32")
        
        while True:
            # Check for incoming MIDI messages
            for msg in midi_in.iter_pending():
                forward_midi_message(msg, esp32)

            drain_esp32_output(esp32)
            
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
    args = parse_args()
    global logger
    logger = configure_logging(args.log_level)

    logger.info("=" * 50)
    logger.info("TeachTiles MIDI Bridge Daemon starting")
    logger.info("=" * 50)
    
    last_midi_port = None
    last_esp32_port = None
    
    while True:
        try:
            # Look for devices
            midi_port = find_midi_port(args.midi_port)
            esp32_port = find_esp32_port(args.serial_port)
            
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
                run_bridge(midi_port, esp32_port, args.baud)
                if args.once:
                    logger.info("Single bridge run complete")
                    break
                logger.info("Bridge stopped, waiting for reconnection...")
                time.sleep(RECONNECT_DELAY)
            else:
                if args.once:
                    logger.error("Requested --once but could not find both devices")
                    sys.exit(1)
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
