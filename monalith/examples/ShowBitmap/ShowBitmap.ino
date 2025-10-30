#include "monalith.h"

// Example: show a 64x64 RGB565 bitmap persistently.
// Replace the contents of `bitmap.h` with your attached bitmap data.
#include "bitmap.h"

void setup() {
  Monalith::init();
  // Provide the example bitmap (replace `example_bitmap` in bitmap.h with your data)
  Monalith::showStaticBitmap(example_bitmap);
  // Ensure the display state machine is set to static bitmap mode
  Monalith::setDisplayState(Monalith::DisplayState::StaticBitmap);
}

void loop() {
  Monalith::tick();
  delay(16);
}
