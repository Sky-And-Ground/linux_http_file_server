#pragma once

#include <array>
#include <string>
#include <map>

enum class HttpStatusCode {
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    method_not_allowed = 405,
    request_entity_too_large = 413,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
};

struct Response {
    int http_version_major;
    int http_version_minor;
    HttpStatusCode code;
    std::string message;

    std::map<std::string, std::string> headers;
};

const char* http_status_code_str(HttpStatusCode code);

/*
    if success, return 1.
    if socket connection closed, return 0.
    if any error occurs, return -1, and you should check the errno.
*/
int send_all(int fd, const char* buf, size_t len) noexcept;

int send_all(int fd, const std::string& str) noexcept;

int send_template_html_reply(int fd, int http_version_major, int http_version_minor, HttpStatusCode code);

int send_response(int fd, const Response& response);

int send_chunked_data(int fd, const char* buf, size_t len);

int send_chunked_data(int fd, const std::string& str);

int send_chunked_end_flag(int fd);