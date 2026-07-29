#pragma once

#include "net_socket.h"
#include "thread_pool.h"
#include "http_request.h"

class Server {
    Socket sock;
    multi_thread::ThreadPool pool;

    void open(const std::string& ip, int port);
public:
    Server();

    void set_http_method_callback(http_method_callback _callback);
    void start();
};