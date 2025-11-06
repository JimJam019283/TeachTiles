#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "stb_image.h"

int main(int argc, char** argv) {
    const char* in = "TeachTiles Graphical Overlay v1.bmp";
    const char* out = "example_bitmap.c";
    if (argc >= 2) in = argv[1];
    if (argc >= 3) out = argv[2];

    int w = 0, h = 0;
    std::vector<uint8_t> rgb;
    if (!load_bmp(in, w, h, rgb)) {
        std::fprintf(stderr, "Failed to load %s\n", in);
        return 2;
    }
    std::printf("Loaded BMP %s (%dx%d)\n", in, w, h);
    const int W = 64, H = 64;
    // Resize? For simplicity, if size mismatches, fail.
    if (w != W || h != H) {
        std::fprintf(stderr, "BMP must be %dx%d, got %dx%d\n", W, H, w, h);
        return 3;
    }
    // Convert to RGB565 and write C file
    std::ofstream f(out, std::ios::trunc);
    if (!f) { std::fprintf(stderr, "Failed to open %s for writing\n", out); return 4; }
    f << "#include <stdint.h>\n\n";
    f << "const uint16_t example_bitmap[64*64] =\n{\n";
    for (int i = 0; i < W*H; ++i) {
        uint8_t r = rgb[i*3 + 0];
        uint8_t g = rgb[i*3 + 1];
        uint8_t b = rgb[i*3 + 2];
        uint16_t r5 = (r >> 3) & 0x1F;
        uint16_t g6 = (g >> 2) & 0x3F;
        uint16_t b5 = (b >> 3) & 0x1F;
        uint16_t val = (r5 << 11) | (g6 << 5) | b5;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "0x%04X, ", val);
        f << buf;
        if ((i & 7) == 7) f << "\n";
    }
    f << "\n};\n";
    f.close();
    std::printf("Wrote %s\n", out);
    return 0;
}
