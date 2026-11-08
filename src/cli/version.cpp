#include "hm/cli/cli.hpp"

#include <iostream>
#include <string_view>

namespace hm::cli {

    int version() {
        constexpr std::string_view version = R"(Help_Make (hm) - Build System
Version: 12.4.3
Author : Xavi99
Website: https://gag91.github.io/HelpMake/
This program is licensed under the MIT License. See "--license" for details.
)";
        std::cout << version << "\n";
        return 0;
    }

    int Dumpversion() {
        constexpr std::string_view dumpversion = "12.4.3";
        std::cout << dumpversion;
        return 0;
    }

} // namespace hm::cli