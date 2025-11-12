#include "monalith.h"
// Arduino-specific includes should only be pulled in when building for the Arduino/ESP32 toolchain.
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>

// Force the user's HUB75 pin mapping to ensure correct wiring is used.
// This safely undefines any previous macros then defines the requested pins.
#ifdef P_R1_PIN
#undef P_R1_PIN
#endif
#define P_R1_PIN 26  // r1 - D26

#ifdef P_G1_PIN
#undef P_G1_PIN
#endif
#define P_G1_PIN 27  // g1 - D27

#ifdef P_B1_PIN
#undef P_B1_PIN
#endif
#define P_B1_PIN 32  // b1 - D32

#ifdef P_R2_PIN
#undef P_R2_PIN
#endif
#define P_R2_PIN 33  // r2 - D33

#ifdef P_G2_PIN
#undef P_G2_PIN
#endif
#define P_G2_PIN 25  // g2 - GPIO25

#ifdef P_B2_PIN
#undef P_B2_PIN
#endif
#define P_B2_PIN 13  // b2 - GPIO13

#ifdef P_E_PIN
#undef P_E_PIN
#endif
#define P_E_PIN 14   // E - GPIO14

#ifdef P_A_PIN
#undef P_A_PIN
#endif
#define P_A_PIN 23   // A - GPIO23

#ifdef P_B_PIN
#undef P_B_PIN
#endif
#define P_B_PIN 22   // B - GPIO22

#ifdef P_C_PIN
#undef P_C_PIN
#endif
#define P_C_PIN 21   // C - GPIO21

#ifdef P_D_PIN
#undef P_D_PIN
#endif
#define P_D_PIN 19   // D - GPIO19

#ifdef P_CLK_PIN
#undef P_CLK_PIN
#endif
#define P_CLK_PIN 18 // CLK - GPIO18

#ifdef P_LAT_PIN
#undef P_LAT_PIN
#endif
#define P_LAT_PIN 5  // LAT - GPIO5

#ifdef P_OE_PIN
#undef P_OE_PIN
#endif
#define P_OE_PIN 15  // OE - GPIO15
// If compiling for Arduino/ESP32, attempt to use FastLED. Otherwise remain a host stub.
#if defined(ARDUINO) && defined(ESP32)
#define P_B1_PIN 32
#define P_R2_PIN 33
#define MONALITH_HAS_FASTLED 1
#else
#define P_R2_PIN 33
#endif

#define P_G2_PIN 25
#ifndef USE_PXMATRIX
#if defined(ESP32)
#define P_B2_PIN 13
#else
#define USE_PXMATRIX 0
#define P_E_PIN 14
#endif
#endif
// Optional HUB75 parallel panel support via PxMatrix. Enable with -DUSE_PXMATRIX

// with USE_PXMATRIX enabled. This avoids CI/host builds trying to find PxMatrix.h.
#if defined(USE_PXMATRIX) && defined(ESP32)
#define P_A_PIN 23
#define P_B_PIN 22
#include <soc/gpio_struct.h>
#include <PxMatrix.h>
#define P_C_PIN 21
#else
#define MONALITH_HAS_PXMATRIX 0
#define P_D_PIN 19

// Forward-declare millis() for host builds; main.cpp provides a host stub when not on ESP32.
#define P_CLK_PIN 18

