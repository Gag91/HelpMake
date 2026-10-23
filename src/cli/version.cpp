#include "hm/cli/cli.hpp"

#include <iostream>
#include <string_view>

namespace hm::cli {

    void version() {
        constexpr std::string_view version = R"(Help_Make (hm) - Build System
<<<<<<< HEAD
Version: 12.0.0
=======
Version: 11.3.0
>>>>>>> 17828e543023d690a3f506a4c8225c3b4f1ca357
Author : Xavi99
Website: https://github.com/Gag91/Help_Make
This program is licensed under the MIT License. See "--license" for details.
)";
        std::cout << version << "\n";
    }

} // namespace hm::cli