#pragma once

#include <string>
#include <vector>

namespace hm {

    struct Dependency {
        std::vector<std::string> builds;
        std::vector<std::string> includes;
        std::vector<std::string> links;
        std::vector<std::string> linkDirs;
        std::vector<std::string> linkFlags;
        std::vector<std::string> depends;
        std::string git;
        std::string url;
        std::string tag;
        std::string name;
        std::string commit;
        std::string path;
        std::string hash;
    };

} // namespace hm
