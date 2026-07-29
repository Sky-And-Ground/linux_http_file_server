#include "http_request.h"
#include "reply.h"
#include "async_logger.h"
#include "utils.h"
#include <cctype>

bool Request::headers_contains(const std::string& key) {
    auto tmp = utils::string_to_lower_case(key);
    auto iter = headers.find(tmp);

    return iter != headers.cend();
}

std::string Request::headers_get(const std::string& key) {
    auto tmp = utils::string_to_lower_case(key);
    auto iter = headers.find(tmp);

    return iter != headers.cend() ? iter->second : "";
}

bool RequestHeaderParser::is_char(int c) {
    return c >= 0 && c <= 127;
}

bool RequestHeaderParser::is_control_char(int c) {
    return (c >= 0 && c <= 31) || (c == 127);
}

bool RequestHeaderParser::is_special(int c) {
    return c == '(' || c == ')' || c == '<' || c == '>' || c == '@'
        || c == ',' || c == ';' || c == ':' || c == '\\' || c == '"'
        || c == '/' || c == '[' || c == ']' || c == '?' || c == '='
        || c == '{' || c == '}' || c == ' ' || c == '\t';
}

// the asio's example http request parser state machine is really good, so this function just uses that idea.
RequestHeaderParser::result_type RequestHeaderParser::consume_one(Request& req, char c) {
    switch (state) {
        case state_type::method_start:
            if (!is_char(c) || is_control_char(c) || is_special(c)) {
                return bad;
            }
            else {
                state = state_type::method;
                req.method += c;
                return indeterminate;
            }
        case state_type::method:
            if (c == ' ') {
                state = state_type::uri;
                return indeterminate;
            }
            else if (!is_char(c) || is_control_char(c) || is_special(c)) {
                return bad;
            }
            else {
                req.method += c;
                return indeterminate;
            }
        case state_type::uri:
            if (c == ' ') {
                state = state_type::http_version_h;
                return indeterminate;
            }
            else if (is_control_char(c)) {
                return bad;
            }
            else {
                req.url += c;
                return indeterminate;
            }
        case state_type::http_version_h:
            if (c == 'H') {
                state = state_type::http_version_t_1;
                return indeterminate;
            }
            else {
                return bad;
            }
        case state_type::http_version_t_1:
            if (c == 'T') {
                state = state_type::http_version_t_2;
                return indeterminate;
            }
            else {
                return bad;
            }
        case state_type::http_version_t_2:
            if (c == 'T') {
                state = state_type::http_version_p;
                return indeterminate;
            }
            else {
                return bad;
            }
        case state_type::http_version_p:
            if (c == 'P') {
                state = state_type::http_version_slash;
                return indeterminate;
            }
            else {
                return bad;
            }
        case state_type::http_version_slash:
            if (c == '/') {
                req.http_version_major = 0;
                req.http_version_minor = 0;

                state = state_type::http_version_major_start;
                return indeterminate;
            }
            else {
                return bad;
            }
        case state_type::http_version_major_start:
            if (isdigit(c)) {
                req.http_version_major = 10 * req.http_version_major + (c - '0');
                state = state_type::http_version_major;
                return indeterminate;
            }
            else {
                return bad;
            }
        case state_type::http_version_major:
            if (c == '.') {
                state = state_type::http_version_minor_start;
                return indeterminate;
            }
            else if (isdigit(c)) {
                req.http_version_major = 10 * req.http_version_major + (c - '0');
                return indeterminate;
            }
            else {
                return bad;
            }
        case state_type::http_version_minor_start:
            if (isdigit(c)) {
                req.http_version_minor = 10 * req.http_version_minor + (c - '0');
                state = state_type::http_version_minor;
                return indeterminate;
            }
            else {
                return bad;
            }
        case state_type::http_version_minor:
            if (c == '\r') {
                state = state_type::expecting_new_line_1;
                return indeterminate;
            }
            else if (isdigit(c)) {
                req.http_version_minor = 10 * req.http_version_minor + (c - '0');
                return indeterminate;
            }
            else {
                return bad;
            }
        case state_type::expecting_new_line_1:
            if (c == '\n') {
                state = state_type::header_line_start;
                return indeterminate;
            }
            else {
                return bad;
            }
        case state_type::header_line_start:
            if (c == '\r') {
                state = state_type::expecting_new_line_3;
                return indeterminate;
            }
            else if (!is_char(c) || is_control_char(c) || is_special(c)) {
                return bad;
            }
            else {
                tmpKey += c;
                state = state_type::header_name;
                return indeterminate;
            }
        case state_type::header_name:
            if (c == ':') {
                state = state_type::space_before_header_value;
                return indeterminate;
            }
            else if (!is_char(c) || is_control_char(c) || is_special(c)) {
                return bad;
            }
            else {
                tmpKey += c;
                return indeterminate;
            }
        case state_type::space_before_header_value:
            if (c == ' ') {
                state = state_type::header_value;
                return indeterminate;
            }
            else {
                return bad;
            }
        case state_type::header_value:
            if (c == '\r') {
                req.headers.emplace(utils::string_to_lower_case(tmpKey), tmpValue);
                tmpKey.clear();
                tmpValue.clear();

                state = state_type::expecting_new_line_2;
                return indeterminate;
            }
            else if (is_control_char(c)) {
                return bad;
            }
            else {
                tmpValue += c;
                return indeterminate;
            }
        case state_type::expecting_new_line_2:
            if (c == '\n') {
                state = state_type::header_line_start;
                return indeterminate;
            }
            else {
                return bad;
            }
        case state_type::expecting_new_line_3:
            return c == '\n' ? complete : bad;
        default:
            return result_type::bad;
    }
}

