#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct Job {
    std::string file;
    std::string logPath;
    std::string responseFilePath;

    std::size_t responseArgStart = 0;
    std::vector<std::string> args;

    bool failed = false;
};