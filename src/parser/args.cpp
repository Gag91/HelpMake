#include "hm/parser/parser.hpp"

void Parser::parseArgument(Config &config, std::string_view value) {

    std::map<std::string_view, bool &> argMap = {
        {"verbose", config.verbose},
        {"debug", config.debug},
        {"run", config.run},
        {"time", config.time},
        {"json", config.json},
        {"rebuild", config.rebuild},
        {"notify", config.notify},
        {"quiet", config.quiet},
        {"dry-run", config.dryRun},
        {"seperate", config.seperate},
        {"frozen", config.frozen},
    };

    for (auto &[arg, flag] : argMap) {
        if (is_typo(value, arg)) {
            cfg.v_Errors.push_back(std::format("[HelpMake] Error: Unknown argument '{}', did you mean '{}'?", value, arg));
        }
        if (similar(value, arg)) {
            flag = true;
        }
    }

    if (std::any_of(argMap.begin(), argMap.end(), [value](const auto &pair) {
            return pair.first == to_lower(value);
        })) {
        cfg.v_Errors.push_back(std::format("[HelpMake] Error: Unknown argument '{}'", value));
    }
}