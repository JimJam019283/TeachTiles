#!/usr/bin/env python3
"""
MIDI to UDP Bridge for TeachTiles (Windows/Mac/Linux)
Reads MIDI from USB dongle and sends UDP packets to ESP32.

Usage:
  python midi2udp.py [--addr IP] [--port PORT]

Default broadcasts to 255.255.255.255:5005
"""

import argparse
import socket
import time
import mido

def main():
    parser = argparse.ArgumentParser(description='MIDI to UDP bridge for TeachTiles')
    parser.add_argument('--addr', default='255.255.255.255', help='ESP32 IP or broadcast (default: 255.255.255.255)')
    parser.add_argument('--port', type=int, default=5005, help='UDP port (default: 5005)')
    parser.add_argument('--device', default='', help='MIDI device name or index')
    args = parser.parse_args()

    # Setup UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    
    dest = (args.addr, args.port)
    
    # Find MIDI port
    ports = mido.get_input_names()
    if not ports:
        print("ERROR: No MIDI input ports found!")
        print("Make sure your MIDI keyboard/dongle is connected.")
        return 1
    
    print("Available MIDI ports:")
    for i, port in enumerate(ports):
        print(f"  [{i}] {port}")
    
    # Select port
    port_name = None
    if args.device:
        try:
            idx = int(args.device)
            if 0 <= idx < len(ports):
                port_name = ports[idx]
        except ValueError:
            for p in ports:
                if args.device.lower() in p.lower():
                    port_name = p
                    break
    
    if not port_name:
        # Auto-select first keyboard-like port
        keywords = ['keyboard', 'piano', 'midi', 'digital', 'yamaha', 'roland', 'casio']
        for p in ports:
            if any(k in p.lower() for k in keywords):
                port_name = p
                break
        if not port_name:
            port_name = ports[0]
    
    print(f"\nUsing MIDI port: {port_name}")
    print(f"Sending UDP to: {args.addr}:{args.port}")
    print("\n" + "="*50)
    print("  MIDI -> UDP Bridge Active!")
    print("  Play your piano... Press Ctrl+C to stop")
    print("="*50 + "\n")
    
    note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
    note_on_times = {}  # Track when notes were pressed
    
    try:
        with mido.open_input(port_name) as midi_in:
            for msg in midi_in:
                now_ms = int(time.time() * 1000)
                
                if msg.type == 'note_on' and msg.velocity > 0:
                    # Note pressed - record time
                    note_on_times[msg.note] = now_ms
                    note = note_names[msg.note % 12]
                    octave = (msg.note // 12) - 1
                    print(f"♪ Note ON:  {note}{octave} (MIDI {msg.note})")
                    
                elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
                    # Note released - calculate duration and send
                    if msg.note in note_on_times:
                        duration_ms = now_ms - note_on_times[msg.note]
                        del note_on_times[msg.note]
                        
                        # Build 5-byte packet: [note, duration(4 bytes big-endian)]
                        packet = bytes([
                            msg.note,
                            (duration_ms >> 24) & 0xFF,
                            (duration_ms >> 16) & 0xFF,
                            (duration_ms >> 8) & 0xFF,
                            duration_ms & 0xFF
                        ])
                        
                        sock.sendto(packet, dest)
                        
                        note = note_names[msg.note % 12]
                        octave = (msg.note // 12) - 1
                        print(f"  Note OFF: {note}{octave} (MIDI {msg.note}) -> SENT duration={duration_ms}ms")
                    
    except KeyboardInterrupt:
        print("\n\nBridge stopped.")
    
    sock.close()
    return 0

if __name__ == '__main__':
    exit(main())
