#include "hm/cli/cli.hpp"

#include <iostream>
#include <string_view>

namespace hm::cli {

    void version() {
        constexpr std::string_view version = R"(Help_Make (hm) - Build System
Version: 12.3.0
Author : Xavi99
Website: https://github.com/Gag91/Help_Make
This program is licensed under the MIT License. See "--license" for details.
)";
        std::cout << version << "\n";
    }

} // namespace hm::cli