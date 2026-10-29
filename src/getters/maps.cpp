#include "hm/parser/parser.hpp"

std::map<std::string, Config> &Parser::getConfigs() {
    return configs;
}

std::map<std::string, std::string> &Parser::getVars() {
    return cfg.var.getVariables();
}
