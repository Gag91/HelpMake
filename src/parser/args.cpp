#include "hm/parser/parser.hpp"

void Parser::parseArgument(Config &config, std::string_view value) {

    if (similar(value, "Verbose")) {
        config.verbose = true;
    } else if (similar(value, "debug")) {
        config.debug = true;
    } else if (similar(value, "Run")) {
        config.run = true;
    } else if (similar(value, "time")) {
        config.time = true;
    } else if (similar(value, "json")) {
        config.json = true;
    } else if (similar(value, "rebuild")) {
        config.rebuild = true;
    } else if (similar(value, "notify")) {
        config.notify = true;
    } else if (similar(value, "quiet")) {
        config.quiet = true;
    }
}