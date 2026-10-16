#include "hm/cli/cli.hpp"
#include "hm/parser/parser.hpp"

#include "hp/other/other.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

int main(int argc, char *argv[]) {
    hp::enableUTF8();

    Config config;
    std::filesystem::path filename = "HelpMake.txt";

    if (argc < 2) {
        hm::cli::help();
        return 0;
    }

    std::string_view argument = argv[1];

    if (argument == "--help" || argument == "-h") {
        hm::cli::help();
    } else if (argument == "--version" || argument == "-v") {
        hm::cli::version();
    } else if (argument == "--license" || argument == "-l") {
        hm::cli::license();
    } else if (argument == "--install" || argument == "install") {
        Parser parser(filename, config);
        parser.install();
    } else if (argument == "--dump") {
        if (std::filesystem::exists(filename)) {
            std::ifstream file(filename);
            std::cout << file.rdbuf() << "\n";
        }
    } else if (argument == "--clean") {
        hm::cli::clean(false, false);
    } else if (argument == "--clean-gcm") {
        hm::cli::clean(true, false);
    } else if (argument == "--clean-all") {
        hm::cli::clean(true, true);
    } else if (argument == "--init") {
        Parser::Init init;
        init.run();
    } else if (argument == "--show") {
        hm::cli::show(filename, config);
    } else if (argument == "--build" || argument == "-b") {
        hm::cli::build(argc, argv, config, filename);
    } else {
        hp::printlnCl("[HelpMake] Error: Invalid command. Use \"--help\" for available commands.", hp::Color::RED);
    }

    return 0;
}