#!/usr/bin/env python3
"""
MIDI to ESP32 Bridge
Reads MIDI from a USB dongle and forwards Note On messages to ESP32 via serial.

Usage:
  python3 midi_to_esp32.py

Requirements:
  pip3 install mido python-rtmidi pyserial
"""

import mido
import serial
import serial.tools.list_ports
import sys
import time

# ESP32 serial settings
ESP32_BAUD = 115200  # Match ESP32's Serial.begin() rate for debug output
MIDI_BAUD = 31250    # Standard MIDI baud rate (what ESP32 expects on Serial2)

def list_midi_ports():
    """List available MIDI input ports."""
    print("\nAvailable MIDI input ports:")
    ports = mido.get_input_names()
    if not ports:
        print("  (none found)")
        return []
    for i, port in enumerate(ports):
        print(f"  [{i}] {port}")
    return ports

def list_serial_ports():
    """List available serial ports."""
    print("\nAvailable serial ports:")
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("  (none found)")
        return []
    for i, port in enumerate(ports):
        print(f"  [{i}] {port.device} - {port.description}")
    return ports

def select_midi_port():
    """Let user select a MIDI input port."""
    ports = list_midi_ports()
    if not ports:
        print("\nNo MIDI ports found! Make sure your MIDI dongle is connected.")
        sys.exit(1)
    
    if len(ports) == 1:
        print(f"\nAuto-selecting: {ports[0]}")
        return ports[0]
    
    while True:
        try:
            choice = input("\nSelect MIDI port number: ")
            idx = int(choice)
            if 0 <= idx < len(ports):
                return ports[idx]
        except (ValueError, IndexError):
            pass
        print("Invalid selection, try again.")

def select_serial_port():
    """Let user select a serial port for ESP32."""
    ports = list_serial_ports()
    if not ports:
        print("\nNo serial ports found! Make sure your ESP32 is connected.")
        sys.exit(1)
    
    # Try to auto-detect ESP32
    for port in ports:
        if 'usbserial' in port.device.lower() or 'usb' in port.device.lower():
            print(f"\nAuto-selecting ESP32: {port.device}")
            return port.device
    
    if len(ports) == 1:
        print(f"\nAuto-selecting: {ports[0].device}")
        return ports[0].device
    
    while True:
        try:
            choice = input("\nSelect serial port number: ")
            idx = int(choice)
            if 0 <= idx < len(ports):
                return ports[idx].device
        except (ValueError, IndexError):
            pass
        print("Invalid selection, try again.")

def main():
    print("=" * 50)
    print("   MIDI to ESP32 Bridge for TeachTiles")
    print("=" * 50)
    
    # Select ports
    midi_port_name = select_midi_port()
    serial_port_name = select_serial_port()
    
    print(f"\nConnecting...")
    print(f"  MIDI: {midi_port_name}")
    print(f"  Serial: {serial_port_name}")
    
    try:
        # Open serial connection to ESP32
        esp32 = serial.Serial(serial_port_name, ESP32_BAUD, timeout=1)
        time.sleep(2)  # Wait for ESP32 to reset after serial connection
        
        # Drain any startup messages
        while esp32.in_waiting:
            line = esp32.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"  ESP32: {line}")
        
        # Open MIDI input
        midi_in = mido.open_input(midi_port_name)
        
        print("\n" + "=" * 50)
        print("   Bridge active! Play your piano...")
        print("   Press Ctrl+C to stop")
        print("=" * 50 + "\n")
        
        note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
        
        while True:
            # Check for incoming MIDI messages (non-blocking)
            for msg in midi_in.iter_pending():
                if msg.type == 'note_on' and msg.velocity > 0:
                    # Send raw MIDI bytes to ESP32
                    # Note On = 0x90 + channel, note, velocity
                    midi_bytes = bytes([0x90 | msg.channel, msg.note, msg.velocity])
                    esp32.write(midi_bytes)
                    
                    # Display note info
                    note_name = note_names[msg.note % 12]
                    octave = (msg.note // 12) - 1
                    print(f"♪ Note On:  {note_name}{octave} (MIDI {msg.note}) vel={msg.velocity}")
                
                elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
                    # Send Note Off to ESP32
                    midi_bytes = bytes([0x80 | msg.channel, msg.note, 0])
                    esp32.write(midi_bytes)
                    
                    note_name = note_names[msg.note % 12]
                    octave = (msg.note // 12) - 1
                    print(f"  Note Off: {note_name}{octave} (MIDI {msg.note})")
            
            # Check for ESP32 debug output
            while esp32.in_waiting:
                line = esp32.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"  [ESP32] {line}")
            
            time.sleep(0.001)  # Small delay to prevent CPU spinning
            
    except serial.SerialException as e:
        print(f"\nSerial error: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n\nBridge stopped.")
    finally:
        try:
            esp32.close()
            midi_in.close()
        except:
            pass

if __name__ == "__main__":
    main()
