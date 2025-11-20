#!/usr/bin/env bash
# Auto-test loop: compile, upload to /dev/cu.usbserial-0001, capture 8s of serial, repeat until /tmp/teachtiles_autotest.stop exists
set -euo pipefail
SKETCH_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PORT="/dev/cu.usbserial-0001"
LOGDIR="$SKETCH_DIR/build_logs"
mkdir -p "$LOGDIR"
ITER=0

# Replace the P_* pin defines in monalith.cpp with the canonical mapping the
# user requested. This runs before every compile so the auto-loop always
# builds with the expected pin mapping.
patch_mon_pins() {
  MON_CPP="$SKETCH_DIR/monalith/monalith.cpp"
  if [ ! -f "$MON_CPP" ]; then
    echo "patch_mon_pins: $MON_CPP not found, skipping"
    return 0
  fi
  python3 - <<PY
from pathlib import Path
p = Path(r"$MON_CPP")
text = p.read_text()
start = text.find('#if MONALITH_HAS_PXMATRIX')
if start == -1:
    print('no MONALITH_HAS_PXMATRIX marker')
    raise SystemExit(0)
idx = text.find('static PxMATRIX', start)
if idx == -1:
    print('no static PxMATRIX marker')
    raise SystemExit(0)
before = text[:start]
after = text[idx:]
new_defs = '''#if MONALITH_HAS_PXMATRIX
#define P_R1_PIN 25
#define P_G1_PIN 26
#define P_B1_PIN 27
#define P_R2_PIN 14
#define P_G2_PIN 12
#define P_B2_PIN 13
#define P_E_PIN 32
#define P_A_PIN 23
#define P_B_PIN 19
#define P_C_PIN 5
#define P_D_PIN 17
#define P_CLK_PIN 16
#define P_LAT_PIN 4
#define P_OE_PIN 15
'''
new_text = before + new_defs + after
if new_text != text:
    p.write_text(new_text)
    print('patched', p)
else:
    print('already patched')
PY
}
while [ ! -f /tmp/teachtiles_autotest.stop ]; do
  ITER=$((ITER+1))
  echo "=== ITER $ITER: $(date) ==="
  # Ensure monalith pins are set correctly before each compile
  patch_mon_pins || true
  echo "Compiling..."
  if ! arduino-cli compile --fqbn esp32:esp32:esp32 "$SKETCH_DIR" >/tmp/teachtiles_compile.log 2>&1; then
    echo "Compile failed, saving /tmp/teachtiles_compile.log to $LOGDIR/compile_fail_$ITER.log"
    cp /tmp/teachtiles_compile.log "$LOGDIR/compile_fail_$ITER.log"
    sleep 5
    continue
  fi
  echo "Compile OK"

  BINPATH=$(arduino-cli compile --fqbn esp32:esp32:esp32 "$SKETCH_DIR" --show-binary-path 2>/dev/null | tail -n1 || true)
  echo "Binary: $BINPATH"

  echo "Uploading..."
  # write upload output both to /tmp and to workspace build_logs for inspection
  UP_TMP=/tmp/teachtiles_upload.log
  UP_LOG="$LOGDIR/upload_cmd_last.log"
  if ! (arduino-cli upload -p "$PORT" --upload-speed 115200 --fqbn esp32:esp32:esp32 "$SKETCH_DIR" 2>&1 | tee "$UP_TMP" "$UP_LOG") ; then
    echo "Upload failed, saving $UP_TMP to $LOGDIR/upload_fail_$ITER.log"
    cp "$UP_TMP" "$LOGDIR/upload_fail_$ITER.log"
    sleep 5
    continue
  fi
  echo "Upload OK"

  echo "Capturing serial for 8s..."
  LOGFILE="$LOGDIR/serial_iter_$ITER.log"
  # Use screen to capture serial for fixed time; fallback to cat + timeout
  if command -v screen >/dev/null 2>&1; then
    # detach screen session that might exist
    screen -S teachtiles_serial -X quit >/dev/null 2>&1 || true
    timeout 10s screen -L -Logfile "$LOGFILE" -S teachtiles_serial "$PORT" 115200 >/dev/null 2>&1 || true
  else
    # fallback: use python -m serial if available
    if command -v python3 >/dev/null 2>&1; then
      python3 - <<PY >"$LOGFILE" 2>&1 &
import sys, time
try:
    import serial
except Exception as e:
    print('missing pyserial:', e)
    sys.exit(1)
ser = serial.Serial('$PORT', 115200, timeout=1)
end = time.time()+8
while time.time() < end:
    data = ser.read(512)
    if data:
        sys.stdout.buffer.write(data)
        sys.stdout.flush()
ser.close()
PY
    else
      timeout 8s cat "$PORT" >"$LOGFILE" 2>&1 || true
    fi
  fi
  echo "Saved serial to $LOGFILE"
  sleep 1
done

echo "Autotest stopped by presence of /tmp/teachtiles_autotest.stop"
