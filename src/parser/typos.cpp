#include "hm/parser/parser.hpp"

void Parser::parseTypos(std::size_t num, std::filesystem::path filename,
                        std::string_view line, std::string_view expected) {
    std::string_view trimmed = trim_view(line);
    if (trimmed.empty())
        return;

    std::string_view display = expected;
    if (display.ends_with(':'))
        display.remove_suffix(1);
    else if (display.ends_with('{'))
        display = trim_view(display.substr(0, display.size() - 1));

    if (is_typo(trimmed, expected)) {
        cfg.v_Errors.push_back(std::format("\n[HelpMake] Error: {}:{}: Found '{}'. Did You mean {} ?", filename.string(), num, trimmed, display));
    }
}