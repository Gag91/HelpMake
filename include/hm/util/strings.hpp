#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <string>
#include <string_view>

#include "hm/config.hpp"
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

inline std::string stripSource(std::string_view src) {
    std::string out;
    out.reserve(src.size());
    bool inBlock = false;
    bool inStr = false;
    bool inChar = false;
    bool escaped = false;
    char quote = 0;

    for (std::size_t i = 0; i < src.size(); ++i) {
        char ch = src[i];
        if (inBlock) {
            if (ch == '*' && i + 1 < src.size() && src[i + 1] == '/') {
                inBlock = false;
                ++i;
            } else if (ch == '\n') {
                out += '\n';
            }
            continue;
        }
        if (inStr || inChar) {
            if (escaped) {
                escaped = false;
                continue;
            }
            if (ch == '\\') {
                escaped = true;
                continue;
            }
            if (ch == quote) {
                inStr = inChar = false;
            } else if (ch == '\n') {
                inStr = inChar = false;
                out += '\n';
            }
            continue;
        }
        if (ch == '/' && i + 1 < src.size()) {
            if (src[i + 1] == '/') {
                while (i < src.size() && src[i] != '\n')
                    ++i;
                if (i < src.size())
                    out += '\n';
                continue;
            }
            if (src[i + 1] == '*') {
                inBlock = true;
                ++i;
                continue;
            }
        }
        if (ch == '"' || ch == '\'') {
            quote = ch;
            inStr = (ch == '"');
            inChar = (ch == '\'');
            continue;
        }
        out += ch;
    }
    return out;
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

inline void print_info(const Config &c, const std::map<std::string, hp::Color> &info) {
    if (info.empty() || c.quiet)
        return;

    std::vector<std::string> errors, warnings, successes, infos, debugs;

    for (const auto &el : info) {
        if (el.second == hp::RED)
            errors.push_back(el.first);
        else if (el.first.find("[Debug]") != std::string::npos && c.debug)
            debugs.push_back(el.first);
        else if (el.second == hp::YELLOW)
            warnings.push_back(el.first);
        else if (el.second == hp::GREEN && c.verbose)
            successes.push_back(el.first);
        else
            infos.push_back(el.first);
    }

    struct Group {
        std::string name;
        std::string sym;
        hp::Color color;
        std::vector<std::string> *items;
    };

    std::vector<Group> groups;
    if (!errors.empty())
        groups.push_back({"Errors", "✘", hp::RED, &errors});
    if (!warnings.empty())
        groups.push_back({"Warnings", "⚠", hp::YELLOW, &warnings});
    if (!successes.empty())
        groups.push_back({"Generated Files", "✔", hp::BRIGHT_GREEN, &successes});
    if (!infos.empty())
        groups.push_back({"Info", "·", hp::CYAN, &infos});
    if (!debugs.empty())
        groups.push_back({"Debug Logs", "·", hp::BRIGHT_BLACK, &debugs});

    if (groups.empty())
        return;

    hp::printlnCl("\nInfo:", hp::CYAN);

    for (std::size_t g = 0; g < groups.size(); ++g) {
        std::cout << '\n';
        hp::printlnCl(std::format("  {}  {}", groups[g].sym, groups[g].name), groups[g].color);

        const auto &items = *groups[g].items;
        for (std::size_t j = 0; j < items.size(); ++j) {
            bool isLast = (j == items.size() - 1);
            std::string text = items[j];
            while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
                text.pop_back();
            if (text.starts_with("[HelpMake] "))
                text.erase(0, 11);

            std::cout << hp::getColorCode(hp::BRIGHT_BLACK)
                      << "     " << (isLast ? "└─ " : "├─ ")
                      << hp::getColorCode(groups[g].color)
                      << text
                      << hp::getColorCode(hp::RESET) << '\n';
        }
    }
    std::cout << '\n';
}

template <typename Container, typename Formatter>
    requires std::is_convertible_v<std::invoke_result_t<Formatter, typename Container::value_type>, std::string>
void print_tree(std::string_view title, const Container &items, hp::Color color, Formatter formatter) {
    if (items.empty())
        return;
    if (!title.empty())
        hp::printlnCl(color, title);

    std::size_t i = 0, total = items.size();
    for (const auto &item : items) {
        bool isLast = (i == total - 1);
        std::cout << hp::getColorCode(hp::BRIGHT_BLACK)
                  << "  " << (isLast ? "└─ " : "├─ ")
                  << hp::getColorCode(color)
                  << formatter(item)
                  << hp::getColorCode(hp::RESET) << '\n';
        ++i;
    }
    std::cout << '\n';
}

template <typename Container, typename Formatter>
    requires std::is_void_v<std::invoke_result_t<Formatter, typename Container::value_type>>
void print_tree(std::string_view title, const Container &items, Formatter formatter) {
    if (items.empty())
        return;
    if (!title.empty())
        std::cout << title << '\n';

    std::size_t i = 0, total = items.size();
    for (const auto &item : items) {
        bool isLast = (i == total - 1);
        std::cout << hp::getColorCode(hp::BRIGHT_BLACK)
                  << "  " << (isLast ? "└─ " : "├─ ")
                  << hp::getColorCode(hp::RESET);
        formatter(item);
        std::cout << hp::getColorCode(hp::RESET) << '\n';
        ++i;
    }
    std::cout << '\n';
}

template <typename Container>
void print_tree(std::string_view title, const Container &items, hp::Color color) {
    print_tree(title, items, color, [](const auto &s) { return std::string(s); });
}