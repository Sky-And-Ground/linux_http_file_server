#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <system_error>
#include <string>
#include "custom_exception.h"

struct EndPoint {
    std::string ip;
    int port;

    EndPoint(const char* _ip, int _port) : ip{ _ip }, port{ _port } {}
};

class Socket {
    int fd;
public:
    Socket(int _fd) : fd{ _fd } {}

    Socket(int af, int type, int protocol) : fd{ -1 } {
        fd = socket(af, type, protocol);

        if (fd < 0) {
            std::error_code ec{ errno, std::system_category() };
            THROW_SYSTEM_ERROR(ec, "sys call socket failed");
        }
    }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept : fd{ other.fd } {
        other.fd = -1;
    }

    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            close();

            fd = other.fd;
            other.fd = -1;
        }

        return *this;
    }

    ~Socket() {
        close();
    }

    void close() noexcept {
        if (fd >= 0) {
            ::close(fd);
        }
    }

    int get() noexcept {
        return fd;
    }
};