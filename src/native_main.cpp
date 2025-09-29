// native_main.cpp
// Shim to allow compiling Arduino-style sketch on host for logic testing.
#include <chrono>
#include <thread>

// Forward declarations from the sketch
void setup();
void loop();

int main(int argc, char** argv) {
    // Call Arduino-style setup once, then call loop() in a timed loop.
    setup();
    while (true) {
        loop();
        // sleep a bit to avoid a tight busy loop; match typical Arduino loop rate
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}
