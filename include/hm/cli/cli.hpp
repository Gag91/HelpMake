#pragma once

#include <filesystem>

#include "hm/config.hpp"

namespace hm::cli {

    int build(int argc, char **argv, Config &config, const std::filesystem::path &filename);
    int clean(bool gcm, bool all, bool verbose = false);

    int help();
    int version();
    int Dumpversion();
    int license();
    int show(const std::filesystem::path &filename, Config &config);

} // namespace hm::cli