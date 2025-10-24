// Moved host-only serial capture utility
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

int set_baud(int fd, int baud) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) return -1;
#if defined(__APPLE__) || defined(__linux__)
    cfmakeraw(&tty);
#endif
    speed_t sp = B115200;
    switch (baud) {
        case 115200: sp = B115200; break;
        case 31250: sp = B38400; /* fallback, not all systems support B31250 */ break;
        default: sp = B115200; break;
    }
    cfsetispeed(&tty, sp);
    cfsetospeed(&tty, sp);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1; // tenths
    if (tcsetattr(fd, TCSANOW, &tty) != 0) return -1;
    return 0;
}

int main(int argc, char** argv) {
    std::string p1 = "/dev/cu.usbserial-0001";
    std::string p2 = "/dev/cu.usbserial-3";
    int duration = 5; // seconds
    int baud = 115200;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--ports" && i+1 < argc) {
            std::string v = argv[++i];
            auto pos = v.find(',');
            if (pos != std::string::npos) {
                p1 = v.substr(0,pos);
                p2 = v.substr(pos+1);
            }
        } else if (a == "--duration" && i+1 < argc) {
            duration = atoi(argv[++i]);
        } else if (a == "--baud" && i+1 < argc) {
            baud = atoi(argv[++i]);
        }
    }
    std::cout << "Capturing serial ports:\n  TX -> " << p1 << "\n  RX -> " << p2 << "\n";
    std::cout << "Baud: " << baud << ", duration: " << duration << "s\n";

    int fd1 = open(p1.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd1 < 0) std::cerr << "Failed open " << p1 << ": " << strerror(errno) << "\n";
    else {
        if (set_baud(fd1, baud) != 0) std::cerr << "Failed set baud on " << p1 << "\n";
    }
    int fd2 = open(p2.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd2 < 0) std::cerr << "Failed open " << p2 << ": " << strerror(errno) << "\n";
    else {
        if (set_baud(fd2, baud) != 0) std::cerr << "Failed set baud on " << p2 << "\n";
    }

    std::ofstream out1("/tmp/esp_tx_cap.txt", std::ios::binary);
    std::ofstream out2("/tmp/esp_rx_cap.txt", std::ios::binary);
    if (!out1) { std::cerr << "Failed create /tmp/esp_tx_cap.txt\n"; }
    if (!out2) { std::cerr << "Failed create /tmp/esp_rx_cap.txt\n"; }

    auto start = std::chrono::steady_clock::now();
    const int BUF_SIZE = 1024;
    std::vector<char> buf(BUF_SIZE);
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() < duration) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int maxfd = -1;
        if (fd1 >= 0) { FD_SET(fd1, &readfds); maxfd = std::max(maxfd, fd1); }
        if (fd2 >= 0) { FD_SET(fd2, &readfds); maxfd = std::max(maxfd, fd2); }
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000; // 200ms
        int r = select(maxfd+1, &readfds, NULL, NULL, &tv);
        if (r > 0) {
            if (fd1 >= 0 && FD_ISSET(fd1, &readfds)) {
                ssize_t n = read(fd1, buf.data(), BUF_SIZE);
                if (n > 0) {
                    out1.write(buf.data(), n);
                    out1.flush();
                }
            }
            if (fd2 >= 0 && FD_ISSET(fd2, &readfds)) {
                ssize_t n = read(fd2, buf.data(), BUF_SIZE);
                if (n > 0) {
                    out2.write(buf.data(), n);
                    out2.flush();
                }
            }
        }
    }
    if (fd1 >= 0) close(fd1);
    if (fd2 >= 0) close(fd2);
    out1.close(); out2.close();

    std::cout << "Capture complete. Files:\n  /tmp/esp_tx_cap.txt\n  /tmp/esp_rx_cap.txt\n";
    auto print_preview = [](const char* path, int maxchars){
        std::ifstream f(path, std::ios::binary);
        if (!f) { std::cout << path << ": (missing)\n"; return; }
        std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (s.empty()) { std::cout << path << ": (empty)\n"; return; }
        bool printable = true;
        int cnt = 0;
        for (char c: s) { if ((unsigned char)c < 9 && (unsigned char)c != 10 && (unsigned char)c != 13) { printable = false; break; } if (++cnt > maxchars) break; }
        std::cout << "-- " << path << " preview --\n";
        if (printable) {
            std::cout << s.substr(0, maxchars) << "\n";
        } else {
            int to = std::min((int)s.size(), 256);
            for (int i = 0; i < to; ++i) {
                printf("%02X ", (unsigned char)s[i]);
                if ((i%16)==15) printf("\n");
            }
            printf("\n");
        }
    };
    print_preview("/tmp/esp_tx_cap.txt", 1000);
    print_preview("/tmp/esp_rx_cap.txt", 1000);
    return 0;
}
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

