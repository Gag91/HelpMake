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

    if (cfg.create)
        createConfig(cfg);
    if (cfg.buildSere || cfg.compiler == "sere")
        return execute();
    if (cfg.n_file)
        return buildCommand();

    std::fstream file(filename);
    if (!file.is_open()) {
        hp::printlnCl(std::format("Error: Could not open file: {}\n", filename.string()), hp::RED);
        return 1;
    }

    auto sections = splitIntoFileSections(file);
    file.close();

    for (auto &sec : sections) {
        if (sec.configName.empty()) {
            parseSection(sec, cfg, true);
        } else {
            Config c;
            c.name = sec.configName;
            parseSection(sec, c, false);
            configs[sec.configName] = c;
            if (cfg.allCfgs)
                executeConfigs({sec.configName});
        }
    }

    if (!cfg.v_Errors.empty()) {
        for (const auto &e : cfg.v_Errors)
            hp::printCl(e, hp::RED);
        hp::printlnCl(std::format("\n[HelpMake] {} Errors generated.\n[HelpMake] Build Failed.",
                                  cfg.v_Errors.size()),
                      hp::RED);
        return 1;
    }

    if (cfg.output.empty()) {
#ifdef _WIN32
        cfg.output = "a.exe";
#else
        cfg.output = "a.out";
#endif
        hp::printlnCl(std::format("Output not specified. Using default: {}", cfg.output), hp::YELLOW);
    }

    dotPos = cfg.output.find_last_of('.');
    baseName = (dotPos != std::string::npos) ? cfg.output.substr(0, dotPos) : cfg.output;
    r_logPath = r_logDir + "/" + baseName + ".txt";
    logPath = logDir + "/" + baseName + ".txt";

    return 0;
}

std::vector<FileSection> Parser::splitIntoFileSections(std::istream &in) {
    std::vector<FileSection> sections;
    FileSection current;
    current.startLine = 1;

    std::string line;
    std::size_t lineNo = 0;
    int braceDepth = 0;
    bool inConfig = false;

    while (std::getline(in, line)) {
        ++lineNo;

        if (std::size_t hash = line.find('#'); hash != std::string::npos)
            line = line.substr(0, hash);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (!inConfig) {
            if (std::size_t pos = line.find("Config:"); pos != std::string::npos) {
                if (!current.lines.empty() || !current.configName.empty() || sections.empty())
                    sections.push_back(std::move(current));
                current = FileSection{};
                current.startLine = lineNo + 1;

                std::string rest = line.substr(pos + 7);
                std::size_t brace = rest.find('{');
                if (brace == std::string::npos) {
                    cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Config block missing '{{'", filename.string(), lineNo));
                    continue;
                }

                current.configName = trim(rest.substr(0, brace));
                if (current.configName.empty()) {
                    cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Config name is empty.", filename.string(), lineNo));
                }

                inConfig = true;
                braceDepth = 1;
                continue;
            }
            current.lines.push_back(line);
        } else {
            for (char c : line) {
                if (c == '{')
                    braceDepth++;
                else if (c == '}')
                    braceDepth--;
            }
            if (braceDepth <= 0) {
                sections.push_back(std::move(current));
                current = FileSection{};
                inConfig = false;
                continue;
            }
            current.lines.push_back(line);
        }
    }

    if (inConfig) {
        cfg.v_Errors.push_back(std::format("[HelpMake] Error: Unclosed Config block starting at line {}", current.startLine));
    }

    if (!current.lines.empty() || !current.configName.empty())
        sections.push_back(std::move(current));

    return sections;
}

