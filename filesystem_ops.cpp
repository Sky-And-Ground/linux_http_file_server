#include "filesystem_ops.h"
#include "custom_exception.h"
#include <dirent.h>
#include <cstring>
#include <sys/stat.h>

class WalkDirHandle {
    DIR* dir;
public:
    WalkDirHandle() : dir{ nullptr } {}

    ~WalkDirHandle() {
        close();
    }

    WalkDirHandle(const WalkDirHandle&) = delete;
    WalkDirHandle& operator=(const WalkDirHandle&) = delete;

    WalkDirHandle(WalkDirHandle&& other) : dir{ other.dir } {
        other.dir = nullptr;
    }

    WalkDirHandle& operator=(WalkDirHandle&& other) {
        if (&other != this) {
            close();

            dir = other.dir;
            other.dir = nullptr;
        }

        return *this;
    }

    bool open(const std::string& path) {
        dir = opendir(path.c_str());
        return dir != nullptr;
    }

    void close() noexcept {
        if (dir) {
            closedir(dir);
        }
    }

    DIR* get() noexcept {
        return dir;
    }
};

std::vector<std::string> fs_ops::walk_dir(const std::string& path) {
    std::vector<std::string> vec;

    WalkDirHandle handle;
    if (!handle.open(path)) {
        return vec;
    }

    struct dirent* d;

    while ((d = readdir(handle.get())) != nullptr) {
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0) {
            continue;
        }

        if (d->d_type == DT_DIR || d->d_type == DT_REG) {
            vec.emplace_back(d->d_name);
        }
    }

    return vec;
}

bool fs_ops::is_dir(const std::string& path) {
    struct stat st;
    
    if (stat(path.c_str(), &st) == -1) {
        return false;
    }

    return S_ISDIR(st.st_mode);
}

bool fs_ops::is_regular_file(const std::string& path) {
    struct stat st;
    
    if (stat(path.c_str(), &st) == -1) {
        return false;
    }

    return S_ISREG(st.st_mode);
}

bool fs_ops::file_size(const std::string& path, size_t& size) {
    struct stat st;
    if (stat(path.c_str(), &st) == -1) {
        return false;
    }

    size = st.st_size;
    return true;
}

std::string fs_ops::get_file_extension(const std::string& path) {
    size_t i = 0;

    while (i < path.length() && path[i] != '.') {
        ++i;
    }

    if (i != path.length()) {
        return path.substr(i + 1);
    }
    else {
        return "";
    }
}