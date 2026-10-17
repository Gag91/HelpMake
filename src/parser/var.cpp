#include "hm/parser/var.hpp"

#include <cstddef>
#include <format>

namespace hm {
    void Var::defineVar(const std::string &key, const std::string &value) {
        variables[key] = value;
    }

    std::string Var::parseVar(const std::string &line, std::vector<std::string> &errors) {
        std::string out;
        out.reserve(line.size());

        std::size_t i = 0;
        while (i < line.size()) {
            if (i + 1 < line.size() && line[i] == '[' && line[i + 1] == '[') {
                if (i + 3 < line.size() && line[i + 2] == '[' && line[i + 3] == '[') {
                    out += "[[";
                    i += 4;
                    continue;
                }

                std::size_t end = line.find("]]", i + 2);
                if (end == std::string::npos) {
                    errors.push_back("[HelpMake] Error: Unterminated variable. Missing \"]]\"");
                    break;
                }

                std::string name = line.substr(i + 2, end - i - 2);

                auto it = variables.find(name);
                if (it != variables.end()) {
                    out += it->second;
                } else {
                    errors.push_back(std::format("[HelpMake] Error: Undefined variable \"[[{}]]\"", name));
                    break;
                }

                i = end + 2;
                continue;
            }
            out += line[i];
            ++i;
        }

        return out;
    }
} // namespace hm