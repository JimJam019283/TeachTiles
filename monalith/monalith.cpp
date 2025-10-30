#include "monalith.h"
#include <Arduino.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>

// If compiling for Arduino/ESP32, attempt to use FastLED. Otherwise remain a host stub.
#if defined(ARDUINO) && defined(ESP32)
#include <FastLED.h>
#define MONALITH_HAS_FASTLED 1
#else
#define MONALITH_HAS_FASTLED 0
#endif

// Force PxMatrix for HUB75 panels by default so no extra compile flags are required.
#ifndef USE_PXMATRIX
#define USE_PXMATRIX 1
#endif
// Optional HUB75 parallel panel support via PxMatrix. Enable with -DUSE_PXMATRIX
#if defined(USE_PXMATRIX)
#if defined(ESP32)
#include <driver/gpio.h>
#include <soc/gpio_struct.h>
#endif
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
// forward-declare xyToIndex for functions defined earlier that use it
static int xyToIndex(int x, int y);
#endif
#if MONALITH_HAS_FASTLED
// Default pin, change if your wiring uses a different GPIO
static const int LED_PIN = 5;
static CRGB leds[NUM_LEDS];
#endif

#if MONALITH_HAS_PXMATRIX
// Pin mapping updated to user's ESP32 wiring
// Mapping requested by user:
// r1 - D26
// G1 - D27
// B1 - D32
// GND - GND
// r2 - d33
// G2 - 25
// B2 - 13
// E - 14
// A - 23
// B - 22
// C - 21
// D - 19
// CLK - 18
// Lat - 5
// OE - 15
#define P_R1_PIN 26
#define P_G1_PIN 27
#define P_B1_PIN 32
#define P_R2_PIN 33
#define P_G2_PIN 25
#define P_B2_PIN 13
#define P_E_PIN 14
#define P_A_PIN 23
#define P_B_PIN 22
#define P_C_PIN 21
#define P_D_PIN 19
#define P_CLK_PIN 18
#define P_LAT_PIN 5
#define P_OE_PIN 15

// Include P_E_PIN when constructing PxMATRIX in case the panel requires 5 address lines
static PxMATRIX matrix(WIDTH, HEIGHT, P_LAT_PIN, P_OE_PIN, P_A_PIN, P_B_PIN, P_C_PIN, P_D_PIN, P_E_PIN);
#endif

// Static bitmap storage and display-state (defined here so Arduino build links them)
static bool staticBitmapActive = false;
static uint16_t staticBitmapBuf[WIDTH * HEIGHT];
static DisplayState currentDisplayState = DisplayState::Normal;

void showStaticBitmap(const uint16_t* bitmap) {
    if (!bitmap) return;
    std::memcpy(staticBitmapBuf, bitmap, sizeof(staticBitmapBuf));
    staticBitmapActive = true;
#if defined(USE_PXMATRIX)
    // Immediately perform a bright full-panel white draw so users can verify the
    // panel is powered and responsive. Then blit the provided bitmap to the
    // framebuffer and present it.
    matrix.setBrightness(255);
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            matrix.drawPixel(x, y, 0xFFFF);
        }
    }
    matrix.display();
    delay(10000);
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
    std::puts("Monalith: static bitmap enabled (library)");
}

void clearStaticBitmap() {
    staticBitmapActive = false;
#if defined(USE_PXMATRIX)
    for (int y = 0; y < HEIGHT; ++y) for (int x = 0; x < WIDTH; ++x) matrix.drawPixel(x, y, 0);
    matrix.display();
#endif
}

void setDisplayState(DisplayState s) { currentDisplayState = s; }
DisplayState getDisplayState() { return currentDisplayState; }

// Extended active note: support velocity (0-127) and a trail intensity
struct ActiveNote { int idx; uint8_t hue; uint8_t vel; uint32_t expire_ms; uint8_t trail_level; };
static std::vector<ActiveNote> activeNotes;
// Demo blink state (non-blocking): when demo_end_ms != 0, tick() will toggle full-panel
static uint32_t demo_end_ms = 0;
static uint32_t demo_next_toggle = 0;
static bool demo_on = false;
// Glyph display state
static uint32_t glyph_end_ms = 0;
static bool glyph_active = false;
static int glyph_x0 = 0;
static int glyph_y0 = 0;
static int glyph_scale = 1;
static uint16_t glyph_c_color = 0xF800; // red default
static uint16_t glyph_hash_color = 0xFFFF; // white default
static bool glyph_printed_map = false;

