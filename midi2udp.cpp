// midi2udp.cpp (host-only)
#if !defined(ARDUINO)
#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <csignal>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "RtMidi.h"

using namespace std;
static volatile bool running = true;
void sigint_handler(int) { running = false; }

void send_packet(int sock, const sockaddr_in& addr, uint8_t note, uint32_t duration_ms) {
    uint8_t pkt[5];
    pkt[0] = note;
    pkt[1] = (duration_ms >> 24) & 0xFF;
    pkt[2] = (duration_ms >> 16) & 0xFF;
    pkt[3] = (duration_ms >> 8) & 0xFF;
    pkt[4] = duration_ms & 0xFF;
    ssize_t r = sendto(sock, pkt, 5, 0, (const sockaddr*)&addr, sizeof(addr));
    if (r < 0) perror("sendto");
    else cout << "SENT note=" << int(note) << " dur=" << duration_ms << " ms\n";
}

int main(int argc, char** argv) {
    string addr_str = "255.255.255.255";
    int port = 5005;
    string device_arg;
    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--addr" && i+1 < argc) { addr_str = argv[++i]; continue; }
        if (a == "--port" && i+1 < argc) { port = stoi(argv[++i]); continue; }
        if (a == "--device" && i+1 < argc) { device_arg = argv[++i]; continue; }
        if (a == "--help") {
            cout << "Usage: midi2udp [--addr ADDR] [--port PORT] [--device name_or_index]\n";
            return 0;
        }
    }

    signal(SIGINT, sigint_handler);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }
    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("setsockopt SO_BROADCAST");
    }

    sockaddr_in dst = {};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(port);
    if (inet_pton(AF_INET, addr_str.c_str(), &dst.sin_addr) != 1) {
        cerr << "Invalid address: " << addr_str << "\n";
        return 1;
    }

    RtMidiIn midiin;
    unsigned int nPorts = midiin.getPortCount();
    if (nPorts == 0) {
        cerr << "No MIDI input ports found. Connect piano/dongle and retry." << endl;
        return 1;
    }
    cout << "MIDI ports:\n";
    for (unsigned int i = 0; i < nPorts; ++i) {
        cout << "  " << i << ": " << midiin.getPortName(i) << "\n";
    }
    unsigned int portIndex = 0;
    if (!device_arg.empty()) {
        try {
            int idx = stoi(device_arg);
            if (idx >= 0 && (unsigned)idx < nPorts) portIndex = (unsigned)idx;
        } catch (...) {
            for (unsigned int i = 0; i < nPorts; ++i) {
                if (midiin.getPortName(i).find(device_arg) != string::npos) { portIndex = i; break; }
            }
        }
    }
    cout << "Opening MIDI port " << portIndex << ": " << midiin.getPortName(portIndex) << "\n";
    midiin.openPort(portIndex);
    midiin.ignoreTypes(false, false, false);

    unordered_map<int, uint64_t> note_on_time;

    cout << "Bridge running: sending to " << addr_str << ":" << port << ". Ctrl-C to exit." << endl;

    while (running) {
        vector<unsigned char> message;
        double stamp = midiin.getMessage(&message);
        if (!message.empty()) {
            uint8_t status = message[0] & 0xF0;
            if (status == 0x90) {
                uint8_t note = message.size() > 1 ? message[1] : 0;
                uint8_t vel = message.size() > 2 ? message[2] : 0;
                uint64_t now_ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();
                if (vel > 0) {
                    note_on_time[note] = now_ms;
                    cout << "NOTE ON " << int(note) << " vel=" << int(vel) << "\n";
                } else {
                    auto it = note_on_time.find(note);
                    if (it != note_on_time.end()) {
                        uint32_t dur = uint32_t(now_ms - it->second);
                        send_packet(sock, dst, note, dur);
                        note_on_time.erase(it);
                    } else {
                        cout << "NOTE OFF (no prior ON) " << int(note) << "\n";
                    }
                }
            } else if (status == 0x80) {
                uint8_t note = message.size() > 1 ? message[1] : 0;
                uint64_t now_ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();
                auto it = note_on_time.find(note);
                if (it != note_on_time.end()) {
                    uint32_t dur = uint32_t(now_ms - it->second);
                    send_packet(sock, dst, note, dur);
                    note_on_time.erase(it);
                } else {
                    cout << "NOTE OFF (no prior ON) " << int(note) << "\n";
                }
            }
        } else {
            this_thread::sleep_for(chrono::milliseconds(1));
        }
    }

    close(sock);
    midiin.closePort();
    cout << "Exiting." << endl;
    return 0;
}
#endif
