#include "hm/generator/generator.hpp"
#include "hp/containers/containers.hpp"
#include "hp/other/other.hpp"

#include <cstdlib>
#include <format>
#include <memory>
#include <string_view>

namespace hm::gen {

    std::vector<std::string> availableGenerators() {
        return {"ninja"};
    }

    bool isKnownGenerator(std::string_view name) {
        if (hp::containsAny(availableGenerators(), name))
            return true;
        return false;
    }

    std::unique_ptr<Generator> makeGenerators(std::string_view name) {
        if (name == "ninja")
            return makeNinja();
        else {
            hp::printlnCl(std::format("[HelpMake] Invalid Generator '{}'.\nAvaibles generators:", name), hp::RED);
            hp::printlnAll(availableGenerators());
            exit(EXIT_FAILURE);
        }
    }

} // namespace hm::gen