// NOTE: forceHardwareWhite removed — white hardware test disabled per user request.

bool init() {
    // Initialize available backends. If both FastLED and PxMatrix are compiled in,
    // initialize both so showNote/tick can safely use either path.
#if MONALITH_HAS_FASTLED
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
    FastLED.clear();
    FastLED.show();
    std::puts("Monalith: FastLED initialized");
#endif
#if MONALITH_HAS_PXMATRIX
    // Force-panel control pins to outputs early to avoid external dongles
    // holding strapping lines and disabling the panel
    pinMode(P_OE_PIN, OUTPUT);
    pinMode(P_LAT_PIN, OUTPUT);
    pinMode(P_CLK_PIN, OUTPUT);
    // Drive OE low (enable output), toggle LAT and CLK to a known state
    digitalWrite(P_OE_PIN, LOW); // enable panel
    digitalWrite(P_LAT_PIN, HIGH);
    digitalWrite(P_LAT_PIN, LOW);
    digitalWrite(P_CLK_PIN, LOW);
    std::printf("Monalith: forced OE=%d LAT=%d CLK=%d\n", digitalRead(P_OE_PIN), digitalRead(P_LAT_PIN), digitalRead(P_CLK_PIN));

    // initialize PxMatrix frame buffer and set reasonable brightness
    matrix.begin();
    matrix.setBrightness(50);
    matrix.clearDisplay();
    matrix.display();
    std::puts("Monalith: PxMatrix (HUB75) initialized");
    // Print configured pin mapping for verification
    std::printf("Monalith: PxMatrix pins LAT=%d OE=%d A=%d B=%d C=%d D=%d CLK=%d R1=%d G1=%d B1=%d R2=%d G2=%d B2=%d E=%d\n",
                P_LAT_PIN, P_OE_PIN, P_A_PIN, P_B_PIN, P_C_PIN, P_D_PIN, P_CLK_PIN,
                P_R1_PIN, P_G1_PIN, P_B1_PIN, P_R2_PIN, P_G2_PIN, P_B2_PIN, P_E_PIN);
#endif
#if !MONALITH_HAS_FASTLED && !MONALITH_HAS_PXMATRIX
    std::puts("Monalith: init (host stub)");
#endif
    activeNotes.clear();
    return true;
}

// Simple non-blocking demo runner: schedule a few notes across the keyboard so the
// display can be sanity-checked during setup. This function schedules notes for
// `ms` milliseconds and returns immediately.
void visualizeDemoSafe(uint32_t /*ms*/) {
    // Demo functions disabled: only static bitmap boot path is used.
    // Intentionally left empty to preserve existing API but avoid test patterns.
}

// Simple 8x8 bitmaps for 'C' and '#'
static const uint8_t GLYPH_C[8] = {
    0b00111100,
    0b01100110,
    0b11000000,
    0b11000000,
    0b11000000,
    0b11000000,
    0b01100110,
    0b00111100
};

static const uint8_t GLYPH_HASH[8] = {
    0b00010010,
    0b00010010,
    0b11111111,
    0b00010010,
    0b00010010,
    0b11111111,
    0b00010010,
    0b00010010
};

// convert 16-bit 565 color to 8-bit RGB
static void color565_to_rgb(uint16_t c565, uint8_t &r, uint8_t &g, uint8_t &b) {
    uint8_t r5 = (c565 >> 11) & 0x1F;
    uint8_t g6 = (c565 >> 5) & 0x3F;
    uint8_t b5 = c565 & 0x1F;
    r = (r5 << 3) | (r5 >> 2);
    g = (g6 << 2) | (g6 >> 4);
    b = (b5 << 3) | (b5 >> 2);
}

