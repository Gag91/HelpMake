#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace hm {
    class Var {
      private:
        std::map<std::string, std::string> variables;
        std::string tag;

        bool isConfig;

      public:
        Var() = default;

        std::string parseVar(std::string_view line, std::vector<std::string> &errors);
        bool isValidVar(std::string_view var) const;

        void defineVar(const std::string &key, const std::string &value);
    };
} // namespace hm