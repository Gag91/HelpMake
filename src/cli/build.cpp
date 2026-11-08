#include "hm/parser/parser.hpp"
#include "hm/util/bools.hpp"
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

    struct ScalarFlag {
        std::string_view name;
        std::string Config::*field;
    };

    static constexpr std::array<ScalarFlag, 11> scalarFlags = {{
        {"compiler", &Config::compiler},
        {"version", &Config::version},
        {"output", &Config::output},
        {"flags", &Config::flags},
        {"includes", &Config::includes},
        {"kind", &Config::kind},
        {"modules", &Config::modules},
        {"input", &Config::inputFile},
        {"pre", &Config::Precmd},
        {"post", &Config::Postcmd},
        {"pch", &Config::pch},
    }};

    struct BoolFlag {
        std::string_view name;
        bool Config::*field;
    };

    static constexpr std::array<BoolFlag, 15> boolFlags = {{
        {"verbose", &Config::verbose},
        {"debug", &Config::debug},
        {"dry-run", &Config::dryRun},
        {"quiet", &Config::quiet},
        {"notify", &Config::notify},
        {"rebuild", &Config::rebuild},
        {"json", &Config::json},
        {"time", &Config::time},
        {"sep", &Config::seperate},
        {"watch", &Config::watch},
        {"run", &Config::run},
        {"nofile", &Config::n_file},
        {"create", &Config::create},
        {"config-all", &Config::allCfgs},
        {"frozen", &Config::frozen},
    }};

    struct ShortBoolFlag {
        char name;
        bool Config::*field;
    };

    static constexpr std::array<ShortBoolFlag, 13> shortBoolFlags = {{
        {'v', &Config::verbose},
        {'d', &Config::debug},
        {'q', &Config::quiet},
        {'r', &Config::rebuild},
        {'w', &Config::watch},
        {'j', &Config::json},
        {'t', &Config::time},
        {'n', &Config::notify},
        {'y', &Config::dryRun},
        {'c', &Config::create},
        {'x', &Config::n_file},
        {'a', &Config::allCfgs},
        {'z', &Config::frozen},
    }};

    struct ShortStringFlag {
        char name;
        std::string Config::*field;
    };

    static constexpr std::array<ShortStringFlag, 9> shortStringFlags = {{
        {'C', &Config::compiler},
        {'V', &Config::version},
        {'O', &Config::output},
        {'K', &Config::kind},
        {'M', &Config::modules},
        {'P', &Config::inputFile},
        {'E', &Config::Precmd},
        {'A', &Config::Postcmd},
        {'H', &Config::pch},
    }};

    int build(int argc, char **argv, Config &config, const std::filesystem::path &filename) {
        std::vector<std::string> selectedConfigs;

        int startIdx = 2;
        std::string_view firstArg = argv[1];
        if (firstArg.length() > 2 && firstArg[0] == '-' && firstArg[1] != '-' && firstArg.find('b') != std::string_view::npos) {
            startIdx = 1;
        }

        bool dryRunRequested = false;
        bool watchRequested = config.watch;
        for (int i = startIdx; i < argc; ++i) {
            std::string_view arg = argv[i];
            dryRunRequested = dryRunRequested || arg == "--dry-run";
            watchRequested = watchRequested || arg == "--watch";
        }
        if (dryRunRequested && watchRequested) {
            hp::printlnCl("[HelpMake] Error: --dry-run cannot be combined with --watch.", hp::RED);
            return 1;
        }

        for (int i = startIdx; i < argc; i++) {
            std::string_view arg = argv[i];
            bool handled = false;

            if (arg.starts_with("--")) {
                std::string_view body = arg.substr(2);
                std::string name;
                std::string value;
                bool hasValue = false;

                std::size_t pos = body.find('=');
                if (pos != std::string_view::npos) {
                    name = body.substr(0, pos);
                    value = std::string(body.substr(pos + 1));
                    hasValue = true;
                } else {
                    name = body;
                }

                for (const auto &f : boolFlags) {
                    if (is_typo(name, f.name)) {
                        hp::printlnCl(std::format("[HelpMake] Error: Unknown Flag '{}'. Did you mean {} ?", name, f.name), hp::RED);
                        return 1;
                    }
                    if (f.name != name)
                        continue;

                    if (!value.empty()) {
                        value = to_lower(value);
                        if (value == "true")
                            config.*f.field = true;
                        else if (value == "false")
                            config.*f.field = false;
                        else {
                            if (is_typo("false", value))
                                hp::printlnCl(std::format("[HelpMake] Error: Unknow bool value '{}'. Did you mean false ?", value), hp::RED);
                            else if (is_typo("true", value))
                                hp::printlnCl(std::format("[HelpMake] Error: Unknow bool value '{}'. Did you mean true ?", value), hp::RED);
                            else
                                hp::printlnCl(std::format("[HelpMake] Error: Unknow bool value '{}'", value), hp::RED);
                            return 1;
                        }
                    } else {
                        config.*f.field = true;
                    }
                    handled = true;
                    break;
                }

                for (const auto &f : scalarFlags) {
                    if (f.name != name)
                        continue;

                    if (!hasValue) {
                        if (i + 1 < argc) {
                            value = std::string(argv[++i]);
                        } else {
                            hp::printlnCl(std::format("[HelpMake] Error: value required after '--{}'.", name), hp::RED);
                            return 1;
                        }
                    }

                    if (name == "kind") {
                        config.kind = to_lower(value);
                    } else {
                        config.*f.field = value;
                    }

                    handled = true;
                    break;
                }

                if (handled)
                    continue;
            }

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
            } else if (arg == "-C") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    config.compiler = std::string(argv[++i]);
                } else {
                    hp::printlnCl("[HelpMake] Error: Compiler not specified after '-C'.", hp::RED);
                    return 1;
                }
            } else if (arg == "-V") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    config.version = std::string(argv[++i]);
                } else {
                    hp::printlnCl("[HelpMake] Error: Version not specified after '-V'.", hp::RED);
                    return 1;
                }
            } else if (arg.rfind("-C=", 0) == 0) {
                std::string value = std::string(arg.substr(3));
                if (value.empty()) {
                    if (i + 1 < argc && argv[i + 1][0] != '-') {
                        value = std::string(argv[++i]);
                    } else {
                        hp::printlnCl("[HelpMake] Error: Compiler not specified after '-C='.", hp::RED);
                        return 1;
                    }
                }
                config.compiler = value;
            } else if (arg.rfind("-V=", 0) == 0) {
                std::string value = std::string(arg.substr(3));
                if (value.empty()) {
                    if (i + 1 < argc && argv[i + 1][0] != '-') {
                        value = std::string(argv[++i]);
                    } else {
                        hp::printlnCl("[HelpMake] Error: Version not specified after '-V='.", hp::RED);
                        return 1;
                    }
                }
                config.version = value;
            } else if (arg == "-o") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    config.output = std::string(argv[++i]);
                } else {
                    hp::printlnCl("[HelpMake] Error: Output file not specified after '-o'.", hp::RED);
                    return 1;
                }
            } else if (arg.rfind("-o", 0) == 0 && arg.size() > 2) {
                config.output = std::string(arg.substr(2));
            } else if (arg == "-F") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    std::string flagValue = std::string(argv[++i]);
                    if (!config.flags.empty())
                        config.flags += " ";
                    config.flags += flagValue;
                    config.v_Flags.push_back(flagValue);
                } else {
                    hp::printlnCl("[HelpMake] Error: Additional flags not specified after '-F'.", hp::RED);
                    return 1;
                }
            } else if (arg.rfind("-F", 0) == 0 && arg.size() > 2) {
                std::string flagValue = std::string(arg.substr(2));
                if (!config.flags.empty())
                    config.flags += " ";
                config.flags += flagValue;
                config.v_Flags.push_back(flagValue);
            } else if (arg == "-I") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    std::string includePath = std::string(argv[++i]);
                    if (!config.flags.empty())
                        config.flags += " ";
                    config.flags += "-I" + includePath;
                    config.includeFiles.push_back(includePath);
                } else {
                    hp::printlnCl("[HelpMake] Error: Include path not specified after '-I'.", hp::RED);
                    return 1;
                }
            } else if (arg.rfind("-I", 0) == 0 && arg.size() > 2) {
                std::string includePath = std::string(arg.substr(2));
                if (!config.flags.empty())
                    config.flags += " ";
                config.flags += "-I" + includePath;
                config.includeFiles.push_back(includePath);
            } else if (arg == "-L") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    std::string libPath = std::string(argv[++i]);
                    if (!config.flags.empty())
                        config.flags += " ";
                    config.flags += "-L" + libPath;
                } else {
                    hp::printlnCl("[HelpMake] Error: Library path not specified after '-L'.", hp::RED);
                    return 1;
                }
            } else if (arg.rfind("-L", 0) == 0 && arg.size() > 2) {
                std::string libPath = std::string(arg.substr(2));
                if (!config.flags.empty())
                    config.flags += " ";
                config.flags += "-L" + libPath;
            } else if (arg == "-f") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    const_cast<std::filesystem::path &>(filename) = std::string(argv[++i]);
                } else {
                    hp::printlnCl("[HelpMake] Error: filename not specified after '-f'.", hp::RED);
                    return 1;
                }
            } else if (arg.rfind("-f=", 0) == 0 && arg.size() > 2) {
                const_cast<std::filesystem::path &>(filename) = std::string(arg.substr(3));
            } else if (arg == "-j") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    try {
                        config.jobs = std::stoi(std::string(argv[++i]));
                    } catch (...) {
                        hp::printlnCl("[HelpMake] Error: Invalid value for '-j'.", hp::RED);
                        return 1;
                    }
                } else {
                    config.jobs = static_cast<int>(std::thread::hardware_concurrency());
                    if (config.jobs == 0)
                        config.jobs = 1;
                }
            } else if (arg.rfind("-j", 0) == 0 && arg.size() > 2) {
                try {
                    config.jobs = std::stoi(std::string(arg.substr(2)));
                } catch (...) {
                    hp::printlnCl(std::format("[HelpMake] Error: Invalid value for '-j{}'.", arg.substr(2)), hp::RED);
                    return 1;
                }
            } else if (arg == "--config") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    selectedConfigs.push_back(std::string(argv[++i]));
                } else {
                    hp::printlnCl("[HelpMake] Error: Config name not specified after '--config'.", hp::RED);
                    return 1;
                }
            } else if (arg.rfind("--config=", 0) == 0 && arg.size() > 9) {
                selectedConfigs.push_back(std::string(arg.substr(9)));
            } else if (arg == "--var") {
                if (i + 1 >= argc) {
                    hp::printlnCl("[HelpMake] Error: --var requires NAME=VALUE", hp::RED);
                    return 1;
                }
                std::string rest = std::string(argv[++i]);
                std::size_t pos = rest.find('=');
                if (pos == std::string::npos || pos == 0) {
                    hp::printlnCl(std::format("[HelpMake] Error: --var expects NAME=VALUE, got '{}'", rest), hp::RED);
                    return 1;
                }
                config.v_Vars[trim(rest.substr(0, pos))] = trim(rest.substr(pos + 1));
            } else if (arg.rfind("--var=", 0) == 0) {
                std::string rest = std::string(arg.substr(6));
                std::size_t pos = rest.find('=');
                if (pos == std::string::npos || pos == 0) {
                    hp::printlnCl(std::format("[HelpMake] Error: --var expects NAME=VALUE, got '{}'", rest), hp::RED);
                    return 1;
                }
                config.v_Vars[trim(rest.substr(0, pos))] = trim(rest.substr(pos + 1));
            } else if (arg == "-ZigCpp" || arg == "-zigcpp" || arg == "-zig-c++" || arg == "-Zigc++") {
                config.compiler = "zig c++";
            } else if (arg == "-ZigCC" || arg == "-zigcc" || arg == "-zig-cc" || arg == "-Zigcc") {
                config.compiler = "zig-cc";
            } else if (arg == "-Go" || arg == "-go" || arg == "-golang") {
                config.compiler = "go";
            } else if (arg == "-v") {
                config.verbose = true;
            } else if (arg == "-r") {
                config.run = true;
            } else if (arg == "--watch") {
                Parser parse(filename, config);
                parse.parse();
                parse.execute();
                parse.watch();
            } else if (arg.length() > 2 && arg[0] == '-' && arg[1] != '-') {
                bool isGroup = true;
                for (std::size_t j = 1; j < arg.length(); ++j) {
                    char c = arg[j];

                    if (c == 'b') {
                        continue;
                    }

                    bool isBool = false;
                    for (const auto &f : shortBoolFlags) {
                        if (f.name == c) {
                            config.*f.field = true;
                            isBool = true;
                            break;
                        }
                    }
                    if (isBool) {
                        continue;
                    }

                    bool isString = false;
                    for (const auto &f : shortStringFlags) {
                        if (f.name == c) {
                            std::string value = std::string(arg.substr(j + 1));
                            if (!value.empty() && value[0] == '=')
                                value.erase(0, 1);

                            if (value.empty()) {
                                hp::printlnCl(std::format("[HelpMake] Error: value required after '-{}' in group '{}'", c, arg), hp::RED);
                                return 1;
                            }

                            if (c == 'K') {
                                config.kind = to_lower(value);
                            } else {
                                config.*f.field = value;
                            }
                            isString = true;
                            break;
                        }
                    }
                    if (isString) {
                        break;
                    }

                    if (c == 'J') {
                        std::string value = std::string(arg.substr(j + 1));
                        if (!value.empty() && value[0] == '=')
                            value.erase(0, 1);

                        if (value.empty()) {
                            hp::printlnCl(std::format("[HelpMake] Error: value required after '-J' in group '{}'", arg), hp::RED);
                            return 1;
                        }

                        try {
                            config.jobs = std::stoi(value);
                        } catch (...) {
                            hp::printlnCl(std::format("[HelpMake] Error: Invalid value for '-J{}'", value), hp::RED);
                            return 1;
                        }
                        break;
                    } else if (c == 'I') {
                        std::string value = std::string(arg.substr(j + 1));
                        if (!value.empty() && value[0] == '=')
                            value.erase(0, 1);
                        if (value.empty()) {
                            hp::printlnCl(std::format("[HelpMake] Error: value required after '-I' in group '{}'", arg), hp::RED);
                            return 1;
                        }
                        if (!config.flags.empty())
                            config.flags += " ";
                        config.flags += "-I" + value;
                        config.includeFiles.push_back(value);
                        break;
                    } else if (c == 'L') {
                        std::string value = std::string(arg.substr(j + 1));
                        if (!value.empty() && value[0] == '=')
                            value.erase(0, 1);
                        if (value.empty()) {
                            hp::printlnCl(std::format("[HelpMake] Error: value required after '-L' in group '{}'", arg), hp::RED);
                            return 1;
                        }
                        if (!config.flags.empty())
                            config.flags += " ";
                        config.flags += "-L" + value;
                        break;
                    } else if (c == 'F') {
                        std::string value = std::string(arg.substr(j + 1));
                        if (!value.empty() && value[0] == '=')
                            value.erase(0, 1);
                        if (value.empty()) {
                            hp::printlnCl(std::format("[HelpMake] Error: value required after '-F' in group '{}'", arg), hp::RED);
                            return 1;
                        }
                        if (!config.flags.empty())
                            config.flags += " ";
                        config.flags += value;
                        config.v_Flags.push_back(value);
                        break;
                    }

                    isGroup = false;
                    break;
                }
                if (isGroup)
                    continue;
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
                config.v_inputFiles.push_back(std::string(arg));
            } else {
                hp::printlnCl(std::format("[HelpMake] Error: Invalid Argument '{}'.\n Use --help for the available arguments", arg), hp::RED);
                return 1;
            }
        }

        if (config.create) {
            Parser parser(filename, config);
            return parser.parse();
        }

        if (config.n_file || !std::filesystem::exists(filename)) {
            if (!config.n_file) {
                if (filename != "HelpMake.txt" && std::filesystem::exists("HelpMake.txt")) {
                    hp::printlnCl("Build file not found. Using default HelpMake.txt", hp::YELLOW);
                    const_cast<std::filesystem::path &>(filename) = "HelpMake.txt";
                }

                hp::printlnCl("Build file not found: " + filename.string(), hp::YELLOW);
                if (config.verbose)
                    hp::printlnCl("Building from command-line arguments only\n", hp::CYAN);

                Parser Rawparser(filename, config);
                if (config.v_inputFiles.empty()) {
                    if (Rawparser.collectFiles(config, ".") != 0) {
                        hp::printlnCl("[HelpMake] Error: No source files found in the current directory.", hp::RED);
                        return 1;
                    }
                }

                if (Rawparser.autoInputFiles(config, "[HelpMake]") != 0)
                    return 1;
                if (config.compiler.empty() && Rawparser.autoCompiler(config, "[HelpMake]") != 0)
                    return 1;
                if (config.version.empty() && Rawparser.autoVersion(config, "[HelpMake]") != 0)
                    return 1;
                if (config.includeFiles.empty() && Rawparser.autoIncludes(config, "[HelpMake]") != 0)
                    return 1;
                if (config.kind.empty() && Rawparser.autoKind(config, "[HelpMake]") != 0)
                    return 1;
                if (config.output.empty() && Rawparser.autoOutput(config, "[HelpMake]") != 0)
                    return 1;
                if (config.v_Flags.empty() && Rawparser.autoLinkFlags(config, "[HelpMake]") != 0)
                    return 1;
            } else {
                if (config.compiler.empty()) {
                    hp::printlnCl("[HelpMake] Error: --nofile requires a compiler. Use -C=<name> or --compiler=<name>.", hp::RED);
                    return 1;
                }
                if (config.v_inputFiles.empty()) {
                    hp::printlnCl("[HelpMake] Error: --nofile requires at least one input file on the command line.", hp::RED);
                    return 1;
                }
                if (config.output.empty()) {
#ifdef _WIN32
                    config.output = "a.exe";
#else
                    config.output = "a.out";
#endif
                }

                if (config.kind.empty())
                    config.kind = "executable";
            }

            if (!config.v_Errors.empty()) {
                for (const auto &e : config.v_Errors)
                    hp::printCl(e, hp::RED);
                return 1;
            }

            Parser full(filename, config);
            return full.execute();
        }

        {
            Parser parser(filename, config);
            int rc = 0;

            if ((rc = parser.parse()) == 0) {
                if (!selectedConfigs.empty())
                    rc = parser.executeConfigs(selectedConfigs);
                else
                    rc = parser.execute();
            }

            if (config.time) {
                const auto &times = parser.getConfig().v_Times;

                auto get = [&](const std::string &name) -> double {
                    auto it = times.find(name);
                    return it == times.end() ? 0.0 : it->second;
                };

                double parseRaw = get("Parse");
                double fetch = get("Fetch");
                double dep = get("Dependency");
                double exec = get("Execute");

                double parsePure = std::max(0.0, parseRaw - fetch);
                double total = parseRaw + dep + exec;

                if (total >= 0.001) {
                    hp::printlnCl("\nTiming:", hp::CYAN);

                    auto line = [](const std::string &label, double s, hp::Color c = hp::YELLOW) {
                        hp::printlnCl(std::format("  {:<13}{:>8.3f}s", label, s), c);
                    };

                    line("Parse:", parsePure);
                    if (fetch > 0.001)
                        line("Fetch:", fetch);
                    if (dep > 0.001)
                        line("Dependency:", dep);
                    line("Execute:", exec);
                    line("Total:", total);
                }
            }

            if (rc != 0)
                return rc;
        }
        return 0;
    }

} // namespace hm::cli