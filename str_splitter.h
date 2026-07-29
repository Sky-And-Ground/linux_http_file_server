#pragma once

#include <string>

class StrSplitter {
    struct data_part {
        const char* str;
        std::string::size_type str_len;

        const char* pattern;
        std::string::size_type pattern_len;

        std::string::size_type current_pos;

        const char* last_segment;
        std::string::size_type last_segment_len;

        data_part(const char* _str, std::string::size_type _str_len, const char* _pattern, std::string::size_type _pattern_len)
            : str{ _str }, str_len{ _str_len }, pattern{ _pattern }, pattern_len{ _pattern_len }, current_pos{ 0 }, last_segment{ str }, last_segment_len{ 0 }
        {
        }
    };

    data_part data;

    std::string::size_type find_pattern_single_char() noexcept;

    std::string::size_type find_pattern_multi_chars() noexcept;

    std::string::size_type find_pattern() noexcept;
public:
    StrSplitter(const char* str, std::string::size_type str_len, const char* pattern, std::string::size_type pattern_len)
        : data{ str, str_len, pattern, pattern_len }
    {
    }

    StrSplitter(const std::string& _str, const std::string& _pattern)
        : StrSplitter{ _str.c_str(), _str.length(), _pattern.c_str(), _pattern.length() }
    {
    }

    bool get_next() noexcept;

    size_t calc_slices_number(bool onlyCaresAboutNotEmpty) noexcept;

    const char* last_segment() const noexcept {
        return data.last_segment;
    }

    std::string::size_type last_segment_len() const noexcept {
        return data.last_segment_len;
    }
};