#pragma once

#include <vector>
#include <string>

namespace fs_ops {
    // collect all the file, dir paths, non-recursive, only collect names.
    std::vector<std::string> walk_dir(const std::string& path);

    bool is_dir(const std::string& path);

    bool is_regular_file(const std::string& path);

    bool file_size(const std::string& path, size_t& size);

    std::string get_file_extension(const std::string& path);
}