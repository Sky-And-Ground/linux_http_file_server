#include "utils.h"
#include "str_splitter.h"
#include "custom_exception.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

std::string utils::get_current_time() {
    char buf[64];

    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);

#ifdef _WIN32
    struct tm tm_buf;
    localtime_s(&tm_buf, &tt);
    std::strftime(buf, sizeof(buf) / sizeof(char), "%Y-%m-%d %H:%M:%S", &tm_buf);
#else
    struct tm tm_buf;
    localtime_r(&tt, &tm_buf);
    std::strftime(buf, sizeof(buf) / sizeof(char), "%Y-%m-%d %H:%M:%S", &tm_buf);
#endif

    return std::string{ buf };
}

int utils::parse_port(const char* param) noexcept {
    int result = 0;

    while (*param != '\0') {
        if (result > 65535) {
            return -1;
        }

        if (isdigit(*param)) {
            result = 10 * result + (*param - '0');
        }
        else {
            return -1;
        }

        ++param;
    }

    if (result > 65535) {
        return -1;
    }

    return result;
}

std::vector<std::string> utils::string_split_to_list(const std::string& str, const std::string& pattern) {
    std::vector<std::string> vec;
    StrSplitter splitter{ str, pattern };

    vec.reserve(splitter.calc_slices_number(true));

    while (splitter.get_next()) {
        if (splitter.last_segment_len() > 0) {
            vec.emplace_back(splitter.last_segment(), splitter.last_segment_len());
        }
    }

    return vec;
}

std::string utils::string_to_lower_case(const std::string& str) {
    std::string tmp;
    tmp.reserve(str.length());

    for (char c : str) {
        tmp += std::tolower(c);
    }

    return tmp;
}

int utils::hex_to_decimal(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    else {
        return -1;
    }
}

std::string utils::decode_percent_encoding_url(const std::string& url) {
    std::string temp;
    temp.reserve(url.length());

    size_t i = 0;

    while (i < url.length()) {
        if (url[i] == '%') {
            if (i + 2 >= url.length()) {   // there must be 2 characters behind a '%'.
                THROW_EXCEPTION("invalid url format");
            }

            int p1 = hex_to_decimal(url[i + 1]);
            int p2 = hex_to_decimal(url[i + 2]);

            if (p1 >= 0 && p2 >= 0) {
                temp += static_cast<char>(16 * p1 + p2);
                i += 3;
            }
            else {
                THROW_EXCEPTION("invalid url format");
            }
        }
        else {
            temp += url[i];
            ++i;
        }
    }

    return temp;
}

void utils::reverse_string(std::string& str) {
    size_t len = str.length();

    for (size_t i = 0; i < len / 2; ++i) {
        std::swap(str[i], str[len - 1 - i]);
    }
}

std::string utils::double_to_str(double number, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << number;
    return oss.str();
}

std::string utils::integer_to_hex_str(size_t number) {
    return str_format("%zx", number);
}