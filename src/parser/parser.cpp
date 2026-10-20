#include "hm/parser/parser.hpp"
#include "hm/util/strings.hpp"
#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"

#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string_view>
#include <vector>

int Parser::parse() {

    if (cfg.kind.empty())
        cfg.kind = "executable";

    if (!isValidKind(cfg.kind)) {
        cfg.v_Errors.push_back(std::format("[HelpMake] Error: Invalid Kind '{}'", cfg.kind));
    }

    if (cfg.create) {
        createConfig(cfg);
    }

    if (cfg.buildSere || cfg.compiler == "sere") {
        return execute();
    }

    if (!cfg.n_file && cfg.verbose)
        std::cout << std::format("[HelpMake] Trying to open file: {}\n", filename.string());

    if (cfg.n_file) {
        buildCommand();
    }

    std::fstream file(filename);
    if (!file.is_open()) {
        hp::printlnCl(std::format("Error: Could not open file: {}\n", filename.string()), hp::RED);
        hp::printlnCl("Make sure the file exists in the current directory.", hp::YELLOW);
        hp::printlnCl("Or provide all required arguments on the command line", hp::YELLOW);
        return 1;
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
        "Jobs:",
        "Copy {",
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
                std::string value = cfg.var.parseVar(trim(line), cfg.v_Errors);
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
                } else if (currentBlock == "Copy") {
                    currentConfig.v_Copy.push_back(value);
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
            if (line.find("Copy {") != std::string::npos) {
                currentBlock = "Copy";
                continue;
            }

            std::string value;
            if (line.find("Arguments:") != std::string::npos) {
                std::string value = cfg.var.parseVar(extractValue(line, "Arguments:"), cfg.v_Errors);
                parseArgument(currentConfig, value);
            } else if (line.find("Compiler:") != std::string::npos) {
                value = cfg.var.parseVar(extractValue(line, "Compiler:"), cfg.v_Errors);
                currentConfig.compiler = value;
                if (cfg.debug)
                    std::cout << std::format("Compiler found in config '{}': '{}'\n", currentConfig.name, value);

            } else if (line.find("Version:") != std::string::npos) {
                value = cfg.var.parseVar(extractValue(line, "Version:"), cfg.v_Errors);
                currentConfig.version = value;
                if (cfg.debug)
                    std::cout << std::format("Version found in config '{}': '{}'\n", currentConfig.name, value);

            } else if (line.find("Output:") != std::string::npos) {
                value = cfg.var.parseVar(extractValue(line, "Output:"), cfg.v_Errors);
                currentConfig.output = value;
                if (cfg.debug)
                    std::cout << std::format("Output found in config '{}': '{}'\n", currentConfig.name, value);

            } else if (line.find("Flags:") != std::string::npos) {
                value = cfg.var.parseVar(extractValue(line, "Flags:"), cfg.v_Errors);
                if (currentConfig.flags.empty())
                    currentConfig.flags = value;
                else
                    currentConfig.flags += " " + value;
                currentConfig.v_Flags.push_back(value);
                if (cfg.debug)
                    std::cout << std::format("Flag found in config '{}': '{}'\n", currentConfig.name, value);

            } else if (line.find("InputFiles:") != std::string::npos) {
                value = cfg.var.parseVar(extractValue(line, "InputFiles:"), cfg.v_Errors);
                if (currentConfig.inputFile.empty())
                    currentConfig.inputFile = value;
                else
                    currentConfig.inputFile += " " + value;
                currentConfig.v_inputFiles.push_back(value);
                if (cfg.debug)
                    std::cout << std::format("Input file found in config '{}': '{}'\n", currentConfig.name, value);

            } else if (line.find("Includes:") != std::string::npos) {
                value = cfg.var.parseVar(extractValue(line, "Includes:"), cfg.v_Errors);
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
                value = cfg.var.parseVar(extractValue(line, "Kind:"), cfg.v_Errors);
                currentConfig.kind = to_lower(value);
                if (!isValidKind(currentConfig.kind))
                    hp::printlnCl(std::format("[HelpMake] Error: {}:{}: Invalid Kind '{}' ", filename.string(), lines, currentConfig.kind), hp::RED);
                if (cfg.debug)
                    std::cout << std::format("Kind found in config '{}': '{}'\n", currentConfig.name, value);
            } else if (line.find("Jobs:") != std::string::npos) {
                value = cfg.var.parseVar(extractValue(line, "Jobs:"), cfg.v_Errors);
                try {
                    currentConfig.jobs = std::stoul(value);
                } catch (const std::invalid_argument &) {
                    cfg.v_Errors.push_back(std::format("[HelpMake] Error: '{}' is not a valid number for 'jobs'", value));
                } catch (const std::out_of_range &) {
                    cfg.v_Errors.push_back(std::format("[HelpMake] Error: '{}' is out of range for 'jobs'", value));
                }
                if (cfg.debug)
                    std::cout << std::format("Jobs found in config '{}': '{}'\n", currentConfig.name, value);
            }
            continue;
        }
        std::string value;

        if (line.find("Arguments:") != std::string::npos) {
            value = cfg.var.parseVar(extractValue(line, "Arguments:"), cfg.v_Errors);
            parseArgument(cfg, value);
        } else if (line.find("Arguments {") != std::string::npos) {
            isArgSet = true;
        } else if (line.find("}") != std::string::npos && isArgSet) {
            isArgSet = false;
        } else if (isArgSet) {
            value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (!value.empty()) {
                if (cfg.debug)
                    hp::printlnCl(std::format("Found Argument: {}", value), hp::YELLOW);
                parseArgument(cfg, value);
            }
        } else if (line.find("Compiler:") != std::string::npos) {
            value = cfg.var.parseVar(extractValue(line, "Compiler:"), cfg.v_Errors);
            if (cfg.compiler.empty())
                cfg.compiler = value;

            if (cfg.compiler.empty()) {
                hp::printlnCl("Error: Compiler not specified in the file.", hp::RED);
                return 1;
            }

            if (cfg.debug)
                std::cout << std::format("Found Compiler: '{}'\n", cfg.compiler);
        } else if (line.find("Version:") != std::string::npos) {
            cfg.version = cfg.var.parseVar(extractValue(line, "Version:"), cfg.v_Errors);
            if (cfg.version.empty()) {
                hp::printlnCl("Error: Version not specified in the file.", hp::RED);
                return 1;
            }
            if (cfg.debug)
                std::cout << std::format("Found Version: '{}'\n", cfg.version);
        } else if (line.find("Output:") != std::string::npos) {
            value = cfg.var.parseVar(extractValue(line, "Output:"), cfg.v_Errors);
            if (cfg.output.empty())
                cfg.output = value;
            if (cfg.output.empty()) {
#ifdef _WIN32
                cfg.output = "a.exe";
#else
                cfg.output = "a.out";
#endif
                hp::printlnCl(std::format("Warning: No output file specified. Using default: {}", cfg.output), hp::YELLOW);
            }
            if (cfg.debug)
                std::cout << std::format("Found Output: '{}'\n", cfg.output);
        } else if (line.find("InputFiles:") != std::string::npos) {
            value = cfg.var.parseVar(extractValue(line, "InputFiles:"), cfg.v_Errors);
            cfg.inputFile += (cfg.inputFile.empty() ? "" : " ") + value;
        } else if (line.find("InputFiles {") != std::string::npos) {
            isInputFileSet = true;
        } else if (line.find("}") != std::string::npos && isInputFileSet) {
            isInputFileSet = false;
        } else if (isInputFileSet) {
            value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (!value.empty()) {
                cfg.inputFile += (cfg.inputFile.empty() ? "" : " ") + value;
                cfg.v_inputFiles.push_back(value);
                if (cfg.debug)
                    std::cout << std::format("Found Input Files: '{}'\n", cfg.inputFile);
            }
        } else if (line.find("Includes:") != std::string::npos) {
            value = cfg.var.parseVar(extractValue(line, "Includes:"), cfg.v_Errors);
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
            value = cfg.var.parseVar(trim(line), cfg.v_Errors);
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
            value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (!value.empty()) {
                cfg.flags += (cfg.flags.empty() ? "" : " ") + value;
                cfg.v_Flags.push_back(value);
            }
        } else if (line.find("Flags:") != std::string::npos) {
            cfg.flags += (cfg.flags.empty() ? "" : " ") + cfg.var.parseVar(extractValue(line, "Flags:"), cfg.v_Errors);
            if (cfg.debug)
                std::cout << std::format("Found Flags: '{}'\n", cfg.flags);
        } else if (line.find("Fetch {") != std::string::npos) {
            isFetchSet = true;
        } else if (line.find("}") != std::string::npos && isFetchSet) {
            isFetchSet = false;
        } else if (isFetchSet) {
            value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (value.empty())
                continue;

            std::string path = processGithubEntry(value);
            if (!path.empty()) {
                cfg.flags += std::format(" -I{}", path);
                cfg.v_Github.push_back(std::format("{} -> {}", value.substr(0, value.find("->")), path));
            }
        } else if (line.find("Modules:") != std::string::npos) {
            value = cfg.var.parseVar(extractValue(line, "Modules:"), cfg.v_Errors);
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
            value = cfg.var.parseVar(trim(line), cfg.v_Errors);
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
            value = cfg.var.parseVar(trim(line), cfg.v_Errors);
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
            value = cfg.var.parseVar(trim(line), cfg.v_Errors);
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
            value = cfg.var.parseVar(extractValue(line, "Kind:"), cfg.v_Errors);
            cfg.kind = to_lower(value);
            if (!isValidKind(cfg.kind))
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Invalid Kind '{}' ", filename.string(), lines, cfg.kind));
            if (cfg.debug)
                std::cout << std::format("Found Kind: '{}'\n", value);
        } else if (line.find("Jobs:") != std::string::npos) {
            value = cfg.var.parseVar(extractValue(line, "Jobs:"), cfg.v_Errors);
            try {
                if (cfg.jobs == 0)
                    cfg.jobs = std::stoul(value);
            } catch (const std::invalid_argument &) {
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: '{}' is not a valid number for 'jobs'", filename.string(), lines, value));
            } catch (const std::out_of_range &) {
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: '{}' is out of range for 'jobs'", filename.string(), lines, value));
            }
            if (cfg.debug)
                std::cout << std::format("Found Jobs: '{}'\n", value);
        } else if (line.find("Copy {") != std::string::npos) {
            isCopySet = true;
        } else if (line.find("}") != std::string::npos && isCopySet) {
            isCopySet = false;
        } else if (isCopySet) {
            value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (!value.empty()) {
                cfg.v_Copy.push_back(value);
                if (cfg.debug)
                    std::cout << std::format("Needs Copy: {}\n", value);
            }
        } else if (line.find("Set {") != std::string::npos) {
            isVarSet = true;
        } else if (line.find("}") != std::string::npos && isVarSet) {
            isVarSet = false;
        } else if (isVarSet) {
            std::size_t pos = line.find(':');
            if (pos == std::string::npos) {
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Missing ':' in '{}'", filename.string(), lines, line));
                continue;
            }
            std::string key = trim(line.substr(0, pos));
            if (key.empty()) {
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Empty variable name in '{}'", filename.string(), lines, line));
                continue;
            }

            auto it = cfg.v_Vars.find(key);
            if (it != cfg.v_Vars.end()) {
                if (cfg.debug)
                    hp::printlnCl(std::format("[Debug] --var override: '{}' = '{}'", key, it->second), hp::CYAN);
                cfg.var.defineVar(key, it->second);
                continue;
            }

            std::string value = cfg.var.parseVar(trim(line.substr(pos + 1)), cfg.v_Errors);
            cfg.var.defineVar(key, value);
        } else if (line.find("PCH:") != std::string::npos) {
            value = cfg.var.parseVar(extractValue(line, "PCH:"), cfg.v_Errors);
            cfg.pch = value;
        } else if (line.find("Config:") != std::string::npos) {
            std::string rest = line.substr(line.find("Config:") + 7);
            std::size_t brace = rest.find('{');
            if (brace == std::string::npos)
                continue;

            std::string name = cfg.var.parseVar(trim(rest.substr(0, brace)), cfg.v_Errors);
            if (name.empty()) {
                cfg.v_Errors.push_back(std::format("Error: {}:{}: Config name is empty.", filename.string(), lines));
                continue;
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
        return 1;
    }
    file.close();

    if (cfg.output.empty()) {
#ifdef _WIN32
        cfg.output = "a.exe";
#else
        cfg.output = "a.out";
#endif
        hp::printlnCl(std::format("Output not specified in the file. Using default: {}", cfg.output), hp::YELLOW);
    }
    if (cfg.inputFile.empty()) {
        hp::printlnCl("Error: InputFile not found in the file.", hp::RED);
        return 1;
    }

    dotPos = cfg.output.find_last_of('.');
    baseName = (dotPos != std::string::npos) ? cfg.output.substr(0, dotPos) : cfg.output;
    r_logPath = r_logDir + "/" + baseName + ".txt";
    logPath = logDir + "/" + baseName + ".txt";

    return 0;
}

std::vector<std::string> Parser::splitArgs(const std::string &args) {
    std::vector<std::string> result;
    std::istringstream iss(args);
    std::string token;
    while (iss >> token)
        result.push_back(token);
    return result;
}