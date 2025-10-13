// Test that durations are non-zero when note held for some time
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include "../src/host_stubs.h"
extern void setup();
extern void loop();
extern HostSerial2 Serial2;
extern HostBT SerialBT;

int main(){
    setup();
    Serial2.push(std::vector<uint8_t>{0x90,62,120});
    // hold for 50ms
    for(int i=0;i<10;i++){ loop(); std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
    Serial2.push(std::vector<uint8_t>{0x80,62,0});
    for(int i=0;i<10;i++){ loop(); std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
    auto caps = SerialBT.getCaptured();
    assert(!caps.empty());
    auto p = caps.back();
    uint32_t duration = ((uint32_t)p[1]<<24)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<8)|p[4];
    std::cout<<"duration="<<duration<<" ms\n";
    assert(duration>0);
    std::cout<<"Test duration passed\n";
    return 0;
}
