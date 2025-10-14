// Test: note-off without prior note-on should be ignored (no BT packet)
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include "../src/host_stubs.h"
extern void setup();
extern void loop();
extern HostSerial2 Serial2;
extern HostBT SerialBT;

int main() {
    setup();
    // Push only a Note OFF message (0x80, note=64, vel=0)
    Serial2.push(std::vector<uint8_t>{0x80, 64, 0});
    for (int i = 0; i < 10; ++i) { loop(); std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
    auto caps = SerialBT.getCaptured();
    if (!caps.empty()) {
        std::cerr << "Expected no BT packets, but captured=" << caps.size() << "\n";
        return 2;
    }
    std::cout << "Test ignore_off_without_on passed\n";
    return 0;
}
