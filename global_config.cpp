#include "global_config.h"
#include "utils.h"
#include "async_logger.h"
#include <fstream>

void GlobalConfig::mime_map_add(const std::string& extension, const std::string& mime) {
    mimeTable.emplace(utils::string_to_lower_case(extension), mime);
}

bool GlobalConfig::load_mime(const std::string& mimePath) {
    std::ifstream in{ mimePath };

    if (!in.is_open()) {
        return false;
    }

    std::string line;

    while (!in.eof()) {
        std::getline(in, line);

        if (!line.empty()) {
            auto splitList = utils::string_split_to_list(line, " ");

            if (splitList.size() != 2) {
                return false;
            }
            else {
                std::string& key = splitList[0];
                std::string& value = splitList[1];

                if (key.back() == '\n') {
                    key.pop_back();
                }

                if (key.back() == '\r') {
                    key.pop_back();
                }

                if (value.back() == '\n') {
                    value.pop_back();
                }

                if (value.back() == '\r') {
                    value.pop_back();
                }

                mime_map_add(key, value);
            }
        }
    }

    return true;
}

std::string GlobalConfig::extension_to_mime(const std::string& key) {
    auto tmp = utils::string_to_lower_case(key);
    auto iter = mimeTable.find(tmp);

    return iter == mimeTable.cend() ? "application/octet-stream" : iter->second;
}

void GlobalConfig::debug_mime() {
    for (const auto& e : mimeTable) {
        SIMPLE_LOG("%s: %s\n", e.first.c_str(), e.second.c_str());
    }
}