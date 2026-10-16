#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include "hm/config.hpp"

inline std::vector<std::string> expandInputs(const Config &c) {
    std::vector<std::string> expanded;

    for (const auto &entry : c.v_inputFiles) {
        bool hasRecursive = entry.find("**") != std::string::npos;
        bool hasGlob = entry.find('*') != std::string::npos;

        if (hasRecursive) {
            std::size_t starPos = entry.find("**");
            std::filesystem::path folder = entry.substr(0, starPos);
            if (folder.empty())
                folder = ".";

            std::string extension = std::filesystem::path(entry).extension().string();

            if (std::filesystem::exists(folder)) {
                for (const auto &f : std::filesystem::recursive_directory_iterator(folder)) {
                    if (!f.is_regular_file())
                        continue;
                    if (!extension.empty() && f.path().extension() != extension)
                        continue;
                    expanded.push_back(f.path().generic_string());
                }
            }
        } else if (hasGlob) {
            std::filesystem::path p(entry);
            std::string dir = p.parent_path().string();
            if (dir.empty())
                dir = ".";

            std::string extension = p.extension().string();
            if (extension.empty()) {
                expanded.push_back(entry);
                continue;
            }

            if (std::filesystem::exists(dir)) {
                for (const auto &f : std::filesystem::directory_iterator(dir)) {
                    if (!f.is_regular_file())
                        continue;
                    if (f.path().extension() != extension)
                        continue;
                    expanded.push_back(f.path().generic_string());
                }
            }
        } else {
            expanded.push_back(entry);
        }
    }

    for (const auto &m : c.v_Modules)
        expanded.push_back(m);

    std::sort(expanded.begin(), expanded.end());
    expanded.erase(std::unique(expanded.begin(), expanded.end()), expanded.end());

    return expanded;
}