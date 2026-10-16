#pragma once

#include <string>

struct Job {
    std::string file;
    std::string cmd;
    std::string logPath;
    bool failed = false;
};