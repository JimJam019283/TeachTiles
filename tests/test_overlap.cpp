// Test overlapping note-ons: two notes on, then off, ensure two packets captured
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
    Serial2.push(std::vector<uint8_t>{0x90,60,100, 0x90,61,100}); // two note-ons
    // process
    for(int i=0;i<10;i++){ loop(); std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
    // now send offs
    Serial2.push(std::vector<uint8_t>{0x80,60,0, 0x80,61,0});
    for(int i=0;i<20;i++){ loop(); std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
    auto caps = SerialBT.getCaptured();
    std::cout<<"captured="<<caps.size()<<"\n";
    assert(caps.size()>=2);
    // check notes present
    bool saw60=false,saw61=false;
    for(auto &p:caps){ if(p[0]==60) saw60=true; if(p[0]==61) saw61=true; }
    assert(saw60 && saw61);
    std::cout<<"Test overlap passed\n";
    return 0;
}
