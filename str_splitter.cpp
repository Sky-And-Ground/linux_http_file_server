#include "str_splitter.h"

std::string::size_type StrSplitter::find_pattern_single_char() noexcept {
    if (data.str_len < data.pattern_len) {
        return std::string::npos;
    }

    // just let the compiler do loop unrolling.
    std::string::size_type i;
    for (i = data.current_pos; i < data.str_len; ++i) {
        if (data.str[i] == data.pattern[0]) {
            return i;
        }
    }

    return std::string::npos;
}

std::string::size_type StrSplitter::find_pattern_multi_chars() noexcept {
    if (data.pattern_len == 0 || data.str_len < data.pattern_len) {
        return std::string::npos;
    }

    std::string::size_type i, j;

    for (i = data.current_pos; i <= (data.str_len - data.pattern_len); ++i) {
        for (j = 0; j < data.pattern_len; ++j) {
            if (data.str[i + j] != data.pattern[j]) {
                break;
            }
        }

        if (j == data.pattern_len) {
            return i;
        }
    }

    return std::string::npos;
}

std::string::size_type StrSplitter::find_pattern() noexcept {
    return data.pattern_len == 1 ? find_pattern_single_char() : find_pattern_multi_chars();
}

bool StrSplitter::get_next() noexcept {
    if (data.current_pos >= data.str_len) {
        return false;
    }

    std::string::size_type pos = find_pattern();

    if (pos != std::string::npos) {
        data.last_segment = data.str + data.current_pos;
        data.last_segment_len = pos - data.current_pos;
        data.current_pos = pos + data.pattern_len;

        return true;
    }
    else {
        if (data.current_pos < data.str_len) {
            data.last_segment = data.str + data.current_pos;
            data.last_segment_len = data.str_len - data.current_pos;
            data.current_pos = data.str_len;

            return true;
        }
        else {
            return false;
        }
    }
}

size_t StrSplitter::calc_slices_number(bool onlyCaresAboutNotEmpty) noexcept {
    data_part tmp = data;
    size_t total = 0;

    if (onlyCaresAboutNotEmpty) {
        while (get_next()) {
            if (data.last_segment_len > 0) {
                ++total;
            }
        }
    }
    else {
        while (get_next()) {
            ++total;
        }
    }

    data = tmp;
    return total;
}
