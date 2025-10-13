#pragma once
#include <cstdint>
#include <vector>
#include <array>

struct HostSerial2 {
    void begin(int, int, int, int);
    int available();
    uint8_t read();
    void push(uint8_t b);
    void push(const std::vector<uint8_t>& bytes);
};

struct HostBT {
    bool begin(const char* name);
    size_t write(const uint8_t* buf, size_t n);
    const std::vector<std::array<uint8_t,5>>& getCaptured() const;
    void clear();
};
