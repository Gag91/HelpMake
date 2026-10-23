#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>

#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"

inline std::string to_lower(std::string_view str) {
    std::string result(str.data(), str.size());
    for (auto &c : result)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}

inline std::string_view trim_view(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.remove_suffix(1);
    return s;
}

inline std::string trim(const std::string &s) {
    std::size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    std::size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

inline std::string escapeJson(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char ch : s) {
        switch (ch) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += ch;
                break;
        }
    }
    return out;
}

inline std::string toUnixPath(const std::string &s) {
    std::string out = s;
    for (char &ch : out)
        if (ch == '\\')
            ch = '/';
    return out;
}

inline std::string extractValue(const std::string &line, const std::string &key) {
    std::size_t pos = line.find(key);
    if (pos == std::string::npos)
        return "";
    return trim(line.substr(pos + key.size()));
}

inline std::string extractModule(const std::filesystem::path &filename) {
    std::ifstream file(filename);
    if (!file) {
        hp::printlnCl(std::format("[HelpMake] Error: Could not open file '{}'", filename.string()), hp::RED);
        std::exit(EXIT_FAILURE);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("export module") == std::string::npos)
            continue;

        std::string value = extractValue(line, "export module");
        while (!value.empty() && (value.back() == ';' || std::isspace(static_cast<unsigned char>(value.back()))))
            value.pop_back();
        return trim(value);
    }

    return "";
}

inline std::string compilerCommand(const std::string &compiler) {
    if (compiler == "msvc")
        return "cl";
    if (compiler == "zig c++" || compiler == "zig-c++" || compiler == "zigcpp" || compiler == "zig-cpp")
        return "zig c++";
    if (compiler == "zig cc" || compiler == "zig-cc" || compiler == "zigcc" || compiler == "zig-c")
        return "zig cc";
    return compiler;
}

inline std::string replaceSlash(std::string_view s) {
    return std::filesystem::path{s}.generic_string();
}

inline std::string replaceSlash(std::string &s) {
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
}

inline bool isMsvcLike(std::string_view compiler) {
    std::string n = to_lower(compiler);
    return n == "msvc" || n == "cl" || n == "clang-cl";
}

inline std::string normalizeVersion(const std::string &version,
                                    std::string_view compiler) {
    if (version.empty() || version.front() == '-' || version.front() == '/')
        return version;
    return std::string(isMsvcLike(compiler) ? "/" : "-") + version;
}