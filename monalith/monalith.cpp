#include "monalith.h"
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#if !defined(ARDUINO)
#include <thread>
#endif
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>

// If compiling for Arduino/ESP32, attempt to use FastLED. Otherwise remain a host stub.
#if defined(ARDUINO) && defined(ESP32)
#include <FastLED.h>
#define MONALITH_HAS_FASTLED 1
#else
#define MONALITH_HAS_FASTLED 0
#endif

// Force PxMatrix for HUB75 panels by default on ESP32 only so host builds don't try to include PxMatrix.
#ifndef USE_PXMATRIX
#if defined(ESP32)
#define USE_PXMATRIX 1
#else
#define USE_PXMATRIX 0
#endif
#endif
// PxMatrix is an ESP32-only library. Only include it when building for ESP32
// with USE_PXMATRIX enabled. This prevents host/CI builds from failing
// because PxMatrix.h is not available outside the microcontroller toolchain.
#if defined(USE_PXMATRIX) && defined(ESP32)
#include <driver/gpio.h>
#include <soc/gpio_struct.h>
#include <PxMatrix.h>
#define MONALITH_HAS_PXMATRIX 1
#else
#define MONALITH_HAS_PXMATRIX 0
#endif

// Forward-declare millis() for host builds; main.cpp provides a host stub when not on ESP32.
extern uint32_t millis();

namespace Monalith {

// Simple configuration - adjust for your matrix
// Default set for the 64x64 HUB75 panel you described (4096 LEDs).
static const int WIDTH = 64;
static const int HEIGHT = 64;
static const int NUM_LEDS = WIDTH * HEIGHT; // 4096
#if MONALITH_HAS_FASTLED
// Default pin, change if your wiring uses a different GPIO
static const int LED_PIN = 5;
static CRGB leds[NUM_LEDS];
#endif

#if MONALITH_HAS_PXMATRIX
// Default pin mapping set to Waveshare RGB-Matrix-P3 (common ESP32 wiring)
// LAT and OE are typically 22 and 21 on NodeMCU-32S wiring from Waveshare.
// A/B/C/D follow the Waveshare diagram variant (A=16,B=17,C=2,D=4).
// If your panel requires the E pin for 1/32 scan, adjust accordingly.
#ifndef P_LAT_PIN
#define P_LAT_PIN 22
#endif
#ifndef P_OE_PIN
#define P_OE_PIN 21
#endif
#ifndef P_A_PIN
#define P_A_PIN 16
#endif
#ifndef P_B_PIN
#define P_B_PIN 17
#endif
#ifndef P_C_PIN
#define P_C_PIN 2
#endif
#ifndef P_D_PIN
#define P_D_PIN 4
#endif

static PxMATRIX matrix(WIDTH, HEIGHT, P_LAT_PIN, P_OE_PIN, P_A_PIN, P_B_PIN, P_C_PIN, P_D_PIN);
#endif

// Extended active note: support velocity (0-127) and a trail intensity
struct ActiveNote { int idx; uint8_t hue; uint8_t vel; uint32_t expire_ms; uint8_t trail_level; };
static std::vector<ActiveNote> activeNotes;

bool init() {
#if MONALITH_HAS_FASTLED
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
    FastLED.clear();
    FastLED.show();
    std::puts("Monalith: FastLED initialized");
#elif MONALITH_HAS_PXMATRIX
    // Initialize PxMatrix framebuffer and parameters for HUB75 panels.
    // Keep initialization minimal here; applications can adjust pwm bits or timings
    // if they have specific panel requirements.
    matrix.begin();
    matrix.clear();
    matrix.display();
    std::puts("Monalith: PxMatrix initialized");
#else
    std::puts("Monalith: init (host stub)");
#endif
    activeNotes.clear();
    return true;
}

// Map a piano MIDI note (21..108) to an LED index inside a WIDTH x HEIGHT matrix.
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
    Serial.printf("Monalith: drawPixel x=%d y=%d color565=0x%04X\n", px, py, color565);
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
    Serial.printf("Monalith: showNote note=%u idx=%d hue=%u vel=%u dur=%u\n", note, idx, (unsigned)hue, (unsigned)velocity, dur);
#else
    std::printf("Monalith: showNote note=%u idx=%d hue=%u vel=%u dur=%u\n", note, idx, (unsigned)hue, (unsigned)velocity, dur);
#endif
}

void tick() {
    uint32_t now = millis();
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

void visualizeDemoSafe(uint32_t ms) {
    uint32_t start = millis();
#if MONALITH_HAS_FASTLED
    // Fill with a distinct color (magenta) for ms duration
    for (int i = 0; i < NUM_LEDS; ++i) leds[i] = CRGB(255, 0, 255);
    FastLED.show();
    while ((int32_t)(millis() - start) < (int32_t)ms) {
        // allow other tasks; simple delay
        delay(20);
    }
    FastLED.clear();
    FastLED.show();
#elif MONALITH_HAS_PXMATRIX
    // Fill matrix with a bright magenta-like color via PxMatrix
    uint8_t r = 255, g = 0, b = 255;
    uint16_t color565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    matrix.clear();
    for (int y = 0; y < HEIGHT; ++y) for (int x = 0; x < WIDTH; ++x) matrix.drawPixel(x, y, color565);
    matrix.display();
    while ((int32_t)(millis() - start) < (int32_t)ms) {
        delay(20);
    }
    matrix.clear();
    matrix.display();
#else
    // Host: print a marker while waiting
    printf("Monalith: visualizeDemoSafe for %u ms\n", (unsigned)ms);
    uint32_t now = millis();
    while ((int32_t)(millis() - now) < (int32_t)ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
#endif
}

} // namespace Monalith
