#include "server.h"
#include "async_logger.h"
#include "global_config.h"
#include "reply.h"
#include "connection.h"
#include <array>
#include <cerrno>
#include <cstring>
#include <memory>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace {
    http_method_callback callback;
}

void Server::open(const std::string& ip, int port) {
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    int option = 1;
    if (::setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, (const void*)(&option), sizeof(option)) < 0) {
        std::error_code ec{ errno, std::system_category() };
        THROW_SYSTEM_ERROR(ec, "setsockopt() on `SO_REUSEADDR` failed");
    }

    int ret = ::inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr));
    if (ret < 0) {
        std::error_code ec{ errno, std::system_category() };
        THROW_SYSTEM_ERROR(ec, "inet_pton() failed");
    }
    else if (ret == 0) {
        THROW_EXCEPTION(ip + " is not a valid ipv4/ipv6 address");
    }

    if (::bind(sock.get(), (const struct sockaddr*)(&addr), sizeof(addr)) != 0) {
        std::error_code ec{ errno, std::system_category() };
        THROW_SYSTEM_ERROR(ec, "bind() failed");
    }

    if (::listen(sock.get(), 32) != 0) {
        std::error_code ec{ errno, std::system_category() };
        THROW_SYSTEM_ERROR(ec, "listen() failed");
    }

    LOG_INFO("server starts at port: %d\n", GlobalConfig::instance().get_port());
}

Server::Server() : sock{ AF_INET, SOCK_STREAM, 0 }, pool{ 32 } {
    callback = [](int fd, const char* ip, int port, const Request& req) {
        LOG_INFO("%s:%d %s %s\n", ip, port, req.method.c_str(), req.url.c_str());
        send_template_html_reply(fd, 1, 1, HttpStatusCode::ok);
        };
}

void Server::set_http_method_callback(http_method_callback _callback) {
    callback = _callback;
}

void Server::start() {
    auto& config = GlobalConfig::instance();
    open(config.get_ip(), config.get_port());

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    std::array<char, INET6_ADDRSTRLEN> ip;
    int port;

    while (true) {
        int client = accept(sock.get(), (struct sockaddr*)&client_addr, &addr_len);

        if (client >= 0) {
            inet_ntop(AF_INET, &(client_addr.sin_addr), ip.data(), (socklen_t)ip.size());
            port = ntohs(client_addr.sin_port);

            auto alived_ip = std::make_shared<std::string>(ip.data());

            pool.submit([client, alived_ip, port]() {
                std::unique_ptr<Connection> conn{ new Connection{ client, alived_ip->c_str(), port } };

                try {
                    conn->handle(callback);
                }
                catch (const SystemError& e) {
                    LOG_ERROR("system error at %s, %s(%d), %s, value: %d, msg: %s\n", e.file, e.func, e.line, e.what(), e.ec.value(), e.ec.message().c_str());
                }
                catch (const Exception& e) {
                    LOG_ERROR("runtime error at %s, %s(%d), %s\n", e.file, e.func, e.line, e.what());
                }
                catch (const std::exception& e) {
                    LOG_ERROR("standard exception: %s\n", e.what());
                }
                catch (...) {
                    LOG_ERROR("unknown exception\n");
                }

                });
        }
        else {
            LOG_ERROR("accept failed, %d, %s\n", errno, strerror(errno));
        }
    }
}
