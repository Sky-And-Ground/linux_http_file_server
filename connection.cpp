#include "connection.h"
#include "async_logger.h"
#include "global_config.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>

bool Connection::read_content(std::string& body_buf, size_t length) {
    while (length > 0) {
        ssize_t ret = ::recv(sock.get(), recv_buf.data(), recv_buf.size(), 0);

        if (ret < 0) {
            LOG_ERROR("%s:%d recv data failed, %d, %s\n", ep.ip.c_str(), ep.port, errno, strerror(errno));
            return false;
        }
        else if (ret == 0) {
            return false;
        }
        else {
            for (ssize_t i = 0; i < ret; ++i) {
                body_buf.push_back(recv_buf[i]);
            }

            length -= ret;
        }
    }

    return true;
}

void Connection::handle(http_method_callback& cb) {
    RequestHeaderParser parser;

    while (true) {
        ssize_t ret = ::recv(sock.get(), recv_buf.data(), recv_buf.size(), 0);

        if (ret < 0) {
            LOG_ERROR("%s:%d recv data failed, %d, %s\n", ep.ip.c_str(), ep.port, errno, strerror(errno));
            return;
        }
        else if (ret > 0) {
            auto parser_result = parser.parse(request, recv_buf.data(), ret);

            if (parser_result.first == RequestHeaderParser::result_type::complete) {
                try {
                    request.url = utils::decode_percent_encoding_url(request.url);
                }
                catch (...) {
                    send_template_html_reply(sock.get(), 1, 1, HttpStatusCode::bad_request);
                    return;
                }

                std::string body_buf;
                ssize_t body_start = (ssize_t)parser_result.second;

                while (body_start < ret) {
                    body_buf += recv_buf[body_start];
                    ++body_start;
                }

                if (request.headers_contains("Content-Length")) {
                    std::string tmp = request.headers_get("Content-Length");
                    size_t content_length = 0;

                    try {
                        content_length = std::stoul(tmp);
                    }
                    catch (...) {
                        send_template_html_reply(sock.get(), 1, 1, HttpStatusCode::bad_request);
                        return;
                    }

                    if (content_length > GlobalConfig::instance().get_max_content_length()) {
                        send_template_html_reply(sock.get(), 1, 1, HttpStatusCode::request_entity_too_large);
                        return;
                    }

                    content_length -= body_buf.size();
                    if (!read_content(body_buf, content_length)) {
                        return;
                    }

                    RequestBodyParser bodyParser;
                    if (!bodyParser.parse(request, body_buf)) {
                        send_template_html_reply(sock.get(), 1, 1, HttpStatusCode::bad_request);
                        return;
                    }
                }
                else if (request.headers_contains("Transfer-Encoding")) {
                    std::string value = request.headers_get("Transfer-Encoding");

                    if (value == "chunked") {
                        send_template_html_reply(sock.get(), 1, 1, HttpStatusCode::not_implemented);
                        return;
                    }
                }
                
                // now do the user callback.
                cb(sock.get(), ep.ip.c_str(), ep.port, request);
                return;
            }
            else if (parser_result.first == RequestHeaderParser::result_type::bad) {
                send_template_html_reply(sock.get(), 1, 1, HttpStatusCode::bad_request);
                return;
            }
            else {
                continue;   // we need more data.
            }
        }
    }
}
