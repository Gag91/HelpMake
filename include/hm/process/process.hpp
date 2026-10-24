#pragma once

#include <filesystem>
#include <string>
#include <vector>


namespace hm {

    struct ProcessResult {
        int exitCode = 0;
        std::string output;
    };

    class Process {
      public:
        static ProcessResult run(const std::string &commandLine);
        static std::string join(const std::vector<std::string> &args);
        static std::string writeResponseFile(const std::vector<std::string> &args, const std::filesystem::path &rspPath);
    };

} // namespace hm