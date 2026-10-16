#pragma once

#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "hm/util/strings.hpp"
#include "hp/colors/color.hpp"
#include "hp/system/command.hpp"

inline constexpr std::array<std::string_view, 2> validKinds = {
    "executable",
    "static"};

inline bool similar(std::string_view first, std::string_view second) {
    if (first.size() != second.size())
        return false;
    if (first == second)
        return true;

    for (std::size_t i = 0; i < first.size(); i++) {
        if (static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(first[i]))) !=
            static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(second[i]))))
            return false;
    }
    return true;
}

inline bool is_typo(std::string_view str1, std::string_view str2) {
    if (str1 == str2)
        return false;

    std::string a = to_lower(str1);
    std::string b = to_lower(str2);

    if (a == b)
        return true;

    if (a.size() == b.size()) {
        int errors = 0;
        for (std::size_t i = 0; i < a.size(); ++i)
            if (a[i] != b[i] && ++errors >= 2)
                return false;
        return errors == 1;
    }

    if (a.size() > b.size())
        std::swap(a, b);
    if (b.size() != a.size() + 1)
        return false;

    std::size_t i = 0, j = 0;
    bool skipped = false;
    while (i < a.size() && j < b.size()) {
        if (a[i] != b[j]) {
            if (skipped)
                return false;
            skipped = true;
            ++j;
        } else {
            ++i;
            ++j;
        }
    }
    return true;
}

inline bool isDeclaredModule(const std::string &file, const std::vector<std::string> &modules) {
    for (const auto &m : modules) {
        if (std::filesystem::path(m).lexically_normal() == std::filesystem::path(file).lexically_normal())
            return true;
    }
    return false;
}

inline bool isModule(const std::string_view &s) {
    return s.ends_with(".cppm") || s.ends_with(".ixx") || s.ends_with(".mpp") ||
           s.ends_with(".cxxm") || s.ends_with(".c++m") || s.ends_with(".ccm");
}

inline bool gitAvailable() {
#ifdef _WIN32
    return hp::command("where git").find("INFO:") == std::string::npos;
#else
    return std::system("which git >/dev/null 2>&1") == 0;
#endif
}

inline bool isValidKind(std::string_view key) {
    std::string k = to_lower(key);

    for (auto v : validKinds) {
        if (v == k)
            return true;
        if (is_typo(k, v)) {
            hp::printlnCl(std::format("[HelpMake] Error: Invalid Kind '{}'. Did you mean {} ?", k, v), hp::RED);
            std::exit(EXIT_FAILURE);
        }
    }
    return false;
}