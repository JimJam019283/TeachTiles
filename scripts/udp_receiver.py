#!/usr/bin/env python3
"""
Simple UDP receiver to accept 5-byte MIDI packets from the ESP32's UDP transport.
Packet format: [note(1), duration(4 bytes big-endian)]
"""
import socket

HOST = '0.0.0.0'
PORT = 5005

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((HOST, PORT))
print(f"Listening on UDP {HOST}:{PORT}")

note_names = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]

def note_name(n):
    return f"{note_names[n%12]}{n//12 - 1}"

while True:
    data, addr = sock.recvfrom(1024)
    if len(data) < 5:
        print("Short packet", data)
        continue
    note = data[0]
    duration = (data[1]<<24) | (data[2]<<16) | (data[3]<<8) | data[4]
    print(f"From {addr}: Note {note_name(note)} ({note}) duration={duration} ms")
