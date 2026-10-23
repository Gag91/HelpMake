#include "hm/parser/var.hpp"
#include "hm/util/bools.hpp"

#include <cstddef>
#include <format>
#include <string_view>

namespace hm {

    bool Var::isValidVar(std::string_view var) const {
        for (const auto &[key, value] : variables) {
            if (key == var)
                return true;
            if (is_typo(key, var)) {
                hp::printlnCl(
                    std::format("[HelpMake] Error: Found '[[{}]]'. Did you mean '[[{}]]' ?", var, key),
                    hp::YELLOW);
            }
        }
        return false;
    }

    void Var::defineVar(const std::string &key, const std::string &value) {
        variables[key] = value;
    }

    std::string Var::parseVar(std::string_view line, std::vector<std::string> &errors) {
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

                std::string name = std::string(line.substr(i + 2, end - i - 2));

                auto it = variables.find(name);
                if (it != variables.end()) {
                    out += it->second;
                } else {
                    errors.push_back(std::format("[HelpMake] Error: Undefined variable \"[[{}]]\"", name));
                    out += "[[" + name + "]]";
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