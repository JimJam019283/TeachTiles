#include "monalith.h"
#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#endif

#if defined(ARDUINO) && defined(ESP32)
#include <FastLED.h>
#define MONALITH_HAS_FASTLED 1
#else
#define MONALITH_HAS_FASTLED 0
#endif

// PxMatrix only on ESP32/ARDUINO builds
#if defined(USE_PXMATRIX) && defined(ESP32)
#include <PxMatrix.h>
#define MONALITH_HAS_PXMATRIX 1
#else
#define MONALITH_HAS_PXMATRIX 0
#endif

extern uint32_t millis();

namespace Monalith {

static const int WIDTH = 64;
static const int HEIGHT = 64;
static const int NUM_LEDS = WIDTH * HEIGHT; // 4096
#if MONALITH_HAS_FASTLED
static const int LED_PIN = 5;
static CRGB leds[NUM_LEDS];
#endif

#if MONALITH_HAS_PXMATRIX
#ifndef P_LAT_PIN
#define P_LAT_PIN 22
#endif
#ifndef P_OE_PIN
#define P_OE_PIN 21
#endif
#ifndef P_A_PIN
#define P_A_PIN 19
#endif
#ifndef P_B_PIN
#define P_B_PIN 18
#endif
#ifndef P_C_PIN
#define P_C_PIN 5
#endif
#ifndef P_D_PIN
#define P_D_PIN 15
#endif

static PxMATRIX matrix(WIDTH, HEIGHT, P_LAT_PIN, P_OE_PIN, P_A_PIN, P_B_PIN, P_C_PIN, P_D_PIN);
#endif

struct ActiveNote { int idx; uint8_t hue; uint8_t vel; uint32_t expire_ms; uint8_t trail_level; };
static std::vector<ActiveNote> activeNotes;

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
    return true;
}

static int xyToIndex(int x, int y) {
    if (x < 0) x = 0; if (x >= WIDTH) x = WIDTH - 1;
    if (y < 0) y = 0; if (y >= HEIGHT) y = HEIGHT - 1;
    if (y % 2 == 0) {
        return y * WIDTH + x;
    } else {
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
    int x = (rel * WIDTH) / range;
    int y = HEIGHT / 2;
    return xyToIndex(x, y);
}

static uint8_t noteToHue(uint8_t note) {
    return (uint8_t)((note % 12) * (256 / 12));
}

void showNote(uint8_t note, uint32_t duration_ms, uint8_t velocity) {
    int idx = noteToIndex(note);
    uint8_t hue = noteToHue(note);
    uint32_t now = millis();
    uint32_t dur = duration_ms == 0 ? 200 : std::min<uint32_t>(duration_ms, 8000);
    uint8_t bri = (uint8_t)std::min<int>((velocity * 2) + 30, 255);
    ActiveNote an{idx, hue, velocity, now + dur, bri};
    activeNotes.push_back(an);

    auto setPixelXY = [&](int px, int py, uint8_t h, uint8_t v){
        if (px < 0 || px >= WIDTH) return;
        if (py < 0 || py >= HEIGHT) return;
#if MONALITH_HAS_PXMATRIX
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
        (void)px; (void)py; (void)h; (void)v;
#endif
    };

    int main_x = (idx % WIDTH);
    int main_y = (idx / WIDTH);
    setPixelXY(main_x, main_y, hue, an.trail_level);
    setPixelXY(main_x - 1, main_y, hue, an.trail_level / 2);
    setPixelXY(main_x + 1, main_y, hue, an.trail_level / 2);
    setPixelXY(main_x, main_y - 1, hue, an.trail_level / 3);
    setPixelXY(main_x, main_y + 1, hue, an.trail_level / 3);

#if MONALITH_HAS_FASTLED
    FastLED.show();
#elif MONALITH_HAS_PXMATRIX
    // matrix.display();
#else
    std::printf("Monalith: showNote note=%u idx=%d hue=%u vel=%u dur=%u\n", note, idx, (unsigned)hue, (unsigned)velocity, dur);
#endif
}

void tick() {
    uint32_t now = millis();
    for (auto &a : activeNotes) {
        if (a.trail_level > 10) a.trail_level = (uint8_t)(a.trail_level * 3 / 4);
        else a.trail_level = 0;
    }
    activeNotes.erase(std::remove_if(activeNotes.begin(), activeNotes.end(), [&](const ActiveNote& a){
        return (int32_t)(a.expire_ms - now) <= 0 && a.trail_level == 0;
    }), activeNotes.end());

#if MONALITH_HAS_FASTLED
    FastLED.clear();
    for (const auto &a : activeNotes) {
        uint8_t v = a.trail_level;
        CRGB col = CHSV(a.hue, 200, v);
        if (v > 180) {
            col = blend(col, CRGB::White, (uint8_t)(v - 160));
        }
        if (a.idx >= 0 && a.idx < NUM_LEDS) leds[a.idx] = blend(leds[a.idx], col, 220);
        int left = a.idx - 1; int right = a.idx + 1;
        if (left >= 0) leds[left] = blend(leds[left], col, 120);
        if (right < NUM_LEDS) leds[right] = blend(leds[right], col, 120);
    }
    FastLED.show();
#elif MONALITH_HAS_PXMATRIX
    matrix.display();
#else
    if (!activeNotes.empty()) {
        std::printf("Monalith: active=%zu\n", activeNotes.size());
    }
#endif
}

} // namespace Monalith
