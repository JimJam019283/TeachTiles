/* stb_image - v2.28 - public domain image loader - http://nothings.org/stb
   only the minimal subset needed (single-file header) */
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
/* include a small, permissively licensed stb_image implementation here to decode BMP */
/* For brevity, this is a tiny BMP-only loader: */

#ifndef SIMPLE_BMP_LOADER_H
#define SIMPLE_BMP_LOADER_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static bool load_bmp(const char* path, int& w, int& h, std::vector<uint8_t>& rgb)
{
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    unsigned char header[54];
    if (fread(header, 1, 54, f) != 54) { fclose(f); return false; }
    if (header[0] != 'B' || header[1] != 'M') { fclose(f); return false; }
    unsigned int dataOffset = *(int*)&header[10];
    unsigned int dibSize = *(int*)&header[14];
    int width = *(int*)&header[18];
    int height = *(int*)&header[22];
    unsigned short planes = *(short*)&header[26];
    unsigned short bpp = *(short*)&header[28];
    if (planes != 1) { fclose(f); return false; }
    if (bpp != 24 && bpp != 32) { fclose(f); return false; }
    w = width; h = height;
    int bytesPerPixel = bpp / 8;
    size_t rowSize = (width * bytesPerPixel + 3) & ~3; // padded to 4 bytes
    rgb.resize(width * height * 3);
    std::vector<unsigned char> row(rowSize);
    // BMP stores pixels bottom-up
    for (int y = 0; y < height; ++y) {
        int rowIdx = height - 1 - y;
        if (fseek(f, dataOffset + (size_t)rowIdx * rowSize, SEEK_SET) != 0) { fclose(f); return false; }
        if (fread(row.data(), 1, rowSize, f) != rowSize) { fclose(f); return false; }
        for (int x = 0; x < width; ++x) {
            unsigned char b = row[x * bytesPerPixel + 0];
            unsigned char g = row[x * bytesPerPixel + 1];
            unsigned char r = row[x * bytesPerPixel + 2];
            rgb[(y * width + x) * 3 + 0] = r;
            rgb[(y * width + x) * 3 + 1] = g;
            rgb[(y * width + x) * 3 + 2] = b;
        }
    }
    fclose(f);
    return true;
}

#endif // SIMPLE_BMP_LOADER_H