namespace Monalith {
#define P_LAT_PIN 5
// Simple configuration - adjust for your matrix
// Configuration - change these to match your hardware
#define P_OE_PIN 15
static const int WIDTH = 64;
static const int HEIGHT = 64;
static const int NUM_LEDS = WIDTH * HEIGHT; // 4096
#define P_A_PIN 23
#define P_B_PIN 22
static const int LED_PIN = 5;
static CRGB leds[NUM_LEDS];
#endif

#if MONALITH_HAS_PXMATRIX
// Default example pin mapping for HUB75 on ESP32 - adjusted to user's wiring.
// We'll define the pins if not already provided via build flags.
#ifndef P_R1_PIN
#define P_R1_PIN 26
#endif
#ifndef P_G1_PIN
#define P_G1_PIN 27
#endif
#ifndef P_B1_PIN
#define P_B1_PIN 32
#endif
#ifndef P_R2_PIN
#define P_R2_PIN 33
#endif
#ifndef P_G2_PIN
#define P_G2_PIN 25
#endif
#ifndef P_B2_PIN
#define P_B2_PIN 13
#endif
#ifndef P_E_PIN
#define P_E_PIN 14
#endif
#ifndef P_A_PIN
#define P_A_PIN 23
#endif
#ifndef P_B_PIN
#define P_B_PIN 22
#endif
#ifndef P_C_PIN
#define P_C_PIN 21
#endif
#ifndef P_D_PIN
#define P_D_PIN 19
#endif
#ifndef P_CLK_PIN
#define P_CLK_PIN 18
#endif
#ifndef P_LAT_PIN
#define P_LAT_PIN 5
#endif
#ifndef P_OE_PIN
#define P_OE_PIN 15
#endif

// Include P_E_PIN when constructing PxMATRIX in case the panel requires 5 address lines
static PxMATRIX matrix(WIDTH, HEIGHT, P_LAT_PIN, P_OE_PIN, P_A_PIN, P_B_PIN, P_C_PIN, P_D_PIN, P_E_PIN);
#endif

// Extended active note: support velocity (0-127) and a trail intensity
struct ActiveNote { int idx; uint8_t hue; uint8_t vel; uint32_t expire_ms; uint8_t trail_level; };
static std::vector<ActiveNote> activeNotes;

// Static bitmap buffer (RGB565). When active, `tick()` will render this buffer
// on the panel instead of the animated note visuals. Caller provides an array
// of WIDTH*HEIGHT uint16_t values in RGB565 layout. The buffer is kept as a
// copy so the caller can free or modify their source after calling.
static bool staticBitmapActive = false;
static uint16_t staticBitmapBuf[NUM_LEDS];

// Display state machine
static DisplayState currentDisplayState = DisplayState::Normal;

void setDisplayState(DisplayState s) {
    currentDisplayState = s;
}

DisplayState getDisplayState() {
    return currentDisplayState;
}

bool init() {
#if MONALITH_HAS_FASTLED
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
    FastLED.clear();
    FastLED.show();
    std::puts("Monalith: FastLED initialized");
#else
    std::puts("Monalith: init (host stub)");
#endif
    activeNotes.clear();
#if MONALITH_HAS_PXMATRIX
    // Print configured pin mapping for verification
    std::printf("Monalith: PxMatrix pins LAT=%d OE=%d A=%d B=%d C=%d D=%d CLK=%d R1=%d G1=%d B1=%d R2=%d G2=%d B2=%d E=%d\n",
                P_LAT_PIN, P_OE_PIN, P_A_PIN, P_B_PIN, P_C_PIN, P_D_PIN, P_CLK_PIN,
                P_R1_PIN, P_G1_PIN, P_B1_PIN, P_R2_PIN, P_G2_PIN, P_B2_PIN, P_E_PIN);
#endif
    return true;
}

// Map a piano MIDI note (21..108) to an LED index inside a WIDTH x HEIGHT matrix.
// Strategy: map the 88-key range across the X axis (columns). Visuals use vertical bars
// in the column for the note; chord spreads occupy neighboring columns/rows.
static int xyToIndex(int x, int y) {
    // serpentine mapping is common for LED matrices; assume row-major serpentine
    if (x < 0) x = 0; if (x >= WIDTH) x = WIDTH - 1;
    if (y < 0) y = 0; if (y >= HEIGHT) y = HEIGHT - 1;
    if (y % 2 == 0) {
        // even row left-to-right
        return y * WIDTH + x;
    } else {
        // odd row right-to-left
        return y * WIDTH + (WIDTH - 1 - x);
    }
}

static int noteToIndex(uint8_t note) {
    const int MIN_NOTE = 21;
    const int MAX_NOTE = 108;
    if (note < MIN_NOTE) note = MIN_NOTE;
    if (note > MAX_NOTE) note = MAX_NOTE;
    int range = MAX_NOTE - MIN_NOTE + 1; // 88
    int rel = note - MIN_NOTE; // 0..87
    // map across width
    int x = (rel * WIDTH) / range;
    // central vertical position (start in middle of height)
    int y = HEIGHT / 2;
    return xyToIndex(x, y);
}

// Convert note to hue (0-255)
static uint8_t noteToHue(uint8_t note) {
    return (uint8_t)((note % 12) * (256 / 12));
}

// velocity: 0-127 influences brightness. duration_ms controls how long the note is held.
// We'll also spawn lightweight 'trail' entries by setting trail_level which decays in tick().
void showNote(uint8_t note, uint32_t duration_ms, uint8_t velocity) {
    int idx = noteToIndex(note);
    uint8_t hue = noteToHue(note);
    uint32_t now = millis();
    // Animation expiry: keep visual for at least duration_ms, clamp to reasonable max
    uint32_t dur = duration_ms == 0 ? 200 : std::min<uint32_t>(duration_ms, 8000);
    // Map velocity (0..127) -> brightness (30..255)
    uint8_t bri = (uint8_t)std::min<int>((velocity * 2) + 30, 255);
    ActiveNote an{idx, hue, velocity, now + dur, bri};
    activeNotes.push_back(an);

    // Unified pixel setter: supports PxMatrix (HUB75) or FastLED strips
    auto setPixelXY = [&](int px, int py, uint8_t h, uint8_t v){
        if (px < 0 || px >= WIDTH) return;
        if (py < 0 || py >= HEIGHT) return;
#if MONALITH_HAS_PXMATRIX
        // Convert HSV to RGB and send to PxMatrix (uses 16-bit color)
        auto hsvToRgb = [](uint8_t H, uint8_t S, uint8_t V, uint8_t &r, uint8_t &g, uint8_t &b){
            uint8_t region = H / 43;
            uint8_t remainder = (H - (region * 43)) * 6;
            uint8_t p = (V * (255 - S)) >> 8;
            uint8_t q = (V * (255 - ((S * remainder) >> 8))) >> 8;
            uint8_t t = (V * (255 - ((S * (255 - remainder)) >> 8))) >> 8;
            switch(region) {
                case 0: r = V; g = t; b = p; break;
                case 1: r = q; g = V; b = p; break;
                case 2: r = p; g = V; b = t; break;
                case 3: r = p; g = q; b = V; break;
                case 4: r = t; g = p; b = V; break;
                default: r = V; g = p; b = q; break;
            }
        };
        uint8_t r,g,b; hsvToRgb(h, 200, v, r, g, b);
        uint16_t color565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        matrix.drawPixel(px, py, color565);
#elif MONALITH_HAS_FASTLED
        int ii = xyToIndex(px, py);
        if (ii < 0 || ii >= NUM_LEDS) return;
        CRGB color = CHSV(h, 200, v);
        leds[ii] = blend(leds[ii], color, 192);
#else
        // host: no-op (debug printing elsewhere)
#endif
    };

    // Determine column/x,y for idx
    int main_x = (idx % WIDTH);
    int main_y = (idx / WIDTH);
    setPixelXY(main_x, main_y, hue, an.trail_level);
    // chord spread: neighboring columns and a small vertical spread
    setPixelXY(main_x - 1, main_y, hue, an.trail_level / 2);
    setPixelXY(main_x + 1, main_y, hue, an.trail_level / 2);
    setPixelXY(main_x, main_y - 1, hue, an.trail_level / 3);
    setPixelXY(main_x, main_y + 1, hue, an.trail_level / 3);

#if MONALITH_HAS_FASTLED
    FastLED.show();
#elif MONALITH_HAS_PXMATRIX
    // For PxMatrix, we defer display() to tick() to allow batching; optionally call here for immediate update
    // matrix.display();
#else
    std::printf("Monalith: showNote note=%u idx=%d hue=%u vel=%u dur=%u\n", note, idx, (unsigned)hue, (unsigned)velocity, dur);
#endif
}

// Display a persistent RGB565 bitmap (WIDTH x HEIGHT). The bitmap is copied
// into an internal buffer and will be displayed until `clearStaticBitmap()`
// is called. If `bitmap` is null the request is ignored.
void showStaticBitmap(const uint16_t* bitmap) {
    if (!bitmap) return;
    // copy NUM_LEDS 16-bit values
    std::memcpy(staticBitmapBuf, bitmap, sizeof(staticBitmapBuf));
    staticBitmapActive = true;
    setDisplayState(DisplayState::StaticBitmap);
    // Immediately perform a bright full-panel white draw so users can verify the
    // panel is powered and responsive. Then blit the provided bitmap to the
    // framebuffer and present it.
#if MONALITH_HAS_PXMATRIX
    matrix.setBrightness(255);
    // draw solid white
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            matrix.drawPixel(x, y, 0xFFFF);
        }
    }
    matrix.display();
    // keep the white frame visible for 30 seconds so the user can inspect the panel
    delay(30000);
    // Color-channel visibility test: red, green, blue, white (3s each)
    const uint16_t COL_RED = 0xF800;
    const uint16_t COL_GREEN = 0x07E0;
    const uint16_t COL_BLUE = 0x001F;
    const uint16_t COL_WHITE = 0xFFFF;
    std::puts("Monalith: starting color-channel test: RED/GREEN/BLUE/WHITE");
    uint16_t colors[] = {COL_RED, COL_GREEN, COL_BLUE, COL_WHITE};
    for (uint16_t c : colors) {
        matrix.clearDisplay();
        for (int y = 0; y < HEIGHT; ++y) for (int x = 0; x < WIDTH; ++x) matrix.drawPixel(x, y, c);
        matrix.display();
        if (c == COL_RED) std::puts("Monalith: COLOR TEST red");
        else if (c == COL_GREEN) std::puts("Monalith: COLOR TEST green");
        else if (c == COL_BLUE) std::puts("Monalith: COLOR TEST blue");
        else std::puts("Monalith: COLOR TEST white");
        delay(3000);
    }
    // Slower row-sweep diagnostic: light each row red and print the row index (200ms per row)
    for (int y = 0; y < HEIGHT; ++y) {
        matrix.clearDisplay();
        for (int x = 0; x < WIDTH; ++x) {
            matrix.drawPixel(x, y, COL_RED); // red (RGB565)
        }
        matrix.display();
        std::printf("Monalith: row-sweep row=%d\n", y);
        delay(200);
    }
    // Blit the copied bitmap immediately after the diagnostic
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            int idx = y * WIDTH + x;
            uint16_t c = staticBitmapBuf[idx];
            matrix.drawPixel(x, y, c);
        }
    }
    matrix.display();