int Parser::parseSection(const FileSection &section, Config &target, bool isRoot) {

    std::string currentBlock;
    std::size_t lineNo = section.startLine - 1;
    hm::Dependency currentDep;

    static constexpr std::array<std::string_view, 24> knownKeys = {
        "Compiler:", "Version:", "Output:", "InputFiles:", "Includes:",
        "Flags:", "Modules:", "Run:", "Arguments:", "Kind:", "Jobs:", "PCH:",
        "Copy {", "InputFiles {", "Includes {", "Flags {", "Fetch {",
        "Modules {", "PreBuild {", "PostBuild {", "Arguments {", "Set {",
        "Dependency:", "Depends:"};

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

    for (const auto &line : section.lines) {
        ++lineNo;

        for (auto key : knownKeys)
            parseTypos(lineNo, filename, line, key);

        if (!currentBlock.empty()) {
            if (line.find("}") != std::string::npos) {
                currentBlock.clear();
                continue;
            }
            std::string value = cfg.var.parseVar(trim(line), cfg.v_Errors);
            if (value.empty())
                continue;

            if (currentBlock == "InputFiles")
                appendInputFiles(target, value);
            else if (currentBlock == "Includes")
                appendIncludes(target, value);
            else if (currentBlock == "Flags")
                appendFlags(target, value);
            else if (currentBlock == "Fetch") {
                std::string path = processGithubEntry(value);
                if (!path.empty()) {
                    target.flags += std::format(" -I{}", path);
                    target.v_Github.push_back(std::format("{} -> {}", value.substr(0, value.find("->")), path));
                }
            } else if (currentBlock == "Modules")
                appendModules(target, value);
            else if (currentBlock == "PreBuild")
                appendPreBuild(target, value);
            else if (currentBlock == "PostBuild")
                appendPostBuild(target, value);
            else if (currentBlock == "Copy")
                target.v_Copy.push_back(value);
            else if (currentBlock == "Argument")
                parseArgument(target, value);
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

        if (isDependencySet) {
            if (line.find("}") != std::string::npos) {
                dependencies[currentDep.name] = currentDep;
                debugField("Dependency saved", currentDep.name);
                currentDep = hm::Dependency{};
                isDependencySet = false;
                continue;
            }

            if (auto v = fieldValue(line, "Git:"); v) {
                currentDep.git = *v;
                debugField("Dependency Git", *v);
            } else if (auto v = fieldValue(line, "Url:"); v) {
                currentDep.url = *v;
                debugField("Dependency Url", *v);
            } else if (auto v = fieldValue(line, "Path:"); v) {
                currentDep.path = *v;
                debugField("Dependency Path", *v);
            } else if (auto v = fieldValue(line, "Tag:"); v) {
                currentDep.tag = *v;
                debugField("Dependency Tag", *v);
            } else if (auto v = fieldValue(line, "Commit:"); v) {
                currentDep.commit = *v;
                debugField("Dependency Commit", *v);
            } else if (auto v = fieldValue(line, "Hash:"); v) {
                currentDep.hash = *v;
                debugField("Dependency Hash", *v);
            } else if (auto v = fieldValue(line, "Build:"); v) {
                currentDep.builds.push_back(*v);
                debugField("Dependency Build", *v);
            } else if (auto v = fieldValue(line, "Include:"); v) {
                currentDep.includes.push_back(*v);
                debugField("Dependency Include", *v);
            } else if (auto v = fieldValue(line, "Link:"); v) {
                currentDep.links.push_back(*v);
                debugField("Dependency Link", *v);
            } else if (auto v = fieldValue(line, "LinkDir:"); v) {
                currentDep.linkDirs.push_back(*v);
                debugField("Dependency LinkDir", *v);
            } else if (auto v = fieldValue(line, "LinkFlags:"); v) {
                currentDep.linkFlags.push_back(*v);
                debugField("Dependency LinkFlags", *v);
            } else if (auto v = fieldValue(line, "Depends:"); v) {
                std::string cleaned;
                for (char c : *v)
                    cleaned += (c == ',') ? ' ' : c;
                for (const auto &depName : splitArgs(cleaned))
                    if (!depName.empty())
                        currentDep.depends.push_back(depName);
                debugField("Dependency Depends", cleaned);
            }

            continue;
        }

        if (line.find("Dependency:") != std::string::npos) {
            if (!isRoot) {
                cfg.v_Errors.push_back(std::format(
                    "[HelpMake] Error: {}:{}: Dependency blocks are only valid at the root level",
                    filename.string(), lineNo));
                continue;
            }
            std::size_t colon = line.find(":");
            std::size_t brace = line.find("{");
            if (colon == std::string::npos || brace == std::string::npos || brace < colon) {
                cfg.v_Errors.push_back(std::format(
                    "[HelpMake] Error: {}:{}: Malformed Dependency line: '{}'",
                    filename.string(), lineNo, line));
                continue;
            }
            std::string name = trim(line.substr(colon + 1, brace - colon - 1));
            if (name.empty()) {
                cfg.v_Errors.push_back(std::format(
                    "[HelpMake] Error: {}:{}: Dependency name is empty",
                    filename.string(), lineNo));
                continue;
            }
            currentDep = hm::Dependency{};
            currentDep.name = name;
            isDependencySet = true;
            debugField("Dependency", name);
            continue;
        }

        if (auto v = fieldValue(line, "Arguments:"); v) {
            parseArgument(target, *v);
        } else if (auto v = fieldValue(line, "Compiler:"); v) {
            if (target.compiler.empty())
                target.compiler = *v;
            debugField("Compiler", target.compiler, section.configName);
        } else if (auto v = fieldValue(line, "Version:"); v) {
            if (target.version.empty())
                target.version = *v;
            debugField("Version", target.version, section.configName);
        } else if (auto v = fieldValue(line, "Output:"); v) {
            if (target.output.empty())
                target.output = *v;
            debugField("Output", target.output, section.configName);
        } else if (auto v = fieldValue(line, "Flags:"); v) {
            appendFlags(target, *v);
            debugField("Flags", *v, section.configName);
        } else if (auto v = fieldValue(line, "InputFiles:"); v) {
            appendInputFiles(target, *v);
        } else if (auto v = fieldValue(line, "Includes:"); v) {
            target.includes += (target.includes.empty() ? "" : " ") + *v;
            appendIncludes(target, *v);
        } else if (line.find("Run:") != std::string::npos) {
            target.run = true;
            debugField("Run", "true", section.configName);
        } else if (auto v = fieldValue(line, "Kind:"); v) {
            setKind(target, *v, lineNo);
            debugField("Kind", target.kind, section.configName);
        } else if (auto v = fieldValue(line, "Jobs:"); v) {
            setJobs(target, *v, lineNo);
            debugField("Jobs", target.jobs, section.configName);
        } else if (auto v = fieldValue(line, "Modules:"); v) {
            appendModules(target, *v);
        } else if (auto v = fieldValue(line, "PCH:"); v) {
            if (target.pch.empty())
                target.pch = *v;
            debugField("PCH", target.pch, section.configName);
        } else if (auto v = fieldValue(line, "Depends:"); v) {
            std::string cleaned;
            for (char c : *v)
                cleaned += (c == ',') ? ' ' : c;
            for (const auto &name : splitArgs(cleaned))
                if (!name.empty())
                    target.v_Depends.push_back(name);
            debugField("Depends", cleaned, section.configName);
        } else if (line.find("Set {") != std::string::npos) {
            isVarSet = true;
        } else if (line.find("}") != std::string::npos && isVarSet) {
            isVarSet = false;
        } else if (isVarSet) {
            std::size_t pos = line.find(':');
            if (pos == std::string::npos) {
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Missing ':' in '{}'", filename.string(), lineNo, line));
                continue;
            }
            std::string key = trim(line.substr(0, pos));
            if (key.empty())
                continue;
            if (auto it = cfg.v_Vars.find(key); it != cfg.v_Vars.end()) {
                cfg.var.defineVar(key, it->second);
                continue;
            }
            std::string value = cfg.var.parseVar(trim(line.substr(pos + 1)), cfg.v_Errors);
            cfg.var.defineVar(key, value);
        }
    }
    return 0;
}
