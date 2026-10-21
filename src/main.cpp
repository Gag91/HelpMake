#include "hm/cli/cli.hpp"
#include "hm/generator/generator.hpp"
#include "hm/parser/parser.hpp"
#include "hp/colors/color.hpp"
#include "hp/containers/containers.hpp"
#include "hp/other/other.hpp"

#include <filesystem>
#include <format>
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
    } else if (argument == "--gen" || argument == "--generate") {
        if (argc < 3) {
            hp::printlnCl("[HelpMake] Available generators:", hp::CYAN);
            hp::printlnAll(hm::gen::availableGenerators());
            return 0;
        }

        std::string genName = argv[2];
        std::string configName;

        for (int i = 3; i < argc; i++) {
            std::string_view a = argv[i];
            if (a == "--config" && i + 1 < argc)
                configName = argv[++i];
            else if (a.rfind("--config=", 0) == 0 && a.size() > 9)
                configName = std::string(a).substr(9);
            else
                hp::printlnCl(std::format("[HelpMake] Warning: ignored '{}' for --gen", a), hp::YELLOW);
        }

        Parser parser(filename, config);
        if (parser.parse() != 0) {
            hp::printlnCl("[HelpMake] Failed to parse HelpMake.txt.", hp::RED);
            return 1;
        }

        if (genName == "json" || genName == "compile_commands") {
            Config toGen;
            if (!configName.empty()) {
                auto &all = parser.getConfigs();
                auto it = all.find(configName);
                if (it == all.end()) {
                    hp::printlnCl(std::format("[HelpMake] Unknown config '{}'. Available:", configName), hp::RED);
                    for (const auto &[n, _] : all)
                        std::cout << "  " << n << "\n";
                    return 1;
                }
                toGen = parser.inheritConfig(it->second);
            } else {
                toGen = parser.getConfig();
            }

            if (parser.generateCompileCommands(toGen) == 0) {
                hp::printlnCl("[HelpMake] Generated compile_commands.json", hp::GREEN);
                return 0;
            }
            hp::printlnCl("[HelpMake] Error: Failed to generate compile_commands.json", hp::RED);
            return 1;
        }

        auto gen = hm::gen::makeGenerators(genName);
        if (!gen) {
            hp::printlnCl(std::format("[HelpMake] Unknown generator '{}'.", genName), hp::RED);
            return 1;
        }

        Config toGen;
        if (!configName.empty()) {
            auto &all = parser.getConfigs();
            auto it = all.find(configName);
            if (it == all.end()) {
                hp::printlnCl(std::format("[HelpMake] Unknown config '{}'. Available:", configName), hp::RED);
                for (const auto &[n, _] : all)
                    std::cout << "  " << n << "\n";
                return 1;
            }
            toGen = parser.inheritConfig(it->second);
        } else {
            toGen = parser.getConfig();
        }

        if (gen->generate(toGen, ".")) {
            hp::printlnCl(std::format("[HelpMake] {} generated.", gen->Filename()), hp::GREEN);
            return 0;
        }

        hp::printlnCl(std::format("[HelpMake] Failed to generate {}.", gen->Filename()), hp::RED);
        return 1;
    } else if (argument == "--build" || argument == "-b") {
        hm::cli::build(argc, argv, config, filename);
    } else {
        hp::printlnCl("[HelpMake] Error: Invalid command. Use \"--help\" for available commands.", hp::RED);
    }

    return 0;
}