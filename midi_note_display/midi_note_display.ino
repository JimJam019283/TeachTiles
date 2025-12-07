// MIDI Note Display on TeachTiles Grand Staff Overlay
// Displays REAL-TIME notes from a piano via USB MIDI bridge
// Notes are shown as 3x3 colored circles with 4-pixel horizontal spacing
// Colors alternate: blue, cyan, green, magenta, yellow, orange
//
// Setup:
//   1. Connect piano MIDI OUT -> USB MIDI dongle -> Mac
//   2. Connect ESP32 to Mac via USB
//   3. Run: python3 scripts/midi_to_esp32.py
//   4. Play piano - notes appear on display!

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Include the TeachTiles bitmap overlay
#include "teachtiles_bitmap.h"

// Panel size
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64

// Pin mapping - verified working
HUB75_I2S_CFG::i2s_pins _pins = {
  25,  // R1
  26,  // G1
  27,  // B1
  14,  // R2
  12,  // G2
  13,  // B2
  23,  // A
  19,  // B
  5,   // C
  17,  // D
  32,  // E
  4,   // LAT
  15,  // OE
  16   // CLK
};

MatrixPanel_I2S_DMA *display = nullptr;

// ============================================
// STAFF LAYOUT MAPPING (from TeachTiles bitmap)
// ============================================
// The TeachTiles overlay has a GRAND STAFF:
// 
// TREBLE CLEF (upper staff) - Y positions of lines:
//   Line 5 (top):    F5 = Y 8
//   Line 4:          D5 = Y 12
//   Line 3:          B4 = Y 16
//   Line 2:          G4 = Y 20
//   Line 1 (bottom): E4 = Y 24
//   (Spaces are between lines: E5=Y10, C5=Y14, A4=Y18, F4=Y22)
//   Below treble staff: D4=Y26, C4=Y28 (middle C ledger line), B3=Y30
//
// GAP between staves: ~Y 31-35
//
// BASS CLEF (lower staff) - Y positions of lines:
//   Line 5 (top):    A3 = Y 36
//   Line 4:          F3 = Y 40
//   Line 3:          D3 = Y 44
//   Line 2:          B2 = Y 48
//   Line 1 (bottom): G2 = Y 52
//   (Spaces: G3=Y38, E3=Y42, C3=Y46, A2=Y50)
//   Below bass staff: F2=Y54, E2=Y56, D2=Y58, C2=Y60
//
// Each staff LINE is 4 pixels apart, each SPACE is 2 pixels from adjacent line

// Staff area where notes can be placed
const int TREBLE_STAFF_START_X = 18;  // After the clef symbol
const int STAFF_NOTE_AREA_WIDTH = 22; // Width available for notes (before bar line at x=42)

// Note horizontal spacing (each note is 4 pixels apart)
const int NOTE_SPACING = 4;
const int MAX_NOTES_DISPLAY = 5;  // Max notes that fit in the staff area

// Alternating note colors (RGB values)
struct Color {
  uint8_t r, g, b;
};

const Color NOTE_COLORS[] = {
  {0, 100, 255},    // Blue (primary)
  {0, 255, 255},    // Cyan
  {0, 255, 100},    // Green
  {255, 0, 255},    // Magenta
  {255, 255, 0},    // Yellow
  {255, 128, 0},    // Orange
};
const int NUM_COLORS = 6;
int colorIndex = 0;  // Track which color to use next

// Accidental types
enum Accidental {
  NATURAL = 0,
  SHARP = 1,
  FLAT = 2
};

// Octave transposition indicators
enum OctaveShift {
  NO_SHIFT = 0,
  SHIFT_UP = 1,    // Note is actually higher (8va, 15ma, etc.)
  SHIFT_DOWN = 2   // Note is actually lower (8vb, 15mb, etc.)
};

// Displayable range constants
// Full piano range we want to support: C2 (MIDI 36) to C7 (MIDI 96)
// Display can show: C2 (Y=59) to about F5/G5 (Y=5-7) directly
// Notes above ~G5 will be transposed down with up-arrow indicator
const uint8_t PIANO_MIN_NOTE = 36;   // C2 - lowest note on your piano  
const uint8_t PIANO_MAX_NOTE = 96;   // C7 - highest note on your piano
const uint8_t DISPLAY_MIN_NOTE = 36; // C2 - bottom ledger lines of bass clef (Y=59)
const uint8_t DISPLAY_MAX_NOTE = 79; // G5 - top of treble clef area (Y=5)

