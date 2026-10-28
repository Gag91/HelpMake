#include "hm/cli/cli.hpp"
#include "hm/generator/generator.hpp"
#include "hm/parser/parser.hpp"
#include "hm/util/containers.hpp"
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

    bool verbose = false;
    for (int i = 2; i < argc; i++) {
        std::string_view a = argv[i];
        if (a == "--verbose" || a == "--debug")
            verbose = true;
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
        return hm::cli::clean(false, false, verbose);
    } else if (argument == "--clean-gcm") {
        return hm::cli::clean(true, false, verbose);
    } else if (argument == "--clean-all") {
        return hm::cli::clean(true, true, verbose);
    } else if (argument == "--update-lock") {
        config.updateLock = true;
        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            if (arg == "-f" || arg == "--file") {
                if (i + 1 >= argc || argv[i + 1][0] == '-') {
                    hp::printlnCl("[HelpMake] Error: filename not specified after '--file'.", hp::RED);
                    return 1;
                }
                filename = argv[++i];
            } else if (arg.rfind("-f=", 0) == 0 && arg.size() > 3) {
                filename = std::string(arg.substr(3));
            } else if (arg.rfind("--file=", 0) == 0 && arg.size() > 7) {
                filename = std::string(arg.substr(7));
            } else {
                hp::printlnCl(std::format("[HelpMake] Error: Invalid argument '{}' for --update-lock.", arg), hp::RED);
                return 1;
            }
        }
        Parser parser(filename, config);
        if (parser.parse() != 0)
            return 1;
        return parser.updateLock();
    } else if (argument == "--init") {
        Parser::Init init;
        init.run();
    } else if (argument == "--show") {
        hm::cli::show(filename, config);
    } else if (argument == "--watch") {
        config.watch = true;
        return hm::cli::build(argc, argv, config, filename);
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
    } else if (argument == "--check") {
        config.check = true;
        Parser parser(filename, config);
        if (parser.parse() != 0)
            return 1;

        const Config &parsed = parser.getConfig();

        std::cout << "Root:\n"
                  << "  Output:   " << parsed.output << '\n'
                  << "  Files:    " << expandInputs(parsed).size() << '\n'
                  << "  Compiler: " << parsed.compiler << '\n';
        if (!parsed.version.empty())
            std::cout << "  Version:  " << parsed.version << '\n';

        if (!parser.getConfigs().empty()) {
            std::cout << "\nConfigs:\n";
            for (const auto &[name, c] : parser.getConfigs()) {
                std::cout << "  " << name << ":\n";
                if (!c.output.empty())
                    std::cout << "    Output:   " << c.output << '\n';
                if (!c.compiler.empty())
                    std::cout << "    Compiler: " << c.compiler << '\n';
                if (!c.version.empty())
                    std::cout << "    Version:  " << c.version << '\n';
                if (c.kind == "static")
                    std::cout << "    Kind:     static\n";
                auto files = expandInputs(c);
                std::cout << "    Files:    " << files.size() << '\n';
                if (!c.v_Depends.empty()) {
                    std::cout << "    Deps:     ";
                    for (size_t i = 0; i < c.v_Depends.size(); i++) {
                        if (i)
                            std::cout << ", ";
                        std::cout << c.v_Depends[i];
                    }
                    std::cout << '\n';
                }
            }
        }

        std::cout << "\nDependencies: ";
        if (parser.dependencies.empty()) {
            std::cout << "(none)\n";
        } else {
            std::cout << '\n';
            for (const auto &[name, dep] : parser.dependencies) {
                std::cout << "  " << name;
                if (!dep.git.empty())
                    std::cout << " [git]";
                if (!dep.url.empty())
                    std::cout << " [url]";
                if (!dep.path.empty())
                    std::cout << " [path]";
                std::cout << '\n';
            }
        }

        hp::printlnCl("[HelpMake] ✓ HelpMake.txt is valid", hp::GREEN);
        return 0;
    } else if (argument == "test" || argument == "--test") {
        for (int i = 2; i < argc; ++i) {
            std::string_view arg = argv[i];
            if (arg == "--frozen") {
                config.frozen = true;
            } else if (arg == "--quiet") {
                config.quiet = true;
            } else if (arg == "--verbose") {
                config.verbose = true;
            } else if (arg == "-f" || arg == "--file") {
                if (i + 1 >= argc || argv[i + 1][0] == '-') {
                    hp::printlnCl("[HelpMake] Error: filename not specified after '--file'.", hp::RED);
                    return 1;
                }
                filename = argv[++i];
            } else if (arg.rfind("-f=", 0) == 0 && arg.size() > 3) {
                filename = std::string(arg.substr(3));
            } else if (arg.rfind("--file=", 0) == 0 && arg.size() > 7) {
                filename = std::string(arg.substr(7));
            } else {
                hp::printlnCl(std::format("[HelpMake] Error: Invalid argument '{}' for test.", arg), hp::RED);
                return 1;
            }
        }

        Parser parser(filename, config);
        if (parser.parse() != 0)
            return 1;

        std::vector<std::string> names;
        for (const auto &[name, cfg] : parser.getConfigs())
            if (cfg.test)
                names.push_back(name);

        if (names.empty()) {
            hp::printlnCl("[HelpMake] Test suite: 0 configs\n", hp::CYAN);
            hp::printlnCl("  No configs marked with Test: true.", hp::YELLOW);
            hp::printlnCl("  Add 'Test: true' to a Config block to include it.", hp::BRIGHT_BLACK);
            return 0;
        }

        if (!config.quiet)
            hp::printlnCl(std::format("[HelpMake] Test suite: {} config{}\n", names.size(), names.size() == 1 ? "" : "s"), hp::CYAN);

        int passed = 0;
        int failed = 0;
        double total = 0;
        std::vector<std::pair<std::string, std::string>> failures;

        for (const auto &name : names) {
            Config local = parser.inheritConfig(parser.getConfigs()[name]);
            local.quiet = true;
            parser.getConfig().quiet = true;
            auto t0 = hp::startTimer();
            int rc = parser.resolveDependencies(local, std::format("[HelpMake] Config '{}':", name));
            if (rc == 0)
                rc = parser.executeConfig(local, name);
            double dt = hp::stopTimer(t0);
            total += dt;

            bool ok = (rc == 0);
            if (ok)
                ++passed;
            else
                ++failed;

            if (!config.quiet) {
                std::string timeStr = std::format("{:.2f}s", dt);
                if (ok) {
                    hp::printlnCl(std::format("  {:<20} {}  {}", name, hp::getColorCode(hp::GREEN) + "PASS" + hp::getColorCode(hp::RESET), timeStr), hp::RESET);
                } else {
                    std::string reason = std::format("exit code {}", rc);
                    hp::printlnCl(std::format("  {:<20} {}  {}  {}", name, hp::getColorCode(hp::RED) + "FAIL" + hp::getColorCode(hp::RESET), reason, timeStr), hp::RESET);
                    failures.emplace_back(name, reason);
                }
            } else if (!ok) {
                failures.emplace_back(name, std::format("exit code {}", rc));
            }
        }

        hp::printlnCl("", hp::RESET);
        if (!config.quiet)
            hp::printlnCl("", hp::RESET);

        hp::Color summaryColor = (failed == 0) ? hp::GREEN : hp::RED;
        hp::printlnCl(std::format("  {} passed, {} failed, {:.2f}s total", passed, failed, total), summaryColor);

        if (!failures.empty()) {
            hp::printlnCl("\nFailures:", hp::RED);
            for (const auto &[name, reason] : failures)
                hp::printlnCl(std::format("  {:<20} {}", name, reason), hp::RED);

            hp::printlnCl("\n  See logs under build/HelpMake/*/logs/", hp::YELLOW);
        }

        return (failed == 0) ? 0 : 1;
    } else if (argument == "--build" || argument == "-b") {
        return hm::cli::build(argc, argv, config, filename);
    } else {
        hp::printlnCl("[HelpMake] Error: Invalid command. Use \"--help\" for available commands.", hp::RED);
    }

    return 0;
}