// Draw an 8x8 glyph scaled by `scale` (each glyph pixel becomes scale x scale block)
void drawGlyphAt(int x0, int y0, const uint8_t *glyph, uint16_t color565, int scale=1, bool print_map=false) {
    for (int gy = 0; gy < 8; ++gy) {
        for (int gx = 0; gx < 8; ++gx) {
            if (!(glyph[gy] & (1 << (7 - gx)))) continue;
            // draw a scale x scale block
            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {
                    int px = x0 + gx * scale + sx;
                    int py = y0 + gy * scale + sy;
#if MONALITH_HAS_PXMATRIX
                    matrix.drawPixel(px, py, color565);
#elif MONALITH_HAS_FASTLED
                    int idx = xyToIndex(px, py);
                    if (idx >= 0 && idx < NUM_LEDS) {
                        uint8_t rr,gg,bb; color565_to_rgb(color565, rr, gg, bb);
                        leds[idx] = CRGB(rr,gg,bb);
                    }
#endif
                    if (print_map) {
                        int idx = xyToIndex(px, py);
                        Serial.printf("MAP px=%d py=%d -> idx=%d\n", px, py, idx);
                    }
                }
            }
        }
    }
}
void drawCSharp(uint32_t ms) {
    // schedule glyph display for ms milliseconds and draw immediately
    uint32_t now = millis();
    glyph_end_ms = now + ms;
    glyph_active = true;
    glyph_printed_map = false;

    // scale glyphs 2x to produce 16x16 each
    int scale = 2;
    int glyphW = 8 * scale;
    int gap = 2 * scale;
    int totalW = glyphW + gap + glyphW;
    int x0 = (WIDTH - totalW) / 2;
    int y0 = (HEIGHT - glyphW) / 2;
    glyph_x0 = x0;
    glyph_y0 = y0;
    glyph_scale = scale;
    glyph_c_color = 0xF800; // red
    glyph_hash_color = 0xFFFF; // white

    // initial draw (also print mapping once)
#if MONALITH_HAS_PXMATRIX
    matrix.clearDisplay();
    drawGlyphAt(x0, y0, GLYPH_C, glyph_c_color, scale, true);
    drawGlyphAt(x0 + glyphW + gap, y0, GLYPH_HASH, glyph_hash_color, scale, true);
    matrix.display();
#elif MONALITH_HAS_FASTLED
    FastLED.clear();
    drawGlyphAt(x0, y0, GLYPH_C, glyph_c_color, scale, true);
    drawGlyphAt(x0 + glyphW + gap, y0, GLYPH_HASH, glyph_hash_color, scale, true);
    FastLED.show();
#endif
    glyph_printed_map = true;
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

void tick() {
    uint32_t now = millis();
    // Handle non-blocking demo blink: 1Hz (toggle every 500ms)
    if (demo_end_ms != 0 && (int32_t)(demo_end_ms - now) > 0) {
        if ((int32_t)(demo_next_toggle - now) <= 0) {
            demo_on = !demo_on;
            demo_next_toggle = now + 500;
            // Render full on or clear depending on demo_on
#if MONALITH_HAS_PXMATRIX
            if (demo_on) {
                uint16_t white = 0xFFFF;
                for (int y = 0; y < HEIGHT; ++y) for (int x = 0; x < WIDTH; ++x) matrix.drawPixel(x, y, white);
            } else {
                matrix.clearDisplay();
            }
            matrix.display();
#elif MONALITH_HAS_FASTLED
            if (demo_on) {
                for (int i = 0; i < NUM_LEDS; ++i) leds[i] = CRGB::White;
            } else {
                for (int i = 0; i < NUM_LEDS; ++i) leds[i] = CRGB::Black;
            }
            FastLED.show();
#endif
        }
        return; // while demo active, skip normal rendering
    } else {
        // demo finished; reset demo state
        demo_end_ms = 0;
        demo_next_toggle = 0;
        demo_on = false;
    }
    // If a glyph is active, redraw it every tick so it persists
    if (glyph_active) {
        if ((int32_t)(glyph_end_ms - now) <= 0) {
            glyph_active = false;
            // clear display when finished
#if MONALITH_HAS_PXMATRIX
            matrix.clearDisplay();
            matrix.display();
#elif MONALITH_HAS_FASTLED
            FastLED.clear();
            FastLED.show();
#endif
        } else {
            // redraw glyph frame
            int x0 = glyph_x0;
            int y0 = glyph_y0;
            int scale = glyph_scale;
#if MONALITH_HAS_PXMATRIX
            drawGlyphAt(x0, y0, GLYPH_C, glyph_c_color, scale, false);
            drawGlyphAt(x0 + (8*scale) + (2*scale), y0, GLYPH_HASH, glyph_hash_color, scale, false);
            matrix.display();
#elif MONALITH_HAS_FASTLED
            drawGlyphAt(x0, y0, GLYPH_C, glyph_c_color, scale, false);
            drawGlyphAt(x0 + (8*scale) + (2*scale), y0, GLYPH_HASH, glyph_hash_color, scale, false);
            FastLED.show();
#endif
        }
        return; // skip normal rendering while glyph active
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