// Note names for serial output
const char* NOTE_NAMES[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// Track active notes with timing (for duration calculation)
struct ActiveNote {
  uint8_t midiNote;
  unsigned long startTime;
  bool active;
};
const int MAX_ACTIVE_NOTES = 16;
ActiveNote activeNotes[MAX_ACTIVE_NOTES];

// Track displayed notes
struct DisplayedNote {
  uint8_t midiNote;       // Original MIDI note
  uint8_t displayNote;    // Note as displayed (may be transposed)
  int xPosition;
  Color color;
  Accidental accidental;
  OctaveShift octaveShift; // Whether note was transposed
  int8_t octavesShifted;   // How many octaves (for 8va vs 15ma display)
  bool active;
};

DisplayedNote displayedNotes[MAX_NOTES_DISPLAY];
int noteCount = 0;
int nextNoteX = TREBLE_STAFF_START_X;

// ============================================
// MIDI NOTE TO Y POSITION MAPPING
// ============================================
// Maps MIDI note number to Y coordinate on the TeachTiles staff
// MIDI 60 = C4 (middle C), 72 = C5, etc.
// 
// Staff positions (Y coordinates):
// Higher Y = lower on screen = lower pitch
//
// Note: Black keys (sharps/flats) are placed on the same line as their
// natural note but with an accidental symbol drawn beside them.

// Check if a MIDI note is a black key (sharp/flat)
Accidental getNoteAccidental(uint8_t midiNote) {
  // Black keys in each octave: C#/Db, D#/Eb, F#/Gb, G#/Ab, A#/Bb
  // Positions in octave: 1, 3, 6, 8, 10
  int noteInOctave = midiNote % 12;
  switch(noteInOctave) {
    case 1:  // C#/Db - display as sharp (going up from C)
    case 6:  // F#/Gb - display as sharp (going up from F)
      return SHARP;
    case 3:  // D#/Eb - display as flat (going down from E)
    case 8:  // G#/Ab - display as flat (going down from A)
    case 10: // A#/Bb - display as flat (going down from B)
      return FLAT;
    default:
      return NATURAL;
  }
}

// Get the Y position for a MIDI note using direct lookup
// This maps notes to their EXACT positions on the TeachTiles grand staff bitmap
// Black keys share the line/space of their base note (C# on C, Db on D, etc.)
int getMidiNoteYPosition(uint8_t midiNote) {
  // DIRECT LOOKUP TABLE for exact Y positions on TeachTiles bitmap
  // These are measured from the actual bitmap staff lines
  // Staff lines occur at specific Y values, spaces are between them
  
  // From bitmap analysis:
  // TREBLE CLEF LINES: Y = 11, 15, 19, 23, 27 (top to bottom: F5, D5, B4, G4, E4)
  // BASS CLEF LINES: Y = 36, 40, 44, 48, 52 (top to bottom: A3, F3, D3, B2, G2)
  // Space between lines = 2 pixels (for notes in spaces)
  // Middle C area = Y ~31-32 (between staves)
  
  // For notes with sharps/flats, use the natural note's position
  int noteInOctave = midiNote % 12;
  int octave = midiNote / 12;  // MIDI octave (C4=60 is octave 5)
  
  // Map to white key note number (0=C, 1=D, 2=E, 3=F, 4=G, 5=A, 6=B)
  int whiteKeyNum;
  switch(noteInOctave) {
    case 0: case 1:   whiteKeyNum = 0; break;  // C, C#
    case 2: case 3:   whiteKeyNum = 1; break;  // D, D#
    case 4:           whiteKeyNum = 2; break;  // E
    case 5: case 6:   whiteKeyNum = 3; break;  // F, F#
    case 7: case 8:   whiteKeyNum = 4; break;  // G, G#
    case 9: case 10:  whiteKeyNum = 5; break;  // A, A#
    case 11:          whiteKeyNum = 6; break;  // B
    default:          whiteKeyNum = 0; break;
  }
  
  // HARDCODED Y positions for each note (based on bitmap analysis)
  // Reference: Treble E4 (bottom line) = Y27, each step up = -2 pixels
  // Reference: Bass G2 (bottom line) = Y52, each step up = -2 pixels
  
  // Calculate based on octave and note within octave
  // Using middle C (C4, octave 5) = Y 31 as reference
  // Each white key step = 2 pixels
  
  int y;
  
  // Calculate steps from middle C (C4, octave 5, whiteKeyNum 0)
  int stepsFromMiddleC = (octave - 5) * 7 + whiteKeyNum;
  
  // Middle C = Y 31 (just above bass clef, ledger line position)
  // Going UP (higher pitch) = smaller Y values
  // Going DOWN (lower pitch) = larger Y values
  y = 31 - (stepsFromMiddleC * 2);
  
  // Clamp to visible area
  if (y < 3) y = 3;    // Top of display (above treble staff)
  if (y > 60) y = 60;  // Bottom of display (below bass staff)
  
  return y;
}

// Draw the TeachTiles bitmap overlay (inverted: black staff on white background)
void drawTeachTilesOverlay() {
  for (int y = 0; y < PANEL_HEIGHT; y++) {
    for (int x = 0; x < PANEL_WIDTH; x++) {
      int idx = y * PANEL_WIDTH + x;
      uint32_t pixel = pgm_read_dword(&epd_bitmap_TeachTiles_Graphical_Overlay_v1[idx]);
      
      // Invert colors (white background becomes black, black lines become white)
      uint8_t r = 255 - ((pixel >> 16) & 0xFF);
      uint8_t g = 255 - ((pixel >> 8) & 0xFF);
      uint8_t b = 255 - (pixel & 0xFF);
      
      display->drawPixelRGB888(x, y, r, g, b);
    }
  }
}

// Draw a 3x3 filled square for natural notes
void drawNoteNatural(int centerX, int centerY, Color color) {
  // 3x3 filled square
  // XXX
  // XXX
  // XXX
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      display->drawPixelRGB888(centerX + dx, centerY + dy, color.r, color.g, color.b);
    }
  }
}

