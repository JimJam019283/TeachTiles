#!/usr/bin/env python3
"""
MIDI to Serial Bridge for TeachTiles (Windows/Mac/Linux)
Reads MIDI from USB dongle and sends directly to ESP32 via USB serial.
No WiFi required!

Usage:
  python midi2serial.py [--port COMPORT]
"""

import argparse
import time
import mido
import serial
import serial.tools.list_ports

def find_esp32_port():
    """Auto-detect ESP32 serial port."""
    ports = list(serial.tools.list_ports.comports())
    keywords = ['cp210', 'ch340', 'ftdi', 'usb', 'serial', 'esp']
    
    for port in ports:
        desc = (port.device + " " + str(port.description)).lower()
        for kw in keywords:
            if kw in desc:
                return port.device
    
    # Return first available if no keyword match
    if ports:
        return ports[0].device
    return None

def main():
    parser = argparse.ArgumentParser(description='MIDI to Serial bridge for TeachTiles')
    parser.add_argument('--port', default='', help='ESP32 serial port (e.g., COM3)')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('--midi', default='', help='MIDI device name or index')
    args = parser.parse_args()

    print("=" * 50)
    print("  TeachTiles MIDI -> Serial Bridge")
    print("  No WiFi Required!")
    print("=" * 50)
    print()

    # Find MIDI port
    midi_ports = mido.get_input_names()
    if not midi_ports:
        print("ERROR: No MIDI input ports found!")
        print("Make sure your MIDI keyboard/dongle is connected.")
        input("Press Enter to exit...")
        return 1
    
    print("Available MIDI ports:")
    for i, port in enumerate(midi_ports):
        print(f"  [{i}] {port}")
    
    # Select MIDI port
    midi_port_name = None
    if args.midi:
        try:
            idx = int(args.midi)
            if 0 <= idx < len(midi_ports):
                midi_port_name = midi_ports[idx]
        except ValueError:
            for p in midi_ports:
                if args.midi.lower() in p.lower():
                    midi_port_name = p
                    break
    
    if not midi_port_name:
        keywords = ['keyboard', 'piano', 'midi', 'digital', 'yamaha', 'roland', 'casio']
        for p in midi_ports:
            if any(k in p.lower() for k in keywords):
                midi_port_name = p
                break
        if not midi_port_name:
            midi_port_name = midi_ports[0]
    
    print(f"\n-> Using MIDI: {midi_port_name}")

    # Find serial port
    serial_ports = list(serial.tools.list_ports.comports())
    print("\nAvailable serial ports:")
    if not serial_ports:
        print("  (none found)")
        print("\nERROR: No serial ports found!")
        print("Make sure ESP32 is connected via USB.")
        input("Press Enter to exit...")
        return 1
    
    for i, port in enumerate(serial_ports):
        print(f"  [{i}] {port.device} - {port.description}")
    
    serial_port = args.port if args.port else find_esp32_port()
    if not serial_port:
        print("\nERROR: Could not find ESP32 serial port!")
        input("Press Enter to exit...")
        return 1
    
    print(f"\n-> Using Serial: {serial_port} @ {args.baud} baud")

    # Open connections
    print("\nConnecting...")
    try:
        esp32 = serial.Serial(serial_port, args.baud, timeout=0.1)
        time.sleep(2)  # Wait for ESP32 reset
        
        # Drain startup messages
        while esp32.in_waiting:
            line = esp32.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"  [ESP32] {line}")
        
        midi_in = mido.open_input(midi_port_name)
        
    except Exception as e:
        print(f"\nERROR: {e}")
        input("Press Enter to exit...")
        return 1

    print("\n" + "=" * 50)
    print("  Bridge Active! Play your piano...")
    print("  Press Ctrl+C to stop")
    print("=" * 50 + "\n")
    
    note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
    
    try:
        while True:
            # Check for MIDI messages
            for msg in midi_in.iter_pending():
                if msg.type == 'note_on' and msg.velocity > 0:
                    # Send Note ON: 0x90 + channel, note, velocity
                    midi_bytes = bytes([0x90 | msg.channel, msg.note, msg.velocity])
                    esp32.write(midi_bytes)
                    
                    note = note_names[msg.note % 12]
                    octave = (msg.note // 12) - 1
                    print(f"♪ Note ON:  {note}{octave} (MIDI {msg.note}) vel={msg.velocity}")
                
                elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
                    # Send Note OFF: 0x80 + channel, note, 0
                    midi_bytes = bytes([0x80 | msg.channel, msg.note, 0])
                    esp32.write(midi_bytes)
                    
                    note = note_names[msg.note % 12]
                    octave = (msg.note // 12) - 1
                    print(f"  Note OFF: {note}{octave} (MIDI {msg.note})")
            
            # Check for ESP32 output
            while esp32.in_waiting:
                line = esp32.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"  [ESP32] {line}")
            
            time.sleep(0.001)  # Small delay to prevent CPU spin
                    
    except KeyboardInterrupt:
        print("\n\nBridge stopped.")
    finally:
        esp32.close()
        midi_in.close()
    
    return 0

if __name__ == '__main__':
    exit(main())