#endif
    // Log activation for serial verification
    std::puts("Monalith: static bitmap enabled");
}

// Non-blocking fast blit: copy and immediately present the bitmap without
// running the long diagnostic sequence. Useful for quick test flashes.
void showStaticBitmapFast(const uint16_t* bitmap) {
    if (!bitmap) return;
    std::memcpy(staticBitmapBuf, bitmap, sizeof(staticBitmapBuf));
    staticBitmapActive = true;
    setDisplayState(DisplayState::StaticBitmap);
#if MONALITH_HAS_PXMATRIX
    // Direct blit
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            int idx = y * WIDTH + x;
            matrix.drawPixel(x, y, staticBitmapBuf[idx]);
        }
    }
    matrix.display();
#endif
    std::puts("Monalith: static bitmap fast enabled");
}

// Stop displaying the static bitmap and resume normal animated behavior.
void clearStaticBitmap() {
    staticBitmapActive = false;
#if MONALITH_HAS_PXMATRIX
    // optionally clear the matrix framebuffer
    for (int y = 0; y < HEIGHT; ++y) for (int x = 0; x < WIDTH; ++x) matrix.drawPixel(x, y, 0);
    matrix.display();
#elif MONALITH_HAS_FASTLED
    FastLED.clear();
    FastLED.show();
#endif
}