// Draw a + pattern for sharp notes
void drawNoteSharp(int centerX, int centerY, Color color) {
  // 3x3 plus pattern
  //  X
  // XXX
  //  X
  display->drawPixelRGB888(centerX, centerY - 1, color.r, color.g, color.b);      // Top
  display->drawPixelRGB888(centerX - 1, centerY, color.r, color.g, color.b);      // Left
  display->drawPixelRGB888(centerX, centerY, color.r, color.g, color.b);          // Center
  display->drawPixelRGB888(centerX + 1, centerY, color.r, color.g, color.b);      // Right
  display->drawPixelRGB888(centerX, centerY + 1, color.r, color.g, color.b);      // Bottom
}

// Draw an X pattern for flat notes
void drawNoteFlat(int centerX, int centerY, Color color) {
  // 3x3 X pattern
  // X X
  //  X
  // X X
  display->drawPixelRGB888(centerX - 1, centerY - 1, color.r, color.g, color.b);  // Top-left
  display->drawPixelRGB888(centerX + 1, centerY - 1, color.r, color.g, color.b);  // Top-right
  display->drawPixelRGB888(centerX, centerY, color.r, color.g, color.b);          // Center
  display->drawPixelRGB888(centerX - 1, centerY + 1, color.r, color.g, color.b);  // Bottom-left
  display->drawPixelRGB888(centerX + 1, centerY + 1, color.r, color.g, color.b);  // Bottom-right
}

// Draw upward arrow indicator (note is actually higher than shown)
void drawUpArrow(int centerX, int centerY, Color color) {
  // Small upward arrow above the note
  //   X
  //  XXX
  int arrowY = centerY - 3;  // Position above the note
  display->drawPixelRGB888(centerX, arrowY - 1, color.r, color.g, color.b);      // Top point
  display->drawPixelRGB888(centerX - 1, arrowY, color.r, color.g, color.b);      // Left
  display->drawPixelRGB888(centerX, arrowY, color.r, color.g, color.b);          // Center
  display->drawPixelRGB888(centerX + 1, arrowY, color.r, color.g, color.b);      // Right
}

// Draw downward arrow indicator (note is actually lower than shown)
void drawDownArrow(int centerX, int centerY, Color color) {
  // Small downward arrow below the note
  //  XXX
  //   X
  int arrowY = centerY + 3;  // Position below the note
  display->drawPixelRGB888(centerX - 1, arrowY, color.r, color.g, color.b);      // Left
  display->drawPixelRGB888(centerX, arrowY, color.r, color.g, color.b);          // Center
  display->drawPixelRGB888(centerX + 1, arrowY, color.r, color.g, color.b);      // Right
  display->drawPixelRGB888(centerX, arrowY + 1, color.r, color.g, color.b);      // Bottom point
}

