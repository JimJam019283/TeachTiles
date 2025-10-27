#!/bin/bash
PORT=/dev/cu.usbserial-0001
stty -f $PORT 115200 raw -echo || true
(cat $PORT > /tmp/serial_ascii_test.log & pid=$!;
 sleep 0.2;
 echo -e "NOTE 60 1200\n" > $PORT; sleep 0.3;
 echo -e "NOTE 64 600\n" > $PORT; sleep 0.3;
 echo -e "NOTE 67 300\n" > $PORT; sleep 2;
 kill $pid) 2>/dev/null || true
sed -n '1,200p' /tmp/serial_ascii_test.log || true
