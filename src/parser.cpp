#include "parser.hpp"
#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"
#include "hp/serializer/serialize.hpp"
#include "hp/system/command.hpp"
#include "hp/time/time.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string_view>
#include <vector>

static std::string_view trim_view(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.remove_suffix(1);
    return s;
}

static bool similar(std::string_view first, std::string_view second) {
    if (first.size() != second.size())
        return false;
    if (first == second)
        return true;

    for (std::size_t i = 0; i < first.size(); i++) {
        if (static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(first[i]))) !=
            static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(second[i]))))
            return false;
    }
    return true;
}

static std::string to_lower(std::string_view str) {
    std::string result(str.data(), str.size());
    std::ranges::transform(result, result.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

static bool is_typo(std::string_view str1, std::string_view str2) {
    if (str1 == str2)
        return false;

    std::string a = to_lower(str1);
    std::string b = to_lower(str2);

    if (a == b)
        return true;

    if (a.size() == b.size()) {
        int errors = 0;
        for (std::size_t i = 0; i < a.size(); ++i)
            if (a[i] != b[i] && ++errors >= 2)
                return false;
        return errors == 1;
    }

    if (a.size() > b.size())
        std::swap(a, b);
    if (b.size() != a.size() + 1)
        return false;

    std::size_t i = 0, j = 0;
    bool skipped = false;
    while (i < a.size() && j < b.size()) {
        if (a[i] != b[j]) {
            if (skipped)
                return false;
            skipped = true;
            ++j;
        } else {
            ++i;
            ++j;
        }
    }
    return true;
}

static bool isDeclaredModule(const std::string &file, const std::vector<std::string> &modules) {
    for (const auto &m : modules) {
        if (std::filesystem::path(m).lexically_normal() == std::filesystem::path(file).lexically_normal())
            return true;
    }
    return false;
}

static std::string trim(const std::string &s) {
    std::size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    std::size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string extractValue(const std::string &line, const std::string &key) {
    std::size_t pos = line.find(key);
    if (pos == std::string::npos)
        return "";
    return trim(line.substr(pos + key.size()));
}

static std::string extractModule(const std::filesystem::path &filename) {
    std::ifstream file(filename);
    if (!file) {
        hp::printlnCl(std::format("[HelpMake] Error: Could not open file '{}'", filename.string()), hp::Color::RED);
        exit(EXIT_FAILURE);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("export module") == std::string::npos)
            continue;

        std::string value = extractValue(line, "export module");

        while (!value.empty() && (value.back() == ';' || std::isspace(static_cast<unsigned char>(value.back()))))
            value.pop_back();

        return trim(value);
    }

    hp::printlnCl(std::format("[HelpMake] Error: File '{}' is not a module", filename.string()), hp::Color::RED);
    exit(EXIT_FAILURE);
}

static bool isModule(const std::string_view &s) {
    return s.ends_with(".cppm") || s.ends_with(".ixx") || s.ends_with(".mpp") ||
           s.ends_with(".cxxm") || s.ends_with(".c++m") || s.ends_with(".ccm");
}

static bool gitAvailable() {
#ifdef _WIN32
    return hp::command("where git").find("INFO:") == std::string::npos;
#else
    return std::system("which git >/dev/null 2>&1") == 0;
#endif
}

static std::string compilerCommand(const std::string &compiler) {
    if (compiler == "msvc")
        return "cl";
    return compiler;
}

static constexpr std::array<std::string_view, 2> validKinds = {
    "executable",
    "static",
};

static bool isValidKind(std::string_view key) {
    std::string k = to_lower(key);

    for (auto v : validKinds) {
        if (v == k)
            return true;
        if (is_typo(k, v)) {
            hp::printlnCl(std::format("[HelpMake] Error: Invalid Kind '{}'. Did you mean {} ?", k, v), hp::RED);
            exit(EXIT_FAILURE);
        }
    }
    return false;
}

void Parser::parse() {

    if (cfg.kind.empty())
        cfg.kind = "executable";

    if (!isValidKind(cfg.kind)) {
        cfg.v_Errors.push_back(std::format("[HelpMake] Error: Invalid Kind '{}'", cfg.kind));
    }

    if (cfg.create) {
        hp::File file;

        cfg.v_inputFiles = splitArgs(cfg.inputFile);
        cfg.v_Flags = splitArgs(cfg.flags);
        cfg.includeFiles = splitArgs(cfg.includes);
        cfg.v_Modules = splitArgs(cfg.modules);

        if (file.exists(filename.string()) && !cfg.quiet) {
            hp::printlnCl(std::format("Warning: {} already exists. Overwrite? (y/n)", filename.string()), hp::Color::YELLOW);
            char verif = hp::get<char>("", "Invalid choice");
            if (verif != 'y') {
                hp::printlnCl("File was not overwritten.", hp::Color::GREEN);
                return;
            }
        }

        hp::save(filename.string(), {{"Compiler", cfg.compiler}, {"Version", cfg.version}, {"Output", cfg.output}});
        std::ofstream make(filename.string(), std::ios::app);

        make << "\nInputFiles {\n";
        for (const auto &input : cfg.v_inputFiles)
            make << "    " << input << "\n";
        make << "}\n";

        if (!cfg.v_Flags.empty()) {
            make << "\nFlags {\n";
            for (const auto &f : cfg.v_Flags)
                make << "    " << f << "\n";
            make << "}\n";
        }
        if (!cfg.v_Github.empty()) {
            make << "\nGithub {\n";
            for (const auto &g : cfg.v_Github)
                make << "    " << g << "\n";
            make << "}\n";
        }
        if (!cfg.v_Modules.empty()) {
            make << "\nModules {\n";
            for (const auto &m : cfg.v_Modules)
                make << "    " << m << "\n";
            make << "}\n";
        }
        if (!cfg.v_Precmd.empty()) {
            make << "\nPreBuild {\n";
            for (const auto &cmd : cfg.v_Precmd)
                make << "    " << cmd << "\n";
            make << "}\n";
        }
        if (!cfg.v_Postcmd.empty()) {
            make << "\nPostBuild {\n";
            for (const auto &cmd : cfg.v_Postcmd)
                make << "    " << cmd << "\n";
            make << "}\n";
        }

        make.close();
        hp::printlnCl(std::format("{} was created successfully.", filename.string()), hp::Color::GREEN);
        return;
    }

    if (cfg.buildSere || cfg.compiler == "sere") {
        execute();
        return;
    }

    if (!cfg.n_file && cfg.verbose)
        std::cout << std::format("[HelpMake] Trying to open file: {}\n", filename.string());

    if (cfg.n_file) {
        buildCommand();
        return;
    }

    std::fstream file(filename);
    if (!file.is_open()) {
        hp::printlnCl(std::format("Error: Could not open file: {}\n", filename.string()), hp::Color::RED);
        hp::printlnCl("Make sure the file exists in the current directory.", hp::Color::YELLOW);
        hp::printlnCl("Or provide all required arguments on the command line", hp::Color::YELLOW);
        exit(EXIT_FAILURE);
    }

    std::string line;
    std::string currentBlock;

    std::vector<std::string_view> args = {
        "Compiler:",
        "Version:",
        "Output:",
        "InputFiles:",
        "Includes:",
        "Flags:",
        "Modules:",
        "Run:",
        "Arguments:",
        "Kind:",
        "InputFiles {",
        "Includes {",
        "Flags {",
        "Fetch {",
        "Modules {",
        "PreBuild {",
        "PostBuild {",
        "Arguments {",
        "Config:",
    };

    std::size_t lines = 0;
    while (std::getline(file, line)) {
        ++lines;
        std::size_t hash = line.find('#');
        if (hash != std::string::npos)
            line = line.substr(0, hash);

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        for (const auto &v : args) {
            parseTypos(lines, filename, line, v);
        }

        if (isConfigSet) {
            for (char c : line) {
                if (c == '{')
                    configBraceDepth++;
                else if (c == '}')
                    configBraceDepth--;
            }

            if (configBraceDepth <= 0) {
                std::string justAdded = currentConfig.name;
                configs[justAdded] = currentConfig;
                if (cfg.debug)
                    std::cout << std::format("Saved Config: '{}'\n", justAdded);
                isConfigSet = false;
                currentConfig = Config{};
                currentBlock.clear();
                if (cfg.allCfgs)
                    executeConfigs({justAdded});
                continue;
            }

            if (!currentBlock.empty()) {
                if (line.find("}") != std::string::npos) {
                    currentBlock.clear();
                    continue;
                }
                std::string value = trim(line);
                if (value.empty())
                    continue;

                if (currentBlock == "InputFiles") {
                    currentConfig.v_inputFiles.push_back(value);
                    if (!currentConfig.inputFile.empty())
                        currentConfig.inputFile += " ";
                    currentConfig.inputFile += value;

                } else if (currentBlock == "Includes") {
                    currentConfig.includeFiles.push_back(value);

                } else if (currentBlock == "Flags") {
                    currentConfig.v_Flags.push_back(value);
                    if (!currentConfig.flags.empty())
                        currentConfig.flags += " ";
                    currentConfig.flags += value;

                } else if (currentBlock == "Fetch") {
                    std::string path = processGithubEntry(value);
                    if (!path.empty()) {
                        currentConfig.flags += std::format(" -I{}", path);
                        currentConfig.v_Github.push_back(std::format("{} -> {}", value.substr(0, value.find("->")), path));
                    }

                } else if (currentBlock == "Modules") {
                    currentConfig.v_Modules.push_back(value);
                    if (!currentConfig.modules.empty())
                        currentConfig.modules += " ";
                    currentConfig.modules += value;

                } else if (currentBlock == "PreBuild") {
                    currentConfig.preBuild.push_back(value);

                } else if (currentBlock == "PostBuild") {
                    currentConfig.postBuild.push_back(value);
                } else if (currentBlock == "Argument") {
                    parseArgument(currentConfig, value);
                }
                continue;
            }

            if (line.find("InputFiles {") != std::string::npos) {
                currentBlock = "InputFiles";
                continue;
            }
            if (line.find("Includes {") != std::string::npos) {
                currentBlock = "Includes";
                continue;
            }
            if (line.find("Flags {") != std::string::npos) {
                currentBlock = "Flags";
                continue;
            }
            if (line.find("Fetch {") != std::string::npos) {
                currentBlock = "Fetch";
                continue;
            }
            if (line.find("Modules {") != std::string::npos) {
                currentBlock = "Modules";
                continue;
            }
            if (line.find("PreBuild {") != std::string::npos) {
                currentBlock = "PreBuild";
                continue;
            }
            if (line.find("PostBuild {") != std::string::npos) {
                currentBlock = "PostBuild";
                continue;
            }
            if (line.find("Arguments {") != std::string::npos) {
                currentBlock = "Argument";
                continue;
            }

            std::string value;
            if (line.find("Arguments:") != std::string::npos) {
                std::string value = extractValue(line, "Arguments:");
                parseArgument(currentConfig, value);
            } else if (line.find("Compiler:") != std::string::npos) {
                value = extractValue(line, "Compiler:");
                currentConfig.compiler = value;
                if (cfg.debug)
                    std::cout << std::format("Compiler found in config '{}': '{}'\n", currentConfig.name, value);

            } else if (line.find("Version:") != std::string::npos) {
                value = extractValue(line, "Version:");
                currentConfig.version = value;
                if (cfg.debug)
                    std::cout << std::format("Version found in config '{}': '{}'\n", currentConfig.name, value);

            } else if (line.find("Output:") != std::string::npos) {
                value = extractValue(line, "Output:");
                currentConfig.output = value;
                if (cfg.debug)
                    std::cout << std::format("Output found in config '{}': '{}'\n", currentConfig.name, value);

            } else if (line.find("Flags:") != std::string::npos) {
                value = extractValue(line, "Flags:");
                if (currentConfig.flags.empty())
                    currentConfig.flags = value;
                else
                    currentConfig.flags += " " + value;
                currentConfig.v_Flags.push_back(value);
                if (cfg.debug)
                    std::cout << std::format("Flag found in config '{}': '{}'\n", currentConfig.name, value);

            } else if (line.find("InputFiles:") != std::string::npos) {
                value = extractValue(line, "InputFiles:");
                if (currentConfig.inputFile.empty())
                    currentConfig.inputFile = value;
                else
                    currentConfig.inputFile += " " + value;
                currentConfig.v_inputFiles.push_back(value);
                if (cfg.debug)
                    std::cout << std::format("Input file found in config '{}': '{}'\n", currentConfig.name, value);

            } else if (line.find("Includes:") != std::string::npos) {
                value = extractValue(line, "Includes:");
                if (currentConfig.includes.empty())
                    currentConfig.includes = value;
                else
                    currentConfig.includes += " " + value;
                currentConfig.includeFiles.push_back(value);
                if (cfg.debug)
                    std::cout << std::format("Include file found in config '{}': '{}'\n", currentConfig.name, value);

            } else if (line.find("Run:") != std::string::npos) {
                currentConfig.run = true;
                if (cfg.debug)
                    std::cout << std::format("Run command found in config '{}'\n", currentConfig.name);

            } else if (line.find("Kind:") != std::string::npos) {
                value = extractValue(line, "Kind:");
                currentConfig.kind = to_lower(value);
                if (isValidKind(currentConfig.kind))
                    hp::printlnCl(std::format("[HelpMake] Error: {}:{}: Invalid Kind '{}' ", filename.string(), lines, currentConfig.kind), hp::RED);
                if (cfg.debug)
                    std::cout << std::format("Kind found in config '{}': '{}'\n", currentConfig.name, value);
            }
            continue;
        }

        if (line.find("Arguments:") != std::string::npos) {
            std::string value = extractValue(line, "Arguments:");
            parseArgument(cfg, value);
        } else if (line.find("Arguments {") != std::string::npos) {
            isArgSet = true;
        } else if (line.find("}") != std::string::npos && isArgSet) {
            isArgSet = false;
        } else if (isArgSet) {
            std::string value = trim(line);
            if (!value.empty()) {
                if (cfg.debug)
                    hp::printlnCl(std::format("Found Argument: {}", value), hp::Color::YELLOW);
                parseArgument(cfg, value);
            }
        } else if (line.find("Compiler:") != std::string::npos) {
            std::string value = extractValue(line, "Compiler:");
            if (cfg.compiler.empty())
                cfg.compiler = value;

            if (cfg.compiler.empty()) {
                hp::printlnCl("Error: Compiler not specified in the file.", hp::Color::RED);
                exit(EXIT_FAILURE);
            }

            if (cfg.debug)
                std::cout << std::format("Found Compiler: '{}'\n", cfg.compiler);
        } else if (line.find("Version:") != std::string::npos) {
            cfg.version = extractValue(line, "Version:");
            if (cfg.version.empty()) {
                hp::printlnCl("Error: Version not specified in the file.", hp::Color::RED);
                exit(EXIT_FAILURE);
            }
            if (cfg.debug)
                std::cout << std::format("Found Version: '{}'\n", cfg.version);
        } else if (line.find("Output:") != std::string::npos) {
            std::string value = extractValue(line, "Output:");
            if (cfg.output.empty())
                cfg.output = value;
            if (cfg.output.empty()) {
#ifdef _WIN32
                cfg.output = "a.exe";
#else
                cfg.output = "a.out";
#endif
                hp::printlnCl(std::format("Warning: No output file specified. Using default: {}", cfg.output), hp::Color::YELLOW);
            }
            if (cfg.debug)
                std::cout << std::format("Found Output: '{}'\n", cfg.output);
        } else if (line.find("InputFiles:") != std::string::npos) {
            std::string value = extractValue(line, "InputFiles:");
            cfg.inputFile += (cfg.inputFile.empty() ? "" : " ") + value;
        } else if (line.find("InputFiles {") != std::string::npos) {
            isInputFileSet = true;
        } else if (line.find("}") != std::string::npos && isInputFileSet) {
            isInputFileSet = false;
        } else if (isInputFileSet) {
            std::string value = trim(line);
            if (!value.empty()) {
                cfg.inputFile += (cfg.inputFile.empty() ? "" : " ") + value;
                cfg.v_inputFiles.push_back(value);
                if (cfg.debug)
                    std::cout << std::format("Found Input Files: '{}'\n", cfg.inputFile);
            }

        } else if (line.find("Includes:") != std::string::npos) {
            std::string value = extractValue(line, "Includes:");
            cfg.flags += (cfg.flags.empty() ? "" : " ") + value;
        } else if (line.find("Includes {") != std::string::npos) {
            isIncludeSet = true;
        } else if (line.find("}") != std::string::npos && isIncludeSet) {
            isIncludeSet = false;
            if (cfg.debug) {
                std::cout << "Found Include Files:\n";
                for (const auto &vec : cfg.includeFiles)
                    std::cout << std::format("- {}\n", vec == "." ? "[root]" : vec);
            }

        } else if (isIncludeSet) {
            std::string value = trim(line);
            if (!value.empty()) {
                cfg.flags += std::format(" -I{}", value);
                cfg.includeFiles.push_back(value);
            }
        } else if (line.find("Flags {") != std::string::npos) {
            isFlagsSet = true;
        } else if (line.find("}") != std::string::npos && isFlagsSet) {
            isFlagsSet = false;
            if (cfg.debug) {
                std::cout << "Found Flags:\n";
                for (const auto &vec : cfg.v_Flags)
                    std::cout << std::format("- {}\n", vec);
            }
        } else if (isFlagsSet) {
            std::string value = trim(line);
            if (!value.empty()) {
                cfg.flags += (cfg.flags.empty() ? "" : " ") + value;
                cfg.v_Flags.push_back(value);
            }
        } else if (line.find("Flags:") != std::string::npos) {
            cfg.flags += " " + extractValue(line, "Flags:");
            if (cfg.debug)
                std::cout << std::format("Found Flags: '{}'\n", cfg.flags);

        } else if (line.find("Fetch {") != std::string::npos) {
            isFetchSet = true;
        } else if (line.find("}") != std::string::npos && isFetchSet) {
            isFetchSet = false;
        } else if (isFetchSet) {
            std::string value = trim(line);
            if (value.empty())
                continue;

            std::string path = processGithubEntry(value);
            if (!path.empty()) {
                cfg.flags += std::format(" -I{}", path);
                cfg.v_Github.push_back(std::format("{} -> {}", value.substr(0, value.find("->")), path));
            }

        } else if (line.find("Modules:") != std::string::npos) {
            std::string value = extractValue(line, "Modules:");
            if (cfg.modules.empty())
                cfg.modules = value;
            else
                cfg.modules += " " + value;
            if (cfg.debug)
                std::cout << std::format("Found Modules: {}\n", value);
        } else if (line.find("Modules {") != std::string::npos) {
            isModulesSet = true;
        } else if (line.find("}") != std::string::npos && isModulesSet) {
            isModulesSet = false;
            if (cfg.debug) {
                std::cout << "Found Modules:\n";
                for (const auto &m : cfg.v_Modules)
                    std::cout << std::format("- {}\n", m);
            }

        } else if (isModulesSet) {
            std::string value = trim(line);
            if (!value.empty()) {
                if (cfg.modules.empty())
                    cfg.modules = value;
                else
                    cfg.modules += " " + value;
                cfg.v_Modules.push_back(value);
            }
        } else if (line.find("PreBuild {") != std::string::npos) {
            isPreBuildSet = true;
        } else if (line.find("}") != std::string::npos && isPreBuildSet) {
            isPreBuildSet = false;
        } else if (isPreBuildSet) {
            std::string value = trim(line);
            if (!value.empty()) {
                if (cfg.Precmd.empty())
                    cfg.Precmd = value;
                else
                    cfg.Precmd += " && " + value;
                if (cfg.debug)
                    std::cout << std::format("Pre-build command: {}\n", value);
                cfg.v_Precmd.push_back(value);
            }
        } else if (line.find("PostBuild {") != std::string::npos) {
            isPostBuildSet = true;
        } else if (line.find("}") != std::string::npos && isPostBuildSet) {
            isPostBuildSet = false;
        } else if (isPostBuildSet) {
            std::string value = trim(line);
            if (!value.empty()) {
                if (cfg.Postcmd.empty())
                    cfg.Postcmd = value;
                else
                    cfg.Postcmd += " && " + value;
                if (cfg.debug)
                    std::cout << std::format("Post-build command: {}\n", value);
                cfg.v_Postcmd.push_back(value);
            }
        } else if (line.find("Kind:") != std::string::npos) {
            std::string value = extractValue(line, "Kind:");
            cfg.kind = to_lower(value);
            if (!isValidKind(cfg.kind))
                hp::printlnCl(std::format("[HelpMake] Error: {}:{}: Invalid Kind '{}' ", filename.string(), lines, cfg.kind), hp::RED);
            if (cfg.debug)
                std::cout << std::format("Found Kind: '{}'\n", value);
        } else if (line.find("Config:") != std::string::npos) {
            std::string rest = line.substr(line.find("Config:") + 7);
            std::size_t brace = rest.find('{');
            if (brace == std::string::npos)
                continue;

            std::string name = trim(rest.substr(0, brace));
            if (name.empty()) {
                hp::printlnCl("Error: Config name is empty.", hp::Color::RED);
                exit(EXIT_FAILURE);
            }

            isConfigSet = true;
            currentConfig = Config{};
            currentConfig.name = name;
            configBraceDepth = 1;
            if (cfg.debug)
                std::cout << std::format("\nFound Config: '{}'\n", name);
        }
    }

    if (!cfg.v_Errors.empty()) {
        for (const auto &e : cfg.v_Errors) {
            hp::printCl(e, hp::RED);
        }
        hp::printlnCl(std::format("\n[HelpMake] {} Errors generated.\n[HelpMake] Build Failed.", cfg.v_Errors.size()), hp::RED);
        exit(EXIT_FAILURE);
    }
    file.close();

    if (cfg.output.empty()) {
#ifdef _WIN32
        cfg.output = "a.exe";
#else
        cfg.output = "a.out";
#endif
        hp::printlnCl(std::format("Output not specified in the file. Using default: {}", cfg.output), hp::Color::YELLOW);
    }
    if (cfg.inputFile.empty()) {
        hp::printlnCl("Error: InputFile not found in the file.", hp::Color::RED);
        exit(EXIT_FAILURE);
    }
}

void Parser::execute() {
    executeConfig(cfg, "");
}

void Parser::buildCommand() {
    if (cfg.compiler.empty()) {
        hp::printlnCl("Error: No compiler specified.", hp::Color::RED);
        exit(EXIT_FAILURE);
    }
    if (cfg.inputFile.empty()) {
        hp::printlnCl("Error: No input files specified.", hp::Color::RED);
        exit(EXIT_FAILURE);
    }
    if (cfg.output.empty()) {
        if (!cfg.quiet)
            hp::printlnCl("Warning: No output specified. Using a.exe", hp::Color::YELLOW);
        cfg.output = "a.exe";
    }

    cfg.v_inputFiles = splitArgs(cfg.inputFile);
    cfg.v_Flags = splitArgs(cfg.flags);
    cfg.includeFiles = splitArgs(cfg.includes);
    cfg.v_Modules = splitArgs(cfg.modules);

    if (cfg.verbose) {
        std::cout << std::format("Compiler:    {}{}\n", hp::getColorCode(hp::YELLOW), cfg.compiler)
                  << hp::getColorCode(hp::RESET);
        std::cout << std::format("Version:     {}{}\n", hp::getColorCode(hp::YELLOW), cfg.version)
                  << hp::getColorCode(hp::RESET);
        std::cout << std::format("Input Files: {}{}\n", hp::getColorCode(hp::YELLOW), cfg.inputFile)
                  << hp::getColorCode(hp::RESET);
        std::cout << std::format("Output:      {}{}\n", hp::getColorCode(hp::YELLOW), cfg.output)
                  << hp::getColorCode(hp::RESET);
        std::cout << std::format("Flags:       {}{}\n", hp::getColorCode(hp::YELLOW), cfg.flags)
                  << hp::getColorCode(hp::RESET);
    }
    execute();
    hp::exit(0);
}

void Parser::executeConfig(const Config &c, const std::string &name) {
    std::string comp = compilerCommand(c.compiler);
    if (comp.empty()) {
        hp::printlnCl("Error: Unspecified Compiler.", hp::Color::RED);
        exit(EXIT_FAILURE);
    }

    bool isClang = (comp == "clang++" || comp == "clang");
    bool showOutput = c.verbose || c.debug;
    bool isConfig = !name.empty();

    std::string tag = isConfig ? std::format("[HelpMake] Config '{}':", name) : "[HelpMake]";

    if (c.json) {
        Parser::generateCompileCommands();
    }

    if (!c.Precmd.empty()) {
        if (showOutput)
            hp::printlnCl(std::format("{} Running pre-build command: {}", tag, c.Precmd), hp::Color::CYAN);
        int error = std::system(c.Precmd.c_str());
        if (error == -1) {
            hp::printlnCl("Error: Could not launch pre-build command.", hp::Color::RED);
            exit(EXIT_FAILURE);
        }
        if (error != 0) {
            hp::printlnCl(std::format("Error: Pre-build command failed (exit code {}).", error), hp::Color::RED);
            exit(EXIT_FAILURE);
        }
        if (showOutput)
            hp::printlnCl("Pre-build command executed successfully.", hp::Color::GREEN);
    }

    std::string version = c.version;
    if (!version.empty() && version.front() != '-')
        version.insert(version.begin(), '-');

    std::filesystem::create_directories(r_logDir);
    if (isConfig)
        std::filesystem::create_directories(logDir);

    std::string CommandFlags;
    if (!version.empty())
        CommandFlags = version;
    if (!c.flags.empty())
        CommandFlags += (CommandFlags.empty() ? "" : " ") + c.flags;

    std::string compileFlags, linkFlags;

    {
        std::istringstream iss(CommandFlags);
        std::string tok;
        while (iss >> tok) {
            if (tok.rfind("-l", 0) == 0) {
                if (!linkFlags.empty())
                    linkFlags += " ";
                linkFlags += tok;
            } else {
                if (!compileFlags.empty())
                    compileFlags += " ";
                compileFlags += tok;
            }
        }
    }

    if (isConfig) {
        hp::printlnCl(std::format("\nBuilding config: {}", name), hp::Color::CYAN);
        if (showOutput) {
            std::string displayCommand = std::format("{} {} {} -o {}", comp, CommandFlags, c.inputFile, c.output);
            std::cout << std::format("Config '{}': Command: {}\n", name, displayCommand);
        }
    }

    int exitCode = 0;
    auto timer = hp::startTimer();

    if (c.seperate) {

        std::filesystem::create_directories("build/HelpMake/obj");
        if (isClang)
            std::filesystem::create_directories("build/HelpMake/modules");

        std::map<std::string, std::string> clangModules;
        if (isClang) {
            for (const auto &m : c.v_Modules) {
                if (!std::filesystem::exists(m))
                    continue;
                std::string moduleName = extractModule(m);
                std::string moduleBase = std::filesystem::path(m).stem().string();
                clangModules[moduleName] = "build/HelpMake/modules/" + moduleBase + ".pcm";
            }
            if (showOutput && !clangModules.empty()) {
                std::cout << std::format("{} Clang modules detected:\n", tag);
                for (const auto &[modName, modPath] : clangModules)
                    std::cout << std::format("  {} -> {}\n", modName, modPath);
            }
        }

        std::vector<std::string> expandedFiles;
        for (const auto &entry : c.v_inputFiles) {
            if (entry.find('*') != std::string::npos) {
                std::filesystem::path p(entry);
                std::string dir = p.parent_path().string();
                if (dir.empty())
                    dir = ".";

                std::string extension = p.extension().string();
                if (extension.empty()) {
                    expandedFiles.push_back(entry);
                    continue;
                }

                for (const auto &file : std::filesystem::directory_iterator(dir)) {
                    if (file.is_regular_file() && file.path().extension() == extension)
                        expandedFiles.push_back(file.path().string());
                }
            } else {
                expandedFiles.push_back(entry);
            }
        }

        for (const auto &m : c.v_Modules) {
            expandedFiles.push_back(m);
        }
        std::partition(expandedFiles.begin(), expandedFiles.end(), [&](const std::string &s) {
            return isModule(s) || isDeclaredModule(s, c.v_Modules);
        });

        if (showOutput) {
            hp::printlnCl(std::format("\n{} {} files to check:", tag, expandedFiles.size()), hp::Color::YELLOW);
            for (const auto &file : expandedFiles) {
                std::string mtag = isModule(file) ? "[Module] " : "";
                hp::printlnCl(std::format("- {}{}", mtag, file), hp::Color::YELLOW);
            }
            std::cout << '\n';
            if (c.rebuild)
                hp::printlnCl(std::format("{} Rebuilding forced, ignoring timestamps.", tag), hp::Color::YELLOW);
        }

        std::vector<std::string> objects;

        for (const auto &file : expandedFiles) {
            std::string objCmd;
            std::string baseName = std::filesystem::path(file).stem().string();
            std::string objPath;

            bool isMod = isModule(file);
            bool userDeclared = isDeclaredModule(file, c.v_Modules);
            bool treatAsModule = isMod || userDeclared;

            if (isClang && treatAsModule) {
                std::string moduleName = extractModule(file);
                std::string pcmPath = "build/HelpMake/modules/" + baseName + ".pcm";
                std::string moduleObj = "build/HelpMake/obj/" + baseName + ".o";

                std::string langFlag;
                if (!isMod)
                    langFlag = "-xc++-module ";

                bool pcmNeedsRebuild = c.rebuild || !std::filesystem::exists(pcmPath) || std::filesystem::last_write_time(file) > std::filesystem::last_write_time(pcmPath);

                if (pcmNeedsRebuild) {
                    std::string pcmCmd = std::format("{} -fdiagnostics-color=always {}{} --precompile \"{}\" -o \"{}\"",
                                                     comp, langFlag, compileFlags, file, pcmPath);

                    if (showOutput)
                        hp::printlnCl(std::format("\n{} Precompile Command: {}", tag, pcmCmd), hp::Color::YELLOW);

                    std::string redirectCmd = std::format("{} > \"{}\" 2>&1", pcmCmd, r_logPath);
                    auto objTimer = hp::startTimer();
                    exitCode = std::system(redirectCmd.c_str());
                    double objElapsed = hp::stopTimer(objTimer);

                    if (exitCode != 0) {
                        hp::printlnCl(std::format("\n{} Failed to precompile: {}", tag, file), hp::Color::RED);
                        break;
                    }
                    if (showOutput || c.time) {
                        hp::printlnCl(std::format("{} Precompiled '{}' in {}s", tag, file, objElapsed), hp::Color::YELLOW);
                    }
                }

                objPath = moduleObj;
                objCmd = std::format("{} -fdiagnostics-color=always {} -c \"{}\" -o \"{}\" {} -MMD -fmodule-file={}={}",
                                     comp, langFlag, file, moduleObj, compileFlags, moduleName, pcmPath);
            } else {
                objPath = std::format("build/HelpMake/obj/{}.o", baseName);
                if (comp == "g++") {
                    objCmd = std::format("{} -fdiagnostics-color=always -xc++ -c \"{}\" -o \"{}\" {} -MMD",
                                         comp, file, objPath, compileFlags);
                } else {
                    objCmd = std::format("{} -fdiagnostics-color=always -c \"{}\" -o \"{}\" {} -MMD",
                                         comp, file, objPath, compileFlags);
                }

                if (isClang && !clangModules.empty()) {
                    for (const auto &[modName, modPath] : clangModules) {
                        objCmd += std::format(" -fmodule-file={}={}", modName, modPath);
                    }
                }
            }

            if (!needsRebuild(file, objPath, c.rebuild)) {
                if (showOutput)
                    hp::printlnCl(std::format("{} Skip (up to date): {}", tag, file), hp::Color::YELLOW);
                objects.push_back(objPath);
                continue;
            }

            if (showOutput)
                hp::printlnCl(std::format("\n{} Object Command: {}", tag, objCmd), hp::Color::YELLOW);

            std::string redirectCmd = std::format("{} > \"{}\" 2>&1", objCmd, r_logPath);
            auto objTime = hp::startTimer();
            exitCode = std::system(redirectCmd.c_str());
            double objElapsed = hp::stopTimer(objTime);

            if (exitCode != 0) {
                hp::printlnCl(std::format("\n{} Failed to compile: {}", tag, file), hp::Color::RED);
                break;
            }

            if (showOutput || c.time)
                hp::printlnCl(std::format("{} Compiled '{}' in {}s", tag, file, objElapsed), hp::Color::YELLOW);

            objects.push_back(objPath);
        }

        if (exitCode == 0) {
            std::string linkCmd = comp + " -fdiagnostics-color=always ";

            if (c.kind == "static") {
                auto parent = std::filesystem::path(c.output).parent_path();
                if (!parent.empty())
                    std::filesystem::create_directories(parent);
                std::filesystem::remove(c.output);

                linkCmd = "ar rcs \"" + c.output + "\"";
                for (const auto &obj : objects)
                    linkCmd += " \"" + obj + "\"";
            } else {

                if (!compileFlags.empty())
                    linkCmd += compileFlags + " ";
                for (const auto &obj : objects)
                    linkCmd += " \"" + obj + "\"";
                if (!linkFlags.empty())
                    linkCmd += " " + linkFlags;
                linkCmd += " -o \"" + c.output + "\"" + (c.run ? " && " + c.output : "");
            }
            if (showOutput)
                hp::printlnCl(std::format("\n{} Link Command: {}", tag, linkCmd), hp::Color::YELLOW);

            std::string redirectCmd = std::format("{} > \"{}\" 2>&1", linkCmd, r_logPath);
            exitCode = std::system(redirectCmd.c_str());
        }

    } else {
        if (c.kind == "static") {
            hp::printlnCl("Error: 'Kind: static' requires separate compilation. Remove --no-sep.", hp::Color::RED);
            exit(EXIT_FAILURE);
        }

        std::string command = comp + " -fdiagnostics-color=always";
        if (!compileFlags.empty())
            command += " " + compileFlags;
        command += " " + c.inputFile;
        if (!linkFlags.empty())
            command += " " + linkFlags;
        command += " -o " + c.output;
        if (c.run)
            command += " && " + c.output;

        if (showOutput)
            hp::printlnCl(std::format("{} Real Command: {}", tag, command), hp::Color::YELLOW);

        std::string redirectCmd = std::format("{} > \"{}\" 2>&1", command, r_logPath);
        exitCode = std::system(redirectCmd.c_str());
    }
    double elapsed = hp::stopTimer(timer);

    std::string result;
    {
        std::ifstream rawFile(r_logPath);
        if (rawFile.is_open())
            result.assign((std::istreambuf_iterator<char>(rawFile)), std::istreambuf_iterator<char>());
    }

    std::regex ansi_pattern("\x1B\\[[0-9;]*[a-zA-Z]");
    std::string cleanLog = std::regex_replace(result, ansi_pattern, "");
    {
        std::ofstream cleanFile(logPath, std::ios::trunc);
        if (cleanFile.is_open())
            cleanFile << cleanLog;
    }

    if (exitCode == 0) {
        if (c.notify)
            Parser::Notify(elapsed);

        if (isConfig)
            hp::printlnCl(std::format("\nConfig '{}' compiled successfully. Output: {}", name, c.output), hp::Color::GREEN);
        else
            hp::printlnCl(std::format("\nCompilation successful. Output file: {}", c.output), hp::Color::GREEN);

        if (!result.empty() && showOutput)
            std::cout << result << "\n";

        if (!c.Postcmd.empty()) {
            if (showOutput)
                hp::printlnCl(std::format("{} Running post-build command: {}", tag, c.Postcmd), hp::Color::CYAN);
            int Error = std::system(c.Postcmd.c_str());
            if (Error == -1 && !c.quiet)
                hp::printlnCl("Warning: Could not launch post-build command.", hp::Color::YELLOW);
            else if (Error != 0 && !c.quiet)
                hp::printlnCl(std::format("Warning: Post-build command failed (exit code {}).", Error), hp::Color::YELLOW);
            else if (showOutput)
                hp::printlnCl("Post-build command executed successfully.", hp::Color::GREEN);
        }
    } else {
        if (c.notify)
            Parser::Notify(elapsed, false);
#ifdef _WIN32
        std::string whichCmd = std::format("where {} 2>nul", c.compiler == "zig" ? "zig" : comp);
        std::string test = hp::command(whichCmd);
        bool missing = test.find("INFO:") != std::string::npos;
#else
        std::string whichCmd = std::format("which {} 2>/dev/null", c.compiler == "zig" ? "zig" : comp);
        bool missing = std::system(whichCmd.c_str()) != 0;
#endif

        if (missing) {
            hp::printlnCl(std::format("Error: {} compiler not found. Please install it and add it to PATH.", c.compiler), hp::Color::RED);
        } else {
            if (!cleanLog.empty())
                std::cout << result << "\n";
            if (isConfig)
                hp::printlnCl(std::format("Config '{}' compilation failed.", name), hp::Color::RED);
            else
                hp::printlnCl("\nCompilation Failed", hp::Color::RED);
            hp::printlnCl(std::format("See Logs: {}", logPath), hp::Color::YELLOW);
        }
        exit(EXIT_FAILURE);
    }

    if (showOutput || c.time) {
        if (isConfig)
            std::cout << std::format("{} Compiling Time {}s\n", tag, elapsed);
        else
            std::cout << std::format("[HelpMake] Compiling Time: {}s\n", elapsed);
    }
}

Config Parser::inheritConfig(const Config &child) {
    Config local = child;

    if (local.compiler.empty())
        local.compiler = cfg.compiler;
    if (local.version.empty())
        local.version = cfg.version;
    if (local.output.empty())
        local.output = cfg.output;
    if (local.flags.empty())
        local.flags = cfg.flags;
    if (local.inputFile.empty())
        local.inputFile = cfg.inputFile;
    if (local.v_Flags.empty())
        local.v_Flags = cfg.v_Flags;
    if (local.v_inputFiles.empty())
        local.v_inputFiles = cfg.v_inputFiles;
    if (local.includeFiles.empty())
        local.includeFiles = cfg.includeFiles;
    if (local.v_Modules.empty())
        local.v_Modules = cfg.v_Modules;
    if (local.v_Precmd.empty())
        local.v_Precmd = cfg.v_Precmd;
    if (local.v_Postcmd.empty())
        local.v_Postcmd = cfg.v_Postcmd;
    if (local.Precmd.empty())
        local.Precmd = cfg.Precmd;
    if (local.Postcmd.empty())
        local.Postcmd = cfg.Postcmd;
    if (local.kind.empty())
        local.kind = cfg.kind;

    local.verbose = local.verbose || cfg.verbose;
    local.debug = local.debug || cfg.debug;
    local.quiet = local.quiet || cfg.quiet;
    local.notify = local.notify || cfg.notify;
    local.rebuild = local.rebuild || cfg.rebuild;
    local.json = local.json || cfg.json;
    local.time = local.time || cfg.time;
    local.seperate = local.seperate && cfg.seperate;

    return local;
}

void Parser::executeConfigs(const std::vector<std::string> &names) {
    for (const auto &name : names) {
        auto it = configs.find(name);
        if (it == configs.end()) {
            if (cfg.debug)
                hp::printlnCl(std::format("[Debug] Config name is: '{}'", name), hp::Color::YELLOW);
            hp::printlnCl(std::format("Error: Config '{}' was either not found or is invalid.", name), hp::Color::RED);
            hp::printlnCl("Available configs:", hp::Color::YELLOW);
            for (const auto &[n, _] : configs)
                std::cout << std::format("- {}\n", n);
            continue;
        }

        Config local = inheritConfig(it->second);
        executeConfig(local, name);
    }
}

std::string Parser::processGithubEntry(const std::string &value) {
    std::size_t arrow = value.find("->");
    if (arrow == std::string::npos)
        return "";

    std::string url = trim(value.substr(0, arrow));
    std::string folder = trim(value.substr(arrow + 2));

    if (!gitAvailable()) {
        hp::printlnCl("Error: Git system not found. Please install it and add it to PATH.", hp::Color::RED);
        exit(EXIT_FAILURE);
    }

    hp::Folder f;
    if (!f.exists("build/HelpMake/dep"))
        f.create("build/HelpMake/dep");

    if (cfg.verbose)
        hp::printlnCl(std::format("\n[Help_Make] Downloading dependency '{}' ...\n", url), hp::Color::CYAN);

    std::size_t slash = url.find_last_of("/");
    std::string include = url.substr(slash + 1);
    if (include.size() > 4 && include.substr(include.size() - 4) == ".git")
        include.erase(include.size() - 4);

    hp::command(std::format("git clone {} build/HelpMake/dep/{}", url, include));

    std::string fullPath = std::format("build/HelpMake/dep/{}", include);
    if (!folder.empty() && folder != ".")
        fullPath += "/" + folder;

    if (cfg.debug) {
        hp::printlnCl(std::format("[Debug] Fetch Include Folder: '{}'", folder), hp::Color::YELLOW);
        hp::printlnCl(std::format("[Debug] Fetch Clone Folder: '{}'", include), hp::Color::YELLOW);
    }

    return fullPath;
}

void Parser::generateCompileCommands() {
    std::ofstream json("compile_commands.json");
    if (!json.is_open()) {
        if (!cfg.quiet)
            hp::printlnCl("Warning: Could not write compile_commands.json", hp::Color::YELLOW);
        return;
    }

    std::string cwd = std::filesystem::current_path().generic_string();
    std::string comp;
    comp = compilerCommand(cfg.compiler);

    std::string incFlags;
    for (const auto &inc : cfg.includeFiles)
        incFlags += std::format(" -I{}", inc);

    json << "[\n";
    bool first = true;
    for (const auto &file : cfg.v_inputFiles) {
        if (!first)
            json << ",\n";
        first = false;

        std::string command = std::format("{} -{} {} -o {} {}{}",
                                          comp, cfg.version, file, cfg.output, cfg.flags, incFlags);

        json << "  {\n";
        json << "    \"directory\": \"" << cwd << "\",\n";
        json << "    \"command\": \"" << command << "\",\n";
        json << "    \"file\": \"" << file << "\"\n";
        json << "  }";
    }
    json << "\n]\n";
    json.close();

    if (cfg.verbose)
        hp::printlnCl("Generated compile_commands.json", hp::Color::GREEN);
}

bool Parser::needsRebuild(const std::string &src, const std::string &obj, bool rebuild) {
    if (!std::filesystem::exists(src)) {
        hp::printlnCl(std::format("[HelpMake] Error: source file not found: {}", src), hp::Color::RED);
        exit(EXIT_FAILURE);
    }

    if (rebuild)
        return true;

    if (!std::filesystem::exists(obj))
        return true;

    auto srcTime = std::filesystem::last_write_time(src);
    auto objTime = std::filesystem::last_write_time(obj);

    if (srcTime > objTime) {
        if (cfg.debug)
            hp::printlnCl(std::format("\n[Debug] Source file '{}' is newer than object '{}'. Rebuilding.", src, obj), hp::Color::YELLOW);
        return true;
    }

    std::ifstream depFile(std::filesystem::path(obj).replace_filename(std::filesystem::path(obj).stem().string() + ".d").generic_string());
    if (!depFile.is_open()) {
        if (!cfg.quiet)
            hp::printlnCl(std::format("\nWarning: Could not open dependency file for '{}'. Rebuilding.\n", obj), hp::Color::YELLOW);
        return true;
    }
    std::string line;
    auto depTime = std::filesystem::file_time_type::min();

    while (std::getline(depFile, line)) {
        std::istringstream iss(line);
        std::string depFilePath;
        while (iss >> depFilePath) {
            if (depFilePath.find(":") != std::string::npos || depFilePath == "\\" || (depFilePath.length() <= 2 && depFilePath.back() == '\\'))
                continue;

            if (depFilePath == "CXX_IMPORTS" || depFilePath == "+=")
                continue;

            if (depFilePath.find(".c++-module") != std::string::npos)
                continue;

            if (!std::filesystem::exists(depFilePath)) {
                if (!cfg.quiet)
                    hp::printlnCl(std::format("Warning: Dependency file '{}' does not exist. Rebuilding.", depFilePath), hp::Color::YELLOW);
                return true;
            }

            depTime = std::filesystem::last_write_time(depFilePath);
            if (depTime > objTime) {
                if (cfg.debug)
                    hp::printlnCl(std::format("\n[Debug] Dependency '{}' is newer than object '{}'. Rebuilding.", depFilePath, obj), hp::Color::YELLOW);
                return true;
            }
        }
    }

    return false;
}

void Parser::Notify(const double &time, bool success) {
    std::string timeStr;
    if (success) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << time;
        timeStr = " - " + ss.str() + "s";
    }

    std::string status = success ? "successful" : "failed";
    std::string notifyCmd;

#ifdef _WIN32
    notifyCmd = "powershell -NoProfile -Command \""
                "[void][System.Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms'); "
                "$n = New-Object System.Windows.Forms.NotifyIcon; "
                "$n.BalloonTipTitle = 'HelpMake'; "
                "$n.BalloonTipText = 'Build " +
                status + timeStr + "'; "
                                   "$n.Icon = [System.Drawing.SystemIcons]::Information; "
                                   "$n.Visible = $true; "
                                   "$n.ShowBalloonTip(5000)\"";
#else
    notifyCmd = "notify-send 'HelpMake' 'Build " + status + timeStr + "'";
#endif

    std::system(notifyCmd.c_str());
}

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

void Parser::parseTypos(std::size_t num, std::filesystem::path filename,
                        std::string_view line, std::string_view expected) {
    std::string_view trimmed = trim_view(line);
    if (trimmed.empty())
        return;

    std::string_view display = expected;
    if (display.ends_with(':'))
        display.remove_suffix(1);
    else if (display.ends_with('{'))
        display = trim_view(display.substr(0, display.size() - 1));

    if (is_typo(trimmed, expected)) {
        cfg.v_Errors.push_back(std::format("\n[HelpMake] Error: {}:{}: Found '{}'. Did You mean {} ?", filename.string(), num, trimmed, display));
    }
}

void Parser::compileLib() {
}
std::vector<std::string> Parser::splitArgs(const std::string &args) {
    std::vector<std::string> result;
    std::istringstream iss(args);
    std::string token;
    while (iss >> token)
        result.push_back(token);
    return result;
}

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
std::string Parser::getCompiler() {
    return cfg.compiler;
}
std::string Parser::getOutput() {
    return cfg.output;
}
std::string Parser::getVersion() {
    return cfg.version;
}