#pragma once

#include <string>
#include <map>

class GlobalConfig {
    std::string ip;
    std::string rootPath;
    int port;
    std::map<std::string, std::string> mimeTable;
    size_t max_content_length = 5 * 1024 * 1024;

    GlobalConfig() = default;
public:
    static GlobalConfig& instance() {
        static GlobalConfig config;
        return config;
    }

    void set_ip(const std::string& _ip) {
        ip = _ip;
    }

    const std::string& get_ip() const noexcept {
        return ip;
    }

    void set_port(int _port) {
        port = _port;
    }

    int get_port() const noexcept {
        return port;
    }

    void set_root_path(const std::string& _rootPath) {
        rootPath = _rootPath;
    }

    size_t get_max_content_length() const noexcept {
        return max_content_length;
    }

    void set_max_content_length(size_t length) {
        max_content_length = length;
    }

    const std::string& get_root_path() const noexcept {
        return rootPath;
    }

    bool load_mime(const std::string& mimePath);

    void mime_map_add(const std::string& extension, const std::string& mime);

    void debug_mime();

    // this function ignore cases, if can't find, return application/octet-stream
    std::string extension_to_mime(const std::string& key);
};
