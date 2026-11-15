#include "hm/process/process.hpp"
#include <array>
#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>

namespace hm {

    inline bool platformMatches(std::string_view tag) {
        if (tag == "win" || tag == "windows")
#ifdef _WIN32
            return true;
#else
            return false;
#endif
        if (tag == "linux")
#ifdef __linux__
            return true;
#else
            return false;
#endif
        if (tag == "mac" || tag == "darwin" || tag == "macos" || tag == "osx")
#ifdef __APPLE__
            return true;
#else
            return false;
#endif
        if (tag == "unix")
#ifndef _WIN32
            return true;
#else
            return false;
#endif
        if (tag == "x64" || tag == "x86_64" || tag == "amd64")
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
            return true;
#else
            return false;
#endif
        if (tag == "arm64" || tag == "aarch64")
#if defined(__aarch64__) || defined(_M_ARM64)
            return true;
#else
            return false;
#endif
        if (tag == "x86" || tag == "i386" || tag == "i686")
#if defined(__i386__) || defined(_M_IX86)
            return true;
#else
            return false;
#endif
        return false;
    }

    struct PlatformLine {
        bool conditional = false;
        bool matches = false;
        std::string_view tag;
        std::string_view body;
    };

    inline PlatformLine stripPlatformPrefix(std::string_view line, std::string_view currentCompiler = "") {
        PlatformLine r;
        r.body = line;

        std::size_t start = 0;
        while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start])))
            ++start;

        std::size_t colon = line.find(':', start);
        if (colon == std::string_view::npos || colon == start)
            return r;

        std::string_view rawTag = line.substr(start, colon - start);
        bool negated = false;
        if (rawTag.starts_with('!')) {
            negated = true;
            rawTag = rawTag.substr(1);
        }

        bool isProgramCheck = false;
        if (rawTag.starts_with('?')) {
            isProgramCheck = true;
            rawTag = rawTag.substr(1);
        }

        if (rawTag.empty())
            return r;

        std::string tagLower;
        tagLower.reserve(rawTag.size());
        for (char c : rawTag)
            tagLower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        static constexpr std::array<std::string_view, 17> platformTags = {
            "win", "windows", "linux", "mac", "darwin", "macos", "osx",
            "unix", "x64", "x86_64", "amd64", "arm64", "aarch64",
            "x86", "i386", "i686", "windows"};

        static constexpr std::array<std::string_view, 8> compilerTags = {
            "gcc", "g++", "clang", "clang++", "msvc", "cl", "zig_cc", "zig_cpp"};

        bool matched = false;

        if (isProgramCheck) {
            if (rawTag.find('.') != std::string_view::npos ||
                rawTag.find('/') != std::string_view::npos ||
                rawTag.find('\\') != std::string_view::npos) {
                matched = std::filesystem::exists(rawTag);
            } else {
                matched = hm::Process::exists(std::string(rawTag));
            }
        } else if (std::ranges::find(platformTags, tagLower) != platformTags.end()) {
            matched = platformMatches(tagLower);
        } else if (std::ranges::find(compilerTags, tagLower) != compilerTags.end()) {
            std::string compLower = std::string(currentCompiler);
            for (char &c : compLower)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            if (tagLower == "gcc") {
                matched = (compLower.ends_with("gcc") || compLower.ends_with("g++"));
            } else if (tagLower == "clang") {
                matched = (compLower.ends_with("clang") || compLower.ends_with("clang++"));
            } else if (tagLower == "msvc") {
                matched = (compLower.ends_with("msvc") || compLower.ends_with("cl") || compLower.ends_with("cl.exe"));
            } else {
                matched = (compLower.ends_with(tagLower));
            }
        } else {
            return r;
        }

        if (negated)
            matched = !matched;

        std::size_t afterColon = colon + 1;
        while (afterColon < line.size() && line[afterColon] == ' ')
            ++afterColon;
        if (afterColon >= line.size())
            return r;

        r.conditional = true;
        r.matches = matched;
        r.tag = rawTag;
        r.body = line.substr(afterColon);
        return r;
    }

} // namespace hm