std::pair<RequestHeaderParser::result_type, size_t> RequestHeaderParser::parse(Request& req, const char* buf, size_t len) {
    size_t i = 0;

    while (i < len) {
        result_type type = consume_one(req, buf[i]);
        ++i;

        if (type == complete || type == bad) {
            return std::make_pair(type, i);
        }
    }

    return std::make_pair(indeterminate, i);
}

void RequestBodyParser::parse_multipart_headers(MultipartPart& multipart, const std::string& headers) {
    std::vector<std::string> vec = utils::string_split_to_list(headers, "\r\n");

    for (const auto& line : vec) {
        if (line.find("Content-Type:") != std::string::npos) {
            size_t pos = line.find(":") + 1;

            while (pos < line.length() && line[pos] == ' ') {
                ++pos;
            }

            multipart.content_type = line.substr(pos);
        }
        else if (line.find("Content-Disposition:") != std::string::npos) {
            size_t name_pos = line.find("name=\"");

            if (name_pos != std::string::npos) {
                name_pos += 6;   // skip "name=\""

                size_t name_end = line.find("\"", name_pos);

                if (name_end != std::string::npos) {
                    multipart.name = line.substr(name_pos, name_end - name_pos);
                }
            }

            size_t filename_pos = line.find("filename=\"");

            if (filename_pos != std::string::npos) {
                filename_pos += 10;   // skip "filename=\""

                size_t filename_end = line.find("\"", filename_pos);

                if (filename_end != std::string::npos) {
                    multipart.filename = line.substr(filename_pos, filename_end - filename_pos);
                }
            }
        }
    }
}

bool RequestBodyParser::parse_multipart_formdata(Request& req, const std::string& boundary, const std::string& buf) {
    std::string boundary_line = "--" + boundary;
    std::string boundary_end = boundary_line + "--";

    size_t pos = 0;
    size_t boundary_pos = buf.find(boundary_line, pos);

    if (boundary_pos == std::string::npos) {
        return false;
    }

    pos = boundary_pos + boundary_line.length();

    if (pos + 1 < buf.length() && buf[pos] == '\r' && buf[pos + 1] == '\n') {
        pos += 2;
    }

    while (pos < buf.length()) {
        size_t header_end = buf.find("\r\n\r\n", pos);

        if (header_end == std::string::npos) {
            break;
        }

        std::string headers = buf.substr(pos, header_end - pos);

        size_t data_start = header_end + 4;   // skip "\r\n\r\n"
        size_t next_boundary_start = buf.find(boundary_line, data_start);

        if (next_boundary_start == std::string::npos) {
            break;
        }

        size_t data_end = next_boundary_start;
        
        while (data_end > data_start && (buf[data_end - 1] == '\r' || buf[data_end - 1] == '\n')) {
            --data_end;
        }

        std::string data_part = buf.substr(data_start, data_end - data_start);

        MultipartPart multipart;
        parse_multipart_headers(multipart, headers);
        multipart.data = std::move(data_part);

        req.multipart_formdata.emplace_back(std::move(multipart));

        // reach the end.
        if (next_boundary_start + boundary_line.length() + 2 <= buf.length()) {
            if (buf.compare(next_boundary_start, boundary_end.length(), boundary_end) == 0) {
                break;
            }
        }

        pos = next_boundary_start + boundary_line.length();

        if (pos + 1 < buf.length() && buf[pos] == '\r' && buf[pos + 1] == '\n') {
            pos += 2;
        }
    }

    return true;
}

bool RequestBodyParser::parse(Request& req, const std::string& buf) {
    if (req.headers_contains("Content-Type")) {
        std::string contentType = req.headers_get("Content-Type");

        if (contentType.find("multipart/form-data") != std::string::npos) {
            req.is_multipart_formdata = true;

            size_t boundary_pos = contentType.find("boundary=");

            if (boundary_pos == std::string::npos) {
                return false;
            }

            boundary_pos += 9;   // skip "boundary="
            size_t end_pos = boundary_pos;

            while (end_pos < contentType.length() && contentType[end_pos] != ';' && contentType[end_pos] != ' ') {
                end_pos++;
            }

            std::string boundary = contentType.substr(boundary_pos, end_pos - boundary_pos);
            return parse_multipart_formdata(req, boundary, buf);
        }
    }

    req.content = buf;
    return true;
}