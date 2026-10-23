#pragma once

#include <filesystem>

#include "hm/config.hpp"

namespace hm::cli {

    int build(int argc, char **argv, Config &config, const std::filesystem::path &filename);

    void help();
    void version();
    void license();
    void clean(bool gcm, bool all);
    void show(const std::filesystem::path &filename, Config &config);

} // namespace hm::cli