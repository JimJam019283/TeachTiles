#!/usr/bin/env python3
"""
Simple serial bridge: forward stdin lines to serial port and print incoming serial.
Usage: python3 tools/serial_bridge.py /dev/cu.usbserial-0001 115200
"""
import sys
import threading
import serial


def reader(ser):
    try:
        while True:
            data = ser.read(1024)
            if data:
                try:
                    sys.stdout.buffer.write(data)
                    sys.stdout.flush()
                except Exception:
                    print(data)
    except Exception:
        pass


def main():
    if len(sys.argv) < 3:
        print("Usage: serial_bridge.py <port> <baud>")
        sys.exit(2)
    port = sys.argv[1]
    baud = int(sys.argv[2])
    ser = serial.Serial(port, baud, timeout=0.1)
    t = threading.Thread(target=reader, args=(ser,), daemon=True)
    t.start()
    try:
        while True:
            line = sys.stdin.readline()
            if not line:
                break
            ser.write(line.encode('utf-8'))
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()

if __name__ == '__main__':
    main()
