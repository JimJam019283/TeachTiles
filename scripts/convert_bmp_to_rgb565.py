#!/usr/bin/env python3
from PIL import Image
import sys
import os

IN = "TeachTiles Graphical Overlay v1.bmp"
OUT = "example_bitmap.c"
W = 64
H = 64

if not os.path.exists(IN):
    print(f"Input {IN} not found")
    sys.exit(2)

im = Image.open(IN)
print("Opened:", im.format, im.size, im.mode)
# Convert to RGB and resize if needed
if im.size != (W, H):
    print(f"Resizing from {im.size} to {(W,H)}")
    im = im.resize((W,H), Image.LANCZOS)

im = im.convert('RGB')
px = list(im.getdata())

# Convert to RGB565
vals = []
for (r,g,b) in px:
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    val = (r5 << 11) | (g6 << 5) | b5
    vals.append(val)

# Write C file
with open(OUT, 'w') as f:
    f.write("#include <stdint.h>\n\n")
    f.write(f"const uint16_t example_bitmap[{W}*{H}] = \n")
    f.write("{\n")
    for i, v in enumerate(vals):
        if (i % 8) == 0:
            f.write("    ")
        f.write(f"0x{v:04X}, ")
        if (i % 8) == 7:
            f.write("\n")
    if (len(vals) % 8) != 0:
        f.write("\n")
    f.write("};\n")

print(f"Wrote {OUT} with {len(vals)} pixels")
