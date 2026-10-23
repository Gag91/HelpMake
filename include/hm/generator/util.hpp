#pragma once

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace hm::gen {

    inline bool writeIfChanged(const std::filesystem::path &path,
                               const std::string &content) {
        if (std::filesystem::exists(path)) {
            std::ifstream existing(path, std::ios::binary);
            if (existing) {
                std::string old((std::istreambuf_iterator<char>(existing)),
                                std::istreambuf_iterator<char>());
                if (old == content)
                    return true;
            }
        }

        auto parent = path.parent_path();
        if (!parent.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(parent, ec);
        }

        std::ofstream out(path, std::ios::binary);
        if (!out)
            return false;
        out << content;
        return static_cast<bool>(out);
    }

} // namespace hm::gen