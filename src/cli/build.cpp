#include "hm/cli/cli.hpp"
#include "hm/parser/parser.hpp"
#include "hm/util/strings.hpp"

#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"

#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace hm::cli {

    void build(int argc, char **argv, Config &config, const std::filesystem::path &filename) {
        std::vector<std::string> selectedConfigs;

        for (int i = 2; i < argc; i++) {
            std::string_view arg = argv[i];

            if (arg == "-Gcc" || arg == "-gcc") {
                config.compiler = "gcc";
            } else if (arg == "-Clang" || arg == "-clang") {
                config.compiler = "clang";
            } else if (arg == "-MSVC" || arg == "-msvc") {
                config.compiler = "msvc";
            } else if (arg == "-Zig" || arg == "-zig") {
                config.compiler = "zig";
            } else if (arg == "-sere" || arg == "-Sere") {
                config.compiler = "sere";
                config.buildSere = true;
            } else if (arg.rfind("-C=", 0) == 0) {
                std::string value = std::string(arg).substr(3);
                if (value.empty()) {
                    if (i + 1 < argc && argv[i + 1][0] != '-')
                        value = argv[++i];
                    else {
                        hp::printlnCl("[HelpMake] Error: Compiler not specified after '-C='.", hp::RED);
                        std::exit(EXIT_FAILURE);
                    }
                }
                config.compiler = value;
                if (config.debug)
                    hp::printlnCl(std::format("[Debug] Special compiler: {}", config.compiler), hp::YELLOW);
            } else if (arg.rfind("-V=", 0) == 0) {
                std::string value = std::string(arg).substr(3);
                if (value.empty()) {
                    if (i + 1 < argc && argv[i + 1][0] != '-')
                        value = argv[++i];
                    else {
                        hp::printlnCl("[HelpMake] Error: Version not specified after '-V='.", hp::RED);
                        std::exit(EXIT_FAILURE);
                    }
                }
                if (value.front() != '-')
                    value.insert(value.begin(), '-');
                config.version = value;
                if (config.debug)
                    hp::printlnCl(std::format("[Debug] Special version: {}", config.version), hp::YELLOW);
            } else if (arg == "-o" || arg == "--output") {
                if (i + 1 < argc && argv[i + 1][0] != '-')
                    config.output = argv[++i];
                else {
                    hp::printlnCl("[HelpMake] Error: Output file not specified after '" + std::string(arg) + "'.", hp::RED);
                    std::exit(EXIT_FAILURE);
                }
            } else if (arg.rfind("-o", 0) == 0 && arg.size() > 2) {
                config.output = std::string(arg).substr(2);
            } else if (arg == "-F" || arg == "--Flag") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    if (!config.v_Flags.empty())
                        config.v_Flags.push_back(" ");
                    config.v_Flags.push_back(argv[++i]);
                } else {
                    hp::printlnCl("[HelpMake] Error: Additional flags not specified after '" + std::string(arg) + "'.", hp::RED);
                    std::exit(EXIT_FAILURE);
                }
            } else if (arg.rfind("-F", 0) == 0 && arg.size() > 2) {
                std::string n_flags = std::string(arg).substr(2);
                if (!config.v_Flags.empty())
                    config.flags += " ";
                config.flags += n_flags;
            } else if (arg == "-I" || arg == "--Include") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    if (!config.v_Flags.empty())
                        config.flags += " ";
                    config.flags += "-I" + std::string(argv[++i]);
                } else {
                    hp::printlnCl("[HelpMake] Error: Include path not specified after '" + std::string(arg) + "'.", hp::RED);
                    std::exit(EXIT_FAILURE);
                }
            } else if (arg.rfind("-I", 0) == 0 && arg.size() > 2) {
                std::string includePath = std::string(arg).substr(2);
                if (!config.v_Flags.empty())
                    config.flags += " ";
                config.flags += "-I" + includePath;
            } else if (arg == "-L" || arg == "--Library") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    if (!config.v_Flags.empty())
                        config.flags += " ";
                    config.flags += "-L" + std::string(argv[++i]);
                } else {
                    hp::printlnCl("[HelpMake] Error: Library path not specified after '" + std::string(arg) + "'.", hp::RED);
                    std::exit(EXIT_FAILURE);
                }
            } else if (arg.rfind("-L", 0) == 0 && arg.size() > 2) {
                std::string libPath = std::string(arg).substr(2);
                if (!config.v_Flags.empty())
                    config.flags += " ";
                config.flags += "-L" + libPath;
            } else if (arg == "-f" || arg == "--file") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    const_cast<std::filesystem::path &>(filename) = std::string(argv[++i]);
                } else {
                    hp::printlnCl("[HelpMake] Error: filename not specified after '" + std::string(arg) + "'.", hp::RED);
                    std::exit(EXIT_FAILURE);
                }
            } else if (arg == "-j" || arg == "--jobs") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    try {
                        config.jobs = std::stoul(argv[++i]);
                    } catch (...) {
                        hp::printlnCl(std::format("Error: Invalid value for '{}'", arg), hp::RED);
                        std::exit(EXIT_FAILURE);
                    }
                } else {
                    config.jobs = std::thread::hardware_concurrency();
                    if (config.jobs == 0)
                        config.jobs = 1;
                }
            } else if (arg.rfind("-j", 0) == 0 && arg.size() > 2) {
                try {
                    config.jobs = std::stoul(std::string(arg).substr(2));
                } catch (...) {
                    hp::printlnCl(std::format("Error: Invalid value for '-j{}'", std::string(arg).substr(2)), hp::RED);
                    std::exit(EXIT_FAILURE);
                }
            } else if (arg == "--config") {
                if (i + 1 < argc && argv[i + 1][0] != '-')
                    selectedConfigs.push_back(std::string(argv[++i]));
                else {
                    hp::printlnCl("[HelpMake] Error: Config name not specified after '" + std::string(arg) + "'.", hp::RED);
                    std::exit(EXIT_FAILURE);
                }
            } else if (arg.rfind("--config=", 0) == 0 && arg.size() > 9) {
                selectedConfigs.push_back(std::string(arg).substr(9));
            } else if (arg == "--Kind") {
                if (i + 1 < argc && argv[i + 1][0] != '-')
                    config.kind = to_lower(argv[++i]);
                else {
                    hp::printlnCl(std::format("[HelpMake] Error: Kind not specified after '{}'.", arg), hp::RED);
                    std::exit(EXIT_FAILURE);
                }
            } else if (arg.rfind("--Kind=", 0) == 0 && arg.size() > 9) {
                config.kind = to_lower(arg.substr(7));
            } else if (arg.rfind("--config-all", 0) == 0 && arg.size() > 9) {
                config.allCfgs = true;
            } else if (arg.rfind("-f", 0) == 0 && arg.size() > 2) {
                const_cast<std::filesystem::path &>(filename) = std::string(arg).substr(2);
            } else if (arg == "-v" || arg == "--verbose") {
                config.verbose = true;
            } else if (arg == "-r" || arg == "--run") {
                config.run = true;
            } else if (arg == "--var") {
                if (i + 1 >= argc) {
                    hp::printlnCl("[HelpMake] Error: --var requires NAME=VALUE", hp::RED);
                    exit(EXIT_FAILURE);
                }
                std::string rest = argv[++i];
                std::size_t pos = rest.find('=');
                if (pos == std::string::npos || pos == 0) {
                    hp::printlnCl(std::format("[HelpMake] Error: --var expects NAME=VALUE, got '{}'", rest), hp::RED);
                    exit(EXIT_FAILURE);
                }
                std::string key = trim(rest.substr(0, pos));
                std::string value = trim(rest.substr(pos + 1));
                config.v_Vars[key] = value;

            } else if (arg.rfind("--var", 0) == 0) {
                std::string rest = std::string(arg).substr(5);
                std::size_t pos = rest.find('=');
                if (pos == std::string::npos || pos == 0) {
                    hp::printlnCl(std::format("[HelpMake] Error: --var expects NAME=VALUE, got '{}'", arg), hp::RED);
                    exit(EXIT_FAILURE);
                }
                std::string key = trim(rest.substr(0, pos));
                std::string value = trim(rest.substr(pos + 1));
                config.v_Vars[key] = value;

            } else if (arg == "--debug") {
                config.debug = true;
            } else if (arg == "--nofile") {
                config.n_file = true;
            } else if (arg == "--create") {
                config.create = true;
            } else if (arg == "--json") {
                config.json = true;
            } else if (arg == "--no-sep") {
                config.seperate = false;
            } else if (arg == "--rebuild") {
                config.rebuild = true;
            } else if (arg == "--quiet") {
                config.quiet = true;
            } else if (arg == "--notify") {
                config.notify = true;
            } else if (arg == "--time") {
                config.time = true;
            } else if (!arg.empty() && (arg.back() == '/' || arg.back() == '\\')) {
                const_cast<std::filesystem::path &>(filename) = std::string(arg) + "HelpMake.txt";
                if (config.verbose)
                    std::cout << "Build file: " << filename << std::endl;
            } else if (arg.find(".txt") != std::string::npos && std::filesystem::is_regular_file(arg)) {
                const_cast<std::filesystem::path &>(filename) = std::string(arg);
                if (config.verbose)
                    std::cout << "Build file: " << filename << std::endl;
            } else if (std::filesystem::is_directory(arg)) {
                const_cast<std::filesystem::path &>(filename) = std::string(arg) + "/HelpMake.txt";
                if (config.verbose)
                    std::cout << "Build file: " << filename << std::endl;
            } else if (arg.rfind("-", 0) != 0) {
                if (!config.inputFile.empty())
                    config.inputFile += " ";
                config.inputFile += std::string(arg);
            } else {
                hp::printlnCl(std::format("[HelpMake] Error: Invalid Argument '{}'.\n Use --help for the available arguments", arg), hp::RED);
                exit(EXIT_FAILURE);
            }
        }

        if (!std::filesystem::exists(filename) && !config.n_file && !config.create) {
            if (filename != "HelpMake.txt" && std::filesystem::exists("HelpMake.txt")) {
                hp::printlnCl("Build file not found. Using default HelpMake.txt", hp::YELLOW);
                const_cast<std::filesystem::path &>(filename) = "HelpMake.txt";
            }
        }

        if (std::filesystem::exists(filename) && !config.n_file && !config.create) {
            Parser parser(filename, config);
            parser.parse();
            if (!config.create) {
                parser.executeConfigs(selectedConfigs);
                if (!selectedConfigs.empty() && config.allCfgs && !config.quiet)
                    hp::printlnCl("Warning: Both --config and --config-all specified. Ignoring --config.", hp::YELLOW);
                if (selectedConfigs.empty())
                    parser.execute();
            }
        } else {
            if (!config.n_file && !config.create)
                hp::printlnCl("Build file not found: " + filename.string(), hp::YELLOW);
            if (config.verbose)
                hp::printlnCl("Building from command-line arguments only\n", hp::CYAN);

            if (config.compiler.empty()) {
                hp::printlnCl("[HelpMake] Error: No compiler specified. Use -Gcc, -Clang, -MSVC, -Zig, -Sere or -C=<compiler>", hp::RED);
                std::exit(EXIT_FAILURE);
            }
            if (config.inputFile.empty()) {
                hp::printlnCl("[HelpMake] Error: No input files specified.", hp::RED);
                std::exit(EXIT_FAILURE);
            }
            if (config.output.empty()) {
#ifdef _WIN32
                config.output = "a.exe";
#else
                config.output = "a.out";
#endif
                if (!config.quiet)
                    hp::printlnCl("Warning: No output file specified. Using default: " + config.output, hp::YELLOW);
            }

            Parser parser(filename, config);
            parser.parse();
            if (!config.create)
                parser.execute();
        }
    }

} // namespace hm::cli