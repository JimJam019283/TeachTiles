// Simple unit test for MIDI parsing and BT packet formation using host stubs.
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

// Pull in the application (it will use host stubs when not ARDUINO)
#include "../src/host_stubs.h"
extern void setup();
extern void loop();
extern HostSerial2 Serial2;
extern HostBT SerialBT;

int main() {
    setup();
    // Simulate: Note on (0x90, note=60, vel=64), then Note off (0x80, note=60, vel=0)
    std::vector<uint8_t> msg = { 0x90, 60, 64, 0x80, 60, 0 };
    Serial2.push(msg);

    // Run the loop a few times to process
    for (int i = 0; i < 20; ++i) {
        loop();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    auto captured = SerialBT.getCaptured();
    if (captured.empty()) {
        std::cerr << "No BT packets captured\n";
        return 2;
    }
    auto pkt = captured.front();
    uint8_t note = pkt[0];
    uint32_t duration = ((uint32_t)pkt[1] << 24) | ((uint32_t)pkt[2] << 16) | ((uint32_t)pkt[3] << 8) | pkt[4];
    std::cout << "Captured packet note=" << (int)note << " duration=" << duration << " ms\n";
    // basic assertions
    assert(note == 60);
    assert(duration >= 0);
    std::cout << "Test passed\n";
    return 0;
}
