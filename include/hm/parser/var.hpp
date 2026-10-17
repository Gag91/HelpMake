#pragma once

#include <map>
#include <string>
#include <vector>

namespace hm {
    class Var {
      private:
        std::map<std::string, std::string> variables;
        std::string tag;

        bool isConfig;

      public:
        Var() = default;

        std::string parseVar(const std::string &line, std::vector<std::string> &errors);

        void defineVar(const std::string &key, const std::string &value);
    };
} // namespace hm