#include "hm/parser/parser.hpp"
#include "hm/util/strings.hpp"
#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"

#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

std::vector<std::string> Parser::splitArgs(const std::string &args) {
    std::vector<std::string> result;
    std::istringstream iss(args);
    std::string token;
    while (iss >> token)
        result.push_back(token);
    return result;
}

int Parser::parse() {
    if (cfg.kind.empty())
        cfg.kind = "executable";
    if (!isValidKind(cfg.kind))
        setKind(cfg, cfg.kind, 0);

    if (cfg.create)
        createConfig(cfg);

    if (cfg.buildSere || cfg.compiler == "sere")
        return execute();

    if (!cfg.n_file && cfg.verbose)
        std::cout << std::format("[HelpMake] Trying to open file: {}\n", filename.string());

    if (cfg.n_file)
        buildCommand();

    std::fstream file(filename);
    if (!file.is_open()) {
        hp::printlnCl(std::format("Error: Could not open file: {}\n", filename.string()), hp::RED);
        hp::printlnCl("Make sure the file exists in the current directory.", hp::YELLOW);
        hp::printlnCl("Or provide all required arguments on the command line", hp::YELLOW);
        return 1;
    }

    static constexpr std::array<std::string_view, 23> knownKeys = {
        "Compiler:", "Version:", "Output:", "InputFiles:", "Includes:",
        "Flags:", "Modules:", "Run:", "Arguments:", "Kind:", "Jobs:", "PCH:",
        "Copy {", "InputFiles {", "Includes {", "Flags {", "Fetch {",
        "Modules {", "PreBuild {", "PostBuild {", "Arguments {", "Config:", "Set {"};

    static constexpr std::array<std::pair<std::string_view, std::string_view>, 9> blockStarts = {{
        {"InputFiles {", "InputFiles"},
        {"Includes {", "Includes"},
        {"Flags {", "Flags"},
        {"Fetch {", "Fetch"},
        {"Modules {", "Modules"},
        {"PreBuild {", "PreBuild"},
        {"PostBuild {", "PostBuild"},
        {"Arguments {", "Argument"},
        {"Copy {", "Copy"},
    }};

    std::string line;
    std::string currentBlock;
    std::size_t lines = 0;

    while (std::getline(file, line)) {
        ++lines;

        if (std::size_t hash = line.find('#') != std::string::npos)
            line = line.substr(0, hash);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        for (auto key : knownKeys)
            parseTypos(lines, filename, line, key);

        if (isConfigSet) {
            for (char c : line) {
                if (c == '{')
                    configBraceDepth++;
                else if (c == '}')
                    configBraceDepth--;
            }

            if (configBraceDepth <= 0) {
                auto justAdded = currentConfig.name;
                configs[justAdded] = currentConfig;
                debugField("Saved config", justAdded);
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
                auto value = cfg.var.parseVar(trim(line), cfg.v_Errors);
                if (value.empty())
                    continue;

                if (currentBlock == "InputFiles")
                    appendInputFiles(currentConfig, value);
                else if (currentBlock == "Includes")
                    appendIncludes(currentConfig, value);
                else if (currentBlock == "Flags")
                    appendFlags(currentConfig, value);
                else if (currentBlock == "Fetch") {
                    auto path = processGithubEntry(value);
                    if (!path.empty()) {
                        currentConfig.flags += std::format(" -I{}", path);
                        currentConfig.v_Github.push_back(std::format("{} -> {}", value.substr(0, value.find("->")), path));
                    }
                } else if (currentBlock == "Modules")
                    appendModules(currentConfig, value);
                else if (currentBlock == "PreBuild")
                    appendPreBuild(currentConfig, value);
                else if (currentBlock == "PostBuild")
                    appendPostBuild(currentConfig, value);
                else if (currentBlock == "Copy")
                    currentConfig.v_Copy.push_back(value);
                else if (currentBlock == "Argument")
                    parseArgument(currentConfig, value);
                continue;
            }

            bool blockMatched = false;
            for (const auto &[pat, name] : blockStarts) {
                if (line.find(pat) != std::string::npos) {
                    currentBlock = std::string(name);
                    blockMatched = true;
                    break;
                }
            }
            if (blockMatched)
                continue;

            if (auto v = fieldValue(line, "Arguments:"); v) {
                parseArgument(currentConfig, *v);
            } else if (auto v = fieldValue(line, "Compiler:"); v) {
                currentConfig.compiler = *v;
                debugField("Compiler", *v, currentConfig.name);
            } else if (auto v = fieldValue(line, "Version:"); v) {
                currentConfig.version = *v;
                debugField("Version", *v, currentConfig.name);
            } else if (auto v = fieldValue(line, "Output:"); v) {
                currentConfig.output = *v;
                debugField("Output", *v, currentConfig.name);
            } else if (auto v = fieldValue(line, "Flags:"); v) {
                appendFlags(currentConfig, *v);
                debugField("Flags", *v, currentConfig.name);
            } else if (auto v = fieldValue(line, "InputFiles:"); v) {
                appendInputFiles(currentConfig, *v);
                debugField("InputFiles", *v, currentConfig.name);
            } else if (auto v = fieldValue(line, "Includes:"); v) {
                currentConfig.includes += (currentConfig.includes.empty() ? "" : " ") + *v;
                appendIncludes(currentConfig, *v);
                debugField("Includes", *v, currentConfig.name);
            } else if (line.find("Run:") != std::string::npos) {
                currentConfig.run = true;
                debugField("Run", "true", currentConfig.name);
            } else if (auto v = fieldValue(line, "Kind:"); v) {
                setKind(currentConfig, *v, lines);
                debugField("Kind", *v, currentConfig.name);
            } else if (auto v = fieldValue(line, "Jobs:"); v) {
                setJobs(currentConfig, *v, lines);
                debugField("Jobs", *v, currentConfig.name);
            }
            continue;
        }

        if (auto v = fieldValue(line, "Arguments:"); v) {
            parseArgument(cfg, *v);
        } else if (line.find("Arguments {") != std::string::npos) {
            isArgSet = true;
        } else if (line.find("}") != std::string::npos && isArgSet) {
            isArgSet = false;
        } else if (isArgSet) {
            auto value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (!value.empty()) {
                debugField("Argument", value);
                parseArgument(cfg, value);
            }
        } else if (auto v = fieldValue(line, "Compiler:"); v) {
            if (cfg.compiler.empty())
                cfg.compiler = *v;
            if (cfg.compiler.empty()) {
                hp::printlnCl("Error: Compiler not specified in the file.", hp::RED);
                return 1;
            }
            debugField("Compiler", cfg.compiler);
        } else if (auto v = fieldValue(line, "Version:"); v) {
            cfg.version = *v;
            if (cfg.version.empty()) {
                hp::printlnCl("Error: Version not specified in the file.", hp::RED);
                return 1;
            }
            debugField("Version", cfg.version);
        } else if (auto v = fieldValue(line, "Output:"); v) {
            if (cfg.output.empty())
                cfg.output = *v;
            if (cfg.output.empty()) {
#ifdef _WIN32
                cfg.output = "a.exe";
#else
                cfg.output = "a.out";
#endif
                hp::printlnCl(std::format("Warning: No output file specified. Using default: {}", cfg.output), hp::YELLOW);
            }
            debugField("Output", cfg.output);
        } else if (auto v = fieldValue(line, "InputFiles:"); v) {
            appendInputFiles(cfg, *v);
        } else if (line.find("InputFiles {") != std::string::npos) {
            isInputFileSet = true;
        } else if (line.find("}") != std::string::npos && isInputFileSet) {
            isInputFileSet = false;
        } else if (isInputFileSet) {
            auto value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (!value.empty()) {
                appendInputFiles(cfg, value);
                debugField("InputFiles", cfg.inputFile);
            }
        } else if (auto v = fieldValue(line, "Includes:"); v) {
            cfg.includes += (cfg.includes.empty() ? "" : " ") + *v;
            appendIncludes(cfg, *v);
        } else if (line.find("Includes {") != std::string::npos) {
            isIncludeSet = true;
        } else if (line.find("}") != std::string::npos && isIncludeSet) {
            isIncludeSet = false;
        } else if (isIncludeSet) {
            auto value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (!value.empty())
                appendIncludes(cfg, value);
        } else if (line.find("Flags {") != std::string::npos) {
            isFlagsSet = true;
        } else if (line.find("}") != std::string::npos && isFlagsSet) {
            isFlagsSet = false;
        } else if (isFlagsSet) {
            auto value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (!value.empty())
                appendFlags(cfg, value);
        } else if (auto v = fieldValue(line, "Flags:"); v) {
            appendFlags(cfg, *v);
            debugField("Flags", cfg.flags);
        } else if (line.find("Fetch {") != std::string::npos) {
            isFetchSet = true;
        } else if (line.find("}") != std::string::npos && isFetchSet) {
            isFetchSet = false;
        } else if (isFetchSet) {
            auto value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (value.empty())
                continue;
            auto path = processGithubEntry(value);
            if (!path.empty()) {
                cfg.flags += std::format(" -I{}", path);
                cfg.v_Github.push_back(std::format("{} -> {}", value.substr(0, value.find("->")), path));
            }
        } else if (auto v = fieldValue(line, "Modules:"); v) {
            appendModules(cfg, *v);
            debugField("Modules", cfg.modules);
        } else if (line.find("Modules {") != std::string::npos) {
            isModulesSet = true;
        } else if (line.find("}") != std::string::npos && isModulesSet) {
            isModulesSet = false;
        } else if (isModulesSet) {
            auto value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (!value.empty())
                appendModules(cfg, value);
        } else if (line.find("PreBuild {") != std::string::npos) {
            isPreBuildSet = true;
        } else if (line.find("}") != std::string::npos && isPreBuildSet) {
            isPreBuildSet = false;
        } else if (isPreBuildSet) {
            auto value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (!value.empty()) {
                appendPreBuild(cfg, value);
                debugField("PreBuild", value);
            }
        } else if (line.find("PostBuild {") != std::string::npos) {
            isPostBuildSet = true;
        } else if (line.find("}") != std::string::npos && isPostBuildSet) {
            isPostBuildSet = false;
        } else if (isPostBuildSet) {
            auto value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (!value.empty()) {
                appendPostBuild(cfg, value);
                debugField("PostBuild", value);
            }
        } else if (line.find("Run:") != std::string::npos) {
            cfg.run = true;
            debugField("Run", "true");
        } else if (auto v = fieldValue(line, "Kind:"); v) {
            setKind(cfg, *v, lines);
            debugField("Kind", *v);
        } else if (auto v = fieldValue(line, "Jobs:"); v) {
            setJobs(cfg, *v, lines);
            debugField("Jobs", *v);
        } else if (line.find("Copy {") != std::string::npos) {
            isCopySet = true;
        } else if (line.find("}") != std::string::npos && isCopySet) {
            isCopySet = false;
        } else if (isCopySet) {
            auto value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (!value.empty()) {
                cfg.v_Copy.push_back(value);
                debugField("Copy", value);
            }
        } else if (line.find("Set {") != std::string::npos) {
            isVarSet = true;
        } else if (line.find("}") != std::string::npos && isVarSet) {
            isVarSet = false;
        } else if (isVarSet) {
            auto pos = line.find(':');
            if (pos == std::string::npos) {
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Missing ':' in '{}'", filename.string(), lines, line));
                continue;
            }
            auto key = trim(line.substr(0, pos));
            if (key.empty()) {
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Empty variable name in '{}'", filename.string(), lines, line));
                continue;
            }
            if (auto it = cfg.v_Vars.find(key); it != cfg.v_Vars.end()) {
                debugField("Variable override", std::format("{} = {}", key, it->second));
                cfg.var.defineVar(key, it->second);
                continue;
            }
            auto value = cfg.var.parseVar(trim(line.substr(pos + 1)), cfg.v_Errors);
            cfg.var.defineVar(key, value);
        } else if (auto v = fieldValue(line, "PCH:"); v) {
            cfg.pch = *v;
            debugField("PCH", cfg.pch);
        } else if (auto pos = line.find("Config:"); pos != std::string::npos) {
            std::string rest = line.substr(pos + 7);
            auto brace = rest.find('{');
            if (brace == std::string::npos)
                continue;
            auto name = cfg.var.parseVar(trim(rest.substr(0, brace)), cfg.v_Errors);
            if (name.empty()) {
                cfg.v_Errors.push_back(std::format("Error: {}:{}: Config name is empty.", filename.string(), lines));
                continue;
            }
            isConfigSet = true;
            currentConfig = Config{};
            currentConfig.name = name;
            configBraceDepth = 1;
            debugField("Config", name);
        }
    }

    if (!cfg.v_Errors.empty()) {
        for (const auto &e : cfg.v_Errors)
            hp::printCl(e, hp::RED);
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
