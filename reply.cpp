#include "reply.h"
#include "utils.h"

#include <sys/socket.h>
#include <sys/types.h>

int send_all(int fd, const char* buf, size_t len) noexcept {
    static constexpr size_t BLOCK_SIZE = 4096;
    size_t total = len;

    while (total > 0) {
        size_t expect_sent = total > BLOCK_SIZE ? BLOCK_SIZE : total;
        ssize_t sent = send(fd, buf, expect_sent, 0);

        if (sent < 0) {
            return -1;
        }
        else if (sent == 0) {
            return 0;
        }

        buf += sent;
        total -= sent;
    }

    return 1;
}

int send_all(int fd, const std::string& str) noexcept {
    return send_all(fd, str.c_str(), str.length());
}

const char* http_status_code_str(HttpStatusCode code) {
    switch (code) {
        case HttpStatusCode::ok:
            return "ok";
        case HttpStatusCode::created:
            return "created";
        case HttpStatusCode::accepted:
            return "accepted";
        case HttpStatusCode::no_content:
            return "no content";
        case HttpStatusCode::multiple_choices:
            return "multiple choices";
        case HttpStatusCode::moved_permanently:
            return "moved permanently";
        case HttpStatusCode::moved_temporarily:
            return "moved temporarily";
        case HttpStatusCode::not_modified:
            return "not modified";
        case HttpStatusCode::bad_request:
            return "bad request";
        case HttpStatusCode::unauthorized:
            return "unauthorized";
        case HttpStatusCode::forbidden:
            return "forbidden";
        case HttpStatusCode::not_found:
            return "not found";
        case HttpStatusCode::method_not_allowed:
            return "method not allowed";
        case HttpStatusCode::request_entity_too_large:
            return "request entity too large";
        case HttpStatusCode::internal_server_error:
            return "internal server error";
        case HttpStatusCode::not_implemented:
            return "not implemented";
        case HttpStatusCode::bad_gateway:
            return "bad gateway";
        case HttpStatusCode::service_unavailable:
            return "service unavailable";
        default:
            return "unknown";
    }
}

int send_template_html_reply(int fd, int http_version_major, int http_version_minor, HttpStatusCode code) {
    std::string body = utils::str_format("<html>"
      "<head><title>%s</title></head>"
      "<body><h1>%s</h1></body>"
            "</html>", http_status_code_str(code), http_status_code_str(code));
    
    std::string tmp;
    tmp.reserve(512);

    tmp += utils::str_format("HTTP/%d.%d %d %s\r\n", http_version_major, http_version_minor, static_cast<int>(code), http_status_code_str(code));
    tmp += utils::str_format("Content-Length: %ld\r\n", body.length());
    tmp += "Connection: closed\r\n";
    tmp += "Content-Type: text/html\r\n\r\n";
    tmp += body;

    return send_all(fd, tmp);
}

int send_response(int fd, const Response& response) {
    std::string str;
    str.reserve(2048);
    
    str += utils::str_format("HTTP/%d.%d %d %s\r\n", response.http_version_major, response.http_version_minor, static_cast<int>(response.code), response.message.c_str());

    for (const auto& e : response.headers) {
        str += utils::str_format("%s: %s\r\n", e.first.c_str(), e.second.c_str());
    }

    str += "\r\n";
    return send_all(fd, str);
}

int send_chunked_data(int fd, const char* buf, size_t len) {
    std::string tmp;
    tmp.reserve(128 + len);

    tmp += utils::integer_to_hex_str(len);
    tmp += "\r\n";
    tmp.append(buf, len);
    tmp += "\r\n";

    return send_all(fd, tmp);
}

int send_chunked_data(int fd, const std::string& str) {
    return send_chunked_data(fd, str.c_str(), str.length());
}

int send_chunked_end_flag(int fd) {
    return send_all(fd, "0\r\n\r\n");
}