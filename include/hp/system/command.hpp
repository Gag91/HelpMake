#pragma once

#include <array>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>

#ifdef _WIN32
#define HP_POPEN _popen
#define HP_PCLOSE _pclose
#else
#define HP_POPEN popen
#define HP_PCLOSE pclose
#endif

namespace hp {

    inline std::string command(const std::string &command, bool std_err = true) {
        std::array<char, 4096> buffer;

        std::string cmd = command;
        if (std_err) {
            cmd += " 2>&1";
        }

        std::unique_ptr<FILE, decltype(&HP_PCLOSE)> pipe(HP_POPEN(cmd.c_str(), "r"), HP_PCLOSE);
        if (!pipe) {
            throw std::runtime_error("hp::command: Failed to open pipe for command: " + command);
        }

        std::string result;
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
            if (result.size() + std::strlen(buffer.data()) > 4096) {
                throw std::runtime_error("hp::command: Output exceeds buffer size for command: " + command);
            }
            result += buffer.data();
        }
        return result;
    }
} // namespace hp