void tick() {
    uint32_t now = millis();
    // If a static bitmap is active, render it and return immediately.
    if (staticBitmapActive) {
#if MONALITH_HAS_PXMATRIX
        // PxMatrix expects 16-bit RGB565; simply blit the bitmap into framebuffer
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                int idx = y * WIDTH + x;
                uint16_t color565 = staticBitmapBuf[idx];
                matrix.drawPixel(x, y, color565);
            }
        }
        matrix.display();
#elif MONALITH_HAS_FASTLED
        // Expand RGB565 into 8-bit RGB and write into FastLED buffer using xyToIndex mapping
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                int idx = y * WIDTH + x;
                uint16_t color565 = staticBitmapBuf[idx];
                // expand RGB565 -> r,g,b (8-bit)
                uint8_t r5 = (color565 >> 11) & 0x1F;
                uint8_t g6 = (color565 >> 5) & 0x3F;
                uint8_t b5 = color565 & 0x1F;
                uint8_t r = (r5 << 3) | (r5 >> 2);
                uint8_t g = (g6 << 2) | (g6 >> 4);
                uint8_t b = (b5 << 3) | (b5 >> 2);
                int ii = xyToIndex(x, y);
                if (ii >= 0 && ii < NUM_LEDS) leds[ii] = CRGB(r, g, b);
            }
        }
        FastLED.show();
