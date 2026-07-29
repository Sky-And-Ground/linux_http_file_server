#pragma once

#include <array>
#include <vector>
#include "net_socket.h"
#include "http_request.h"
#include "reply.h"

class Connection {
    std::array<char, 8192> recv_buf;
    Socket sock;
    EndPoint ep;
    Request request;

    bool read_content(std::string& body_buf, size_t length);
public:
    Connection(int fd, const char* ip, int port) : recv_buf{}, sock{ fd }, ep{ ip, port } {}

    void handle(http_method_callback& cb);
};