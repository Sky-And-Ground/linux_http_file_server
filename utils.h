#pragma once

#include <string>
#include <vector>

namespace utils {
    template<typename ... Args>
    std::string str_format(const char* fmt, Args ... args) {
        int need_len = std::snprintf(nullptr, 0, fmt, args...);

        if (need_len <= 0) {
            return "";
        }

        std::string buf(need_len, '\0');
        std::snprintf(&(buf[0]), need_len + 1, fmt, args...);
        return buf;
    }

    std::string get_current_time();

    int parse_port(const char* param) noexcept;

    std::vector<std::string> string_split_to_list(const std::string& str, const std::string& pattern);

    std::string string_to_lower_case(const std::string& str);

    int hex_to_decimal(char c);

    std::string decode_percent_encoding_url(const std::string& url);

    void reverse_string(std::string& str);

    std::string double_to_str(double number, int precision);

    std::string integer_to_hex_str(size_t number);
}