// Draw a complete note based on accidental type, with optional octave indicator
void drawNote(int centerX, int centerY, Color color, Accidental accidental, OctaveShift octaveShift = NO_SHIFT) {
  // Draw the note shape
  switch (accidental) {
    case NATURAL:
      drawNoteNatural(centerX, centerY, color);
      break;
    case SHARP:
      drawNoteSharp(centerX, centerY, color);
      break;
    case FLAT:
      drawNoteFlat(centerX, centerY, color);
      break;
  }
  
  // Draw octave shift indicator if needed
  switch (octaveShift) {
    case SHIFT_UP:
      drawUpArrow(centerX, centerY, color);
      break;
    case SHIFT_DOWN:
      drawDownArrow(centerX, centerY, color);
      break;
    case NO_SHIFT:
    default:
      break;
  }
}

// Calculate transposed note and shift direction for notes outside displayable range
void getDisplayableNote(uint8_t midiNote, uint8_t &displayNote, OctaveShift &shift, int8_t &octavesShifted) {
  displayNote = midiNote;
  shift = NO_SHIFT;
  octavesShifted = 0;
  
  // Transpose notes that are too high down by octaves
  while (displayNote > DISPLAY_MAX_NOTE) {
    displayNote -= 12;
    octavesShifted++;
    shift = SHIFT_UP;  // Arrow points up to show actual note is higher
  }
  
  // Transpose notes that are too low up by octaves
  while (displayNote < DISPLAY_MIN_NOTE) {
    displayNote += 12;
    octavesShifted++;
    shift = SHIFT_DOWN;  // Arrow points down to show actual note is lower
  }
}

// Add a new note to display
void addNote(uint8_t midiNote) {
  if (noteCount >= MAX_NOTES_DISPLAY) {
    // Clear all notes and start over
    noteCount = 0;
    nextNoteX = TREBLE_STAFF_START_X;
    colorIndex = 0;  // Reset color cycle
    Serial.println("Staff full - clearing notes");
  }
  
  // Calculate display note (may be transposed if outside range)
  uint8_t displayNote;
  OctaveShift octaveShift;
  int8_t octavesShifted;
  getDisplayableNote(midiNote, displayNote, octaveShift, octavesShifted);
  
  // Detect if this is a sharp/flat (use original note for correct accidental)
  Accidental accidental = getNoteAccidental(midiNote);
  
  displayedNotes[noteCount].midiNote = midiNote;
  displayedNotes[noteCount].displayNote = displayNote;
  displayedNotes[noteCount].xPosition = nextNoteX;
  displayedNotes[noteCount].color = NOTE_COLORS[colorIndex];
  displayedNotes[noteCount].accidental = accidental;
  displayedNotes[noteCount].octaveShift = octaveShift;
  displayedNotes[noteCount].octavesShifted = octavesShifted;
  displayedNotes[noteCount].active = true;
  
  noteCount++;
  nextNoteX += NOTE_SPACING;
  colorIndex = (colorIndex + 1) % NUM_COLORS;  // Cycle to next color
  
  // Enhanced serial output
  Serial.print("Added note: MIDI ");
  Serial.print(midiNote);
  Serial.print(" (");
  printNoteName(midiNote);
  Serial.print(")");
  if (accidental == SHARP) Serial.print(" [SHARP]");
  if (accidental == FLAT) Serial.print(" [FLAT]");
  if (octaveShift != NO_SHIFT) {
    Serial.print(" -> displayed as ");
    printNoteName(displayNote);
    Serial.print(" (");
    Serial.print(octavesShifted);
    Serial.print(" octave");
    if (octavesShifted > 1) Serial.print("s");
    Serial.print(octaveShift == SHIFT_UP ? " 8va↑" : " 8vb↓");
    Serial.print(")");
  }
  Serial.print(" at X=");
  Serial.print(displayedNotes[noteCount-1].xPosition);
  Serial.print(", Y=");
  Serial.println(getMidiNoteYPosition(displayNote));
}

// Clear all displayed notes
void clearNotes() {
  noteCount = 0;
  nextNoteX = TREBLE_STAFF_START_X;
}

// Redraw the entire display (overlay + notes)
void redrawDisplay() {
  // First draw the TeachTiles overlay
  drawTeachTilesOverlay();
  
  // Then draw all active notes on top
  for (int i = 0; i < noteCount; i++) {
    if (displayedNotes[i].active) {
      // Use displayNote (which may be transposed) for Y position
      int y = getMidiNoteYPosition(displayedNotes[i].displayNote);
      drawNote(displayedNotes[i].xPosition, y, displayedNotes[i].color, 
               displayedNotes[i].accidental, displayedNotes[i].octaveShift);
    }
  }
}

// Get note name string (e.g., "C4", "F#5")
void printNoteName(uint8_t midiNote) {
  int noteInOctave = midiNote % 12;
  int octave = (midiNote / 12) - 1;  // MIDI octave convention
  Serial.print(NOTE_NAMES[noteInOctave]);
  Serial.print(octave);
}

