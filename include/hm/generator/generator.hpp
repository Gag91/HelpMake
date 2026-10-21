#pragma once

#include "hm/config.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hm::gen {

    class Generator {
      public:
        virtual ~Generator() = default;

        virtual std::string_view name() const = 0;
        virtual std::string_view Filename() const = 0;
        virtual std::string_view description() const = 0;

        virtual bool generate(const Config &cfg, const std::filesystem::path &projectRoot = ".") = 0;
    };

    std::vector<std::string> availableGenerators();
    std::unique_ptr<Generator> makeGenerators(std::string_view name);
    std::unique_ptr<Generator> makeNinja();
    std::unique_ptr<Generator> makeCMake();
    bool isKnownGenerator(std::string_view name);

} // namespace hm::gen