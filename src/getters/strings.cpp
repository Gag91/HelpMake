#include "hm/parser/parser.hpp"

std::string Parser::getCompiler() {
    return cfg.compiler;
}
std::string Parser::getOutput() {
    return cfg.output;
}
std::string Parser::getVersion() {
    return cfg.version;
}
std::string Parser::getKind() {
    return cfg.kind;
}
