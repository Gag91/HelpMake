#include "hm/generator/generator.hpp"
#include "hp/containers/containers.hpp"
#include "hp/other/other.hpp"

#include <cstdlib>
#include <format>
#include <memory>
#include <string_view>

namespace hm::gen {

    std::vector<std::string> availableGenerators() {
        return {"ninja", "cmake", ".clangd", "compile_commands.json"};
    }

    bool isKnownGenerator(std::string_view name) {
        if (hp::containsAny(availableGenerators(), name))
            return true;
        return false;
    }

    std::unique_ptr<Generator> makeGenerators(std::string_view name) {
        if (name == "ninja")
            return makeNinja();
        if (name == "cmake")
            return makeCMake();
        else {
            hp::printlnCl(std::format("[HelpMake] Invalid Generator '{}'.", name), hp::RED);
            std::cout << "\nAvaibles generators:\n";
            for (const auto &g : availableGenerators()) {
                std::cout << "- " << g << '\n';
            }
            exit(EXIT_FAILURE);
        }
    }

} // namespace hm::gen