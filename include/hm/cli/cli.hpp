#pragma once

#include <filesystem>

#include "hm/config.hpp"

namespace hm::cli {

    void help();
    void version();
    void license();
    void clean(bool gcm, bool all);
    void show(const std::filesystem::path &filename, const Config &config);
    void build(int argc, char **argv, Config &config, const std::filesystem::path &filename);

} // namespace hm::cli