// Find or create an active note slot
int findActiveNoteSlot(uint8_t midiNote, bool create) {
  // First, look for existing note
  for (int i = 0; i < MAX_ACTIVE_NOTES; i++) {
    if (activeNotes[i].active && activeNotes[i].midiNote == midiNote) {
      return i;
    }
  }
  // If creating, find empty slot
  if (create) {
    for (int i = 0; i < MAX_ACTIVE_NOTES; i++) {
      if (!activeNotes[i].active) {
        return i;
      }
    }
  }
  return -1;
}

// Process incoming MIDI data from Serial (USB bridge)
void processMIDI() {
  static uint8_t midiBuffer[3];
  static int midiBufferIndex = 0;
  
  while (Serial.available()) {
    uint8_t byte = Serial.read();
    
    // Check for status byte (Note On = 0x90, Note Off = 0x80)
    if (byte & 0x80) {
      midiBufferIndex = 0;
      midiBuffer[midiBufferIndex++] = byte;
    } else if (midiBufferIndex > 0 && midiBufferIndex < 3) {
      midiBuffer[midiBufferIndex++] = byte;
      
      // Complete MIDI message received
      if (midiBufferIndex == 3) {
        uint8_t status = midiBuffer[0] & 0xF0;
        uint8_t note = midiBuffer[1];
        uint8_t velocity = midiBuffer[2];
        
        if (status == 0x90 && velocity > 0) {
          // Note On
          int slot = findActiveNoteSlot(note, true);
          if (slot >= 0) {
            activeNotes[slot].midiNote = note;
            activeNotes[slot].startTime = millis();
            activeNotes[slot].active = true;
          }
          
          Serial.print("♪ NOTE ON:  ");
          printNoteName(note);
          Serial.print(" (MIDI ");
          Serial.print(note);
          Serial.print(") vel=");
          Serial.println(velocity);
          
          addNote(note);
          redrawDisplay();
        }
        else if (status == 0x80 || (status == 0x90 && velocity == 0)) {
          // Note Off
          int slot = findActiveNoteSlot(note, false);
          unsigned long duration = 0;
          if (slot >= 0) {
            duration = millis() - activeNotes[slot].startTime;
            activeNotes[slot].active = false;
          }
          
          Serial.print("  NOTE OFF: ");
          printNoteName(note);
          Serial.print(" (MIDI ");
          Serial.print(note);
          Serial.print(") held for ");
          Serial.print(duration);
          Serial.println(" ms");
        }
        
        midiBufferIndex = 0;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n========================================");
  Serial.println("=== MIDI Note Display on TeachTiles ===");
  Serial.println("========================================");
  
  // Initialize active notes tracking
  for (int i = 0; i < MAX_ACTIVE_NOTES; i++) {
    activeNotes[i].active = false;
  }
  
  // Configure for 64x64 panel
  HUB75_I2S_CFG mxconfig(PANEL_WIDTH, PANEL_HEIGHT, 1, _pins);
  mxconfig.gpio.e = 32;
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::FM6126A;
  
  Serial.println("Initializing display...");
  display = new MatrixPanel_I2S_DMA(mxconfig);
  
  if (!display->begin()) {
    Serial.println("*** ERROR: Display begin() failed! ***");
    while(1) { delay(1000); }
  }
  
  display->setBrightness8(128);
  display->clearScreen();
  
  // Draw initial overlay
  drawTeachTilesOverlay();
  
  Serial.println("TeachTiles overlay displayed!");
  Serial.println("");
  Serial.println("Waiting for MIDI from USB bridge...");
  Serial.println("Run: python3 scripts/midi_to_esp32.py");
  Serial.println("");
  Serial.println("Note shapes:");
  Serial.println("  Filled square = Natural");
  Serial.println("  + shape = Sharp");
  Serial.println("  X shape = Flat");
  Serial.println("");
  Serial.println("Octave indicators:");
  Serial.println("  Arrow UP = Note is actually higher (8va)");
  Serial.println("  Arrow DOWN = Note is actually lower (8vb)");
  Serial.println("");
  Serial.print("Display range: ");
  printNoteName(DISPLAY_MIN_NOTE);
  Serial.print(" to ");
  printNoteName(DISPLAY_MAX_NOTE);
  Serial.println(" (notes outside transposed by octave)");
}

void loop() {
  // Process real-time MIDI input from piano
  processMIDI();
  
  delay(10);  // Small delay, fast response for real-time input
}
