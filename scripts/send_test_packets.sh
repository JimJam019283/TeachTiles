#!/bin/bash
PORT=/dev/cu.usbserial-0001
stty -f $PORT 115200 raw -echo || true
(cat $PORT > /tmp/serial_test_multi.log & pid=$!;
 sleep 0.2;
 for i in 1 2 3 4; do
   printf 'MARK%02d\n' $i > $PORT; sleep 0.05;
   # C4 (60), dur 1200 -> 00 00 04 B0
   printf '\x3C\x00\x00\x04\xB0' > $PORT; sleep 0.15;
   # E4 (64), dur 600 -> 00 00 02 58
   printf '\x40\x00\x00\x02\x58' > $PORT; sleep 0.15;
   # G4 (67), dur 300 -> 00 00 01 2C
   printf '\x43\x00\x00\x01\x2C' > $PORT; sleep 0.3;
 done;
 sleep 4; kill $pid) 2>/dev/null || true
sed -n '1,200p' /tmp/serial_test_multi.log || true