#else
        // Host: print a small notice once
        static bool once = false;
        if (!once) { std::puts("Monalith: static bitmap active (host stub)"); once = true; }
#endif
        return;
    }
    // Decay trail levels and remove expired notes
    for (auto &a : activeNotes) {
        // trail_level decays over time
        if (a.trail_level > 10) a.trail_level = (uint8_t)(a.trail_level * 3 / 4);
        else a.trail_level = 0;
    }
    activeNotes.erase(std::remove_if(activeNotes.begin(), activeNotes.end(), [&](const ActiveNote& a){
        return (int32_t)(a.expire_ms - now) <= 0 && a.trail_level == 0;
    }), activeNotes.end());

#if MONALITH_HAS_FASTLED
    // Blue/white themed rendering with simple trail blending
    FastLED.clear();
    for (const auto &a : activeNotes) {
        uint8_t v = a.trail_level;
        // Convert base hue to a blue-white blend: lower hues -> bluer, high brightness -> white
        CRGB col = CHSV(a.hue, 200, v);
        // Convert very bright notes toward white to create blue/white palette
        if (v > 180) {
            // mix towards white
            col = blend(col, CRGB::White, (uint8_t)(v - 160));
        }
        // Place the main pixel
        if (a.idx >= 0 && a.idx < NUM_LEDS) leds[a.idx] = blend(leds[a.idx], col, 220);
        // soft spread for chords: neighbors get reduced intensity
        int left = a.idx - 1; int right = a.idx + 1;
        if (left >= 0) leds[left] = blend(leds[left], col, 120);
        if (right < NUM_LEDS) leds[right] = blend(leds[right], col, 120);
    }
    FastLED.show();
#elif MONALITH_HAS_PXMATRIX
    // For PxMatrix, we've plotted pixels during showNote; now present the framebuffer
    matrix.display();
#else
    // host: print active notes for debugging
    if (!activeNotes.empty()) {
        std::printf("Monalith: active=%zu\n", activeNotes.size());
    }
#endif
}

} // namespace Monalith
