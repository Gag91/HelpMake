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
#include <thread>
#include <vector>

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

std::vector<std::string> Parser::splitArgs(const std::string &args) {
    std::vector<std::string> result;
    std::istringstream iss(args);
    std::string token;
    while (iss >> token)
        result.push_back(token);
    return result;
}