int set_baud(int fd, int baud) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) return -1;
#if defined(__APPLE__) || defined(__linux__)
    cfmakeraw(&tty);
#endif
    speed_t sp = B115200;
    switch (baud) {
        case 115200: sp = B115200; break;
        case 31250: sp = B38400; /* fallback, not all systems support B31250 */ break;
        default: sp = B115200; break;
    }
    cfsetispeed(&tty, sp);
    cfsetospeed(&tty, sp);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1; // tenths
    if (tcsetattr(fd, TCSANOW, &tty) != 0) return -1;
    return 0;
}

int main(int argc, char** argv) {
    std::string p1 = "/dev/cu.usbserial-0001";
    std::string p2 = "/dev/cu.usbserial-3";
    int duration = 5; // seconds
    int baud = 115200;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--ports" && i+1 < argc) {
            std::string v = argv[++i];
            auto pos = v.find(',');
            if (pos != std::string::npos) {
                p1 = v.substr(0,pos);
                p2 = v.substr(pos+1);
            }
        } else if (a == "--duration" && i+1 < argc) {
            duration = atoi(argv[++i]);
        } else if (a == "--baud" && i+1 < argc) {
            baud = atoi(argv[++i]);
        }
    }
    std::cout << "Capturing serial ports:\n  TX -> " << p1 << "\n  RX -> " << p2 << "\n";
    std::cout << "Baud: " << baud << ", duration: " << duration << "s\n";

    int fd1 = open(p1.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd1 < 0) std::cerr << "Failed open " << p1 << ": " << strerror(errno) << "\n";
    else {
        if (set_baud(fd1, baud) != 0) std::cerr << "Failed set baud on " << p1 << "\n";
    }
    int fd2 = open(p2.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd2 < 0) std::cerr << "Failed open " << p2 << ": " << strerror(errno) << "\n";
    else {
        if (set_baud(fd2, baud) != 0) std::cerr << "Failed set baud on " << p2 << "\n";
    }

    std::ofstream out1("/tmp/esp_tx_cap.txt", std::ios::binary);
    std::ofstream out2("/tmp/esp_rx_cap.txt", std::ios::binary);
    if (!out1) { std::cerr << "Failed create /tmp/esp_tx_cap.txt\n"; }
    if (!out2) { std::cerr << "Failed create /tmp/esp_rx_cap.txt\n"; }

    auto start = std::chrono::steady_clock::now();
    const int BUF_SIZE = 1024;
    std::vector<char> buf(BUF_SIZE);
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() < duration) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int maxfd = -1;
        if (fd1 >= 0) { FD_SET(fd1, &readfds); maxfd = std::max(maxfd, fd1); }
        if (fd2 >= 0) { FD_SET(fd2, &readfds); maxfd = std::max(maxfd, fd2); }
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000; // 200ms
        int r = select(maxfd+1, &readfds, NULL, NULL, &tv);
        if (r > 0) {
            if (fd1 >= 0 && FD_ISSET(fd1, &readfds)) {
                ssize_t n = read(fd1, buf.data(), BUF_SIZE);
                if (n > 0) {
                    out1.write(buf.data(), n);
                    out1.flush();
                }
            }
            if (fd2 >= 0 && FD_ISSET(fd2, &readfds)) {
                ssize_t n = read(fd2, buf.data(), BUF_SIZE);
                if (n > 0) {
                    out2.write(buf.data(), n);
                    out2.flush();
                }
            }
        }
    }
    if (fd1 >= 0) close(fd1);
    if (fd2 >= 0) close(fd2);
    out1.close(); out2.close();

    std::cout << "Capture complete. Files:\n  /tmp/esp_tx_cap.txt\n  /tmp/esp_rx_cap.txt\n";
    // Print a short readable preview (try UTF-8, fallback to hex)
    auto print_preview = [](const char* path, int maxchars){
        std::ifstream f(path, std::ios::binary);
        if (!f) { std::cout << path << ": (missing)\n"; return; }
        std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (s.empty()) { std::cout << path << ": (empty)\n"; return; }
        bool printable = true;
        int cnt = 0;
        for (char c: s) { if ((unsigned char)c < 9 && (unsigned char)c != 10 && (unsigned char)c != 13) { printable = false; break; } if (++cnt > maxchars) break; }
        std::cout << "-- " << path << " preview --\n";
        if (printable) {
            std::cout << s.substr(0, maxchars) << "\n";
        } else {
            // hex dump first 256 bytes
            int to = std::min((int)s.size(), 256);
            for (int i = 0; i < to; ++i) {
                printf("%02X ", (unsigned char)s[i]);
                if ((i%16)==15) printf("\n");
            }
            printf("\n");
        }
    };
    print_preview("/tmp/esp_tx_cap.txt", 1000);
    print_preview("/tmp/esp_rx_cap.txt", 1000);
    return 0;
}
