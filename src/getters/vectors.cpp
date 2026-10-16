#include "hm/parser/parser.hpp"

std::vector<std::string> Parser::getInputFile() {
    return cfg.v_inputFiles;
}
std::vector<std::string> Parser::getInclude() {
    return cfg.includeFiles;
}
std::vector<std::string> Parser::getFlags() {
    return cfg.v_Flags;
}
std::vector<std::string> Parser::getModules() {
    return cfg.v_Modules;
}
std::vector<std::string> Parser::getGithub() {
    return cfg.v_Github;
}
std::vector<std::string> Parser::getCopy() {
    return cfg.v_Copy;
}