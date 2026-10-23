#include "hm/parser/parser.hpp"
#include "hp/time/time.hpp"
#include <filesystem>

void Parser::watch() {
    auto collectFiles = [&]() {
        std::vector<std::string> files = expandInputs(cfg);

        std::error_code ec;
        for (const auto &inc : cfg.includeFiles) {
            if (!std::filesystem::is_directory(inc, ec))
                continue;
            for (auto &f : std::filesystem::recursive_directory_iterator(inc, ec))
                if (f.is_regular_file())
                    files.push_back(f.path().string());
        }

        files.push_back(filename.string());
        return files;
    };

    auto snapshot = [&]() {
        std::map<std::string, std::filesystem::file_time_type> s;
        std::error_code ec;
        for (const auto &f : collectFiles()) {
            auto t = std::filesystem::last_write_time(f, ec);
            if (!ec)
                s[f] = t;
        }
        return s;
    };

    auto prev = snapshot();
    while (true) {
        hp::wait_ms(300);
        auto curr = snapshot();
        if (curr == prev)
            continue;
        prev = curr;
        execute();
    }
}