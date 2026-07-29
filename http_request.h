#pragma once

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <functional>
#include "net_socket.h"

struct MultipartPart {
    std::string name;
    std::string filename;
    std::string content_type;
    std::string data;
};

struct Request {
    std::string method;
    std::string url;
    int http_version_major;
    int http_version_minor;
    std::map<std::string, std::string> headers;

    bool is_multipart_formdata = false;

    std::string content;
    std::vector<MultipartPart> multipart_formdata;

    bool headers_contains(const std::string& key);

    std::string headers_get(const std::string& key);
};

class RequestHeaderParser {
public:
    enum result_type {
        complete,
        bad,
        indeterminate
    };

    RequestHeaderParser() = default;

    // return result type, and the end parsed index.
    std::pair<result_type, size_t> parse(Request& req, const char* buf, size_t len);
private:
    // the asio http example's state machine.
    enum state_type {
        method_start,
        method,
        uri,
        http_version_h,
        http_version_t_1,
        http_version_t_2,
        http_version_p,
        http_version_slash,
        http_version_major_start,
        http_version_major,
        http_version_minor_start,
        http_version_minor,
        expecting_new_line_1,   // first line \r\n.
        header_line_start,
        header_name,
        space_before_header_value,
        header_value,
        expecting_new_line_2,   // headers line \r\n.
        expecting_new_line_3    // \r\n after headers.
    };

    state_type state = state_type::method_start;
    std::string tmpKey;
    std::string tmpValue;

    result_type consume_one(Request& req, char c);

    bool is_char(int c);
    bool is_control_char(int c);
    bool is_special(int c);
};

class RequestBodyParser {
    void parse_multipart_headers(MultipartPart& multipart, const std::string& headers);
    bool parse_multipart_formdata(Request& req, const std::string& boundary, const std::string& buf);
public:
    bool parse(Request& req, const std::string& buf);
};

using http_method_callback = std::function<void(int fd, const char* ip, int port, const Request& req)>;
