#include "hm/parser/parser.hpp"
#include "hm/generator/generator.hpp"
#include "hm/process/process.hpp"
#include "hm/util/strings.hpp"
#include "hm/util/system.hpp"
#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"

#include <cctype>
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
    if (cfg.frozen) {
        std::filesystem::path lockPath = "build/HelpMake/dep/lock/HelpMake.lock";
        std::ifstream lockFile(lockPath);
        if (!lockFile) {
            hp::printlnCl(std::format("[HelpMake] Error: Frozen build requires lockfile '{}'. Run 'hm --update-lock'.", lockPath.string()), hp::RED);
            return 1;
        }

        std::string line;
        std::size_t lineNumber = 0;
        while (std::getline(lockFile, line)) {
            ++lineNumber;
            if (line.empty() || line[0] == '#')
                continue;

            std::vector<std::string> fields;
            std::size_t start = 0;
            while (true) {
                std::size_t tab = line.find('\t', start);
                fields.push_back(line.substr(start, tab == std::string::npos ? tab : tab - start));
                if (tab == std::string::npos)
                    break;
                start = tab + 1;
            }

            if (fields.size() != 4 || (fields[0] != "fetch" && fields[0] != "dependency")) {
                hp::printlnCl(std::format("[HelpMake] Error: Invalid lockfile entry at {}:{}.", lockPath.string(), lineNumber), hp::RED);
                return 1;
            }
            if ((fields[3].size() != 40 && fields[3].size() != 64) ||
                !std::ranges::all_of(fields[3], [](unsigned char c) { return std::isxdigit(c) != 0; })) {
                hp::printlnCl(std::format("[HelpMake] Error: Invalid Git commit in lockfile at {}:{}.", lockPath.string(), lineNumber), hp::RED);
                return 1;
            }
            if (fields[1].empty() || fields[2].empty() || (fields[0] == "fetch" && fields[1] != fields[2])) {
                hp::printlnCl(std::format("[HelpMake] Error: Incomplete lockfile entry at {}:{}.", lockPath.string(), lineNumber), hp::RED);
                return 1;
            }

            std::string key = (fields[0] == "fetch") ? "fetch:" + fields[1] : "dependency:" + fields[1];
            if (lockEntries.contains(key)) {
                hp::printlnCl(std::format("[HelpMake] Error: Duplicate lockfile entry at {}:{}.", lockPath.string(), lineNumber), hp::RED);
                return 1;
            }
            lockEntries[key] = {fields[2], fields[3]};
        }
    }

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
        }
    }
    if (cfg.allCfgs) {
        std::vector<std::string> names;
        for (const auto &[n, _] : configs)
            names.push_back(n);
        executeConfigs(names);
    }

    if (!cfg.v_Errors.empty()) {
        for (const auto &e : cfg.v_Errors)
            hp::printCl(e, hp::RED);
        hp::printlnCl(std::format("\n[HelpMake] {} Errors generated.\n[HelpMake] {}", cfg.v_Errors.size(), cfg.check ? "" : "Build Failed"), hp::RED);
        return 1;
    }

    if (cfg.output.empty()) {
#ifdef _WIN32
        cfg.output = "a.exe";
#else
        cfg.output = "a.out";
#endif
        if (!cfg.updateLock)
            hp::printlnCl(std::format("Output not specified. Using default: {}", cfg.output), hp::YELLOW);
    }

    if (!cfg.v_Gens.empty() && !cfg.check && !cfg.updateLock) {
        Config genCfg = cfg;
        if (cfg.dryRun) {
            for (const auto &g : cfg.v_Gens) {
                if (g == "json" || g == "compile_commands") {
                    cfg.v_Info["[HelpMake] Dry run: would generate compile_commands.json"] = hp::CYAN;
                } else if (auto gen = hm::gen::makeGenerators(g)) {
                    cfg.v_Info[std::format("[HelpMake] Dry run: would generate {}", gen->Filename())] = hp::CYAN;
                }
            }
        } else {
            resolveDependencies(genCfg, "[HelpMake]");
            for (const auto &g : cfg.v_Gens) {
                if (g == "json" || g == "compile_commands") {
                    if (generateCompileCommands(genCfg) == 0)
                        cfg.v_Info["[HelpMake] Generated compile_commands.json"] = hp::GREEN;
                } else {
                    auto gen = hm::gen::makeGenerators(g);
                    if (gen->generate(genCfg))
                        cfg.v_Info[std::format("[HelpMake] Generated {}", gen->Filename())] = hp::GREEN;
                }
                for (const auto &[key, value] : genCfg.v_Info)
                    cfg.v_Info[key] = value;
            }
        }
    }

    baseName = std::filesystem::path(cfg.output).stem().string();
    r_logPath = r_logDir + "/" + baseName + ".txt";
    logPath = logDir + "/" + baseName + ".txt";

    return 0;
}

int Parser::updateLock() {
    std::map<std::string, LockEntry> updated;

    auto resolveCommit = [](const std::string &url, const std::string &tag, const std::string &specifiedCommit, std::string &commit) -> bool {
        if (!specifiedCommit.empty()) {
            commit = specifiedCommit;
            if ((commit.size() == 40 || commit.size() == 64) &&
                std::ranges::all_of(commit, [](unsigned char c) { return std::isxdigit(c) != 0; }))
                return true;
            hp::printlnCl(std::format("[HelpMake] Error: Invalid Git commit '{}' for '{}'.", commit, url), hp::RED);
            return false;
        }

        std::string refs = "HEAD";
        if (!tag.empty())
            refs = std::format("\"refs/tags/{}*\" \"refs/heads/{}\"", tag, tag);

        auto result = hm::Process::run(std::format("git ls-remote \"{}\" {}", url, refs));
        if (result.exitCode != 0) {
            hp::printlnCl(std::format("[HelpMake] Error: Could not resolve Git ref '{}' from '{}'.", tag.empty() ? "HEAD" : tag, url), hp::RED);
            if (!result.output.empty())
                hp::printlnCl(result.output, hp::RED);
            return false;
        }

        std::istringstream lines(result.output);
        std::string line;
        while (std::getline(lines, line)) {
            std::size_t separator = line.find('\t');
            if (separator == std::string::npos)
                continue;
            std::string ref = line.substr(separator + 1);
            if (ref == "HEAD" || ref == "refs/tags/" + tag + "^{}" || ref == "refs/tags/" + tag || ref == "refs/heads/" + tag) {
                commit = line.substr(0, separator);
                if (ref.ends_with("^{}"))
                    return true;
                if (ref == "HEAD" || ref == "refs/heads/" + tag)
                    return true;
            }
        }
        if (!commit.empty())
            return true;
        hp::printlnCl(std::format("[HelpMake] Error: Git ref '{}' was not found in '{}'.", tag.empty() ? "HEAD" : tag, url), hp::RED);
        return false;
    };

    for (const auto &[url, _] : fetchRoots) {
        std::string commit;
        if (!resolveCommit(url, "", "", commit))
            return 1;
        updated["fetch:" + url] = {url, commit};
    }

    for (const auto &[name, dep] : dependencies) {
        if (dep.git.empty()) {
            hp::printlnCl(std::format("[HelpMake] Error: Cannot lock non-Git dependency '{}'; use a Git source.", name), hp::RED);
            return 1;
        }
        std::string commit;
        if (!resolveCommit(dep.git, dep.tag, dep.commit, commit))
            return 1;
        updated["dependency:" + name] = {dep.git, commit};
    }

    std::filesystem::path lockPath = filename.parent_path() / "HelpMake.lock";
    std::ofstream lockFile(lockPath, std::ios::trunc);
    if (!lockFile) {
        hp::printlnCl(std::format("[HelpMake] Error: Could not write lockfile '{}'.", lockPath.string()), hp::RED);
        return 1;
    }
    lockFile << "# HelpMake lockfile v1\n";
    for (const auto &[key, entry] : updated) {
        const bool fetch = key.rfind("fetch:", 0) == 0;
        const std::string nameOrUrl = fetch ? key.substr(6) : key.substr(11);
        lockFile << (fetch ? "fetch" : "dependency") << '\t' << nameOrUrl << '\t' << entry.url << '\t' << entry.commit << '\n';
    }
    if (!lockFile) {
        hp::printlnCl(std::format("[HelpMake] Error: Failed while writing lockfile '{}'.", lockPath.string()), hp::RED);
        return 1;
    }

    hp::printlnCl(std::format("[HelpMake] Updated '{}'.", lockPath.string()), hp::GREEN);
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

    if (!isRoot && cfg.debug)
        hp::printlnCl(std::format("\nFound Config {}:", section.configName), hp::CYAN);

    static constexpr std::array<std::string_view, 27> knownKeys = {
        "Compiler:", "Version:", "Output:", "InputFiles:", "Includes:",
        "Flags:", "Modules:", "Run:", "Arguments:", "Kind:", "Jobs:", "PCH:",
        "Gen:", "Test:", "Copy {", "InputFiles {", "Includes {", "Flags {", "Fetch {",
        "Modules {", "PreBuild {", "PostBuild {", "Arguments {", "Set {",
        "Dependency:", "Depends:", "Gen {"};

    static constexpr std::array<std::pair<std::string_view, std::string_view>, 10> blockStarts = {{
        {"InputFiles {", "InputFiles"},
        {"Includes {", "Includes"},
        {"Flags {", "Flags"},
        {"Fetch {", "Fetch"},
        {"Modules {", "Modules"},
        {"PreBuild {", "PreBuild"},
        {"PostBuild {", "PostBuild"},
        {"Arguments {", "Argument"},
        {"Copy {", "Copy"},
        {"Gen {", "Gen"},
    }};

    for (const auto &e_line : section.lines) {
        ++lineNo;

        auto pl = hm::stripPlatformPrefix(e_line);
        if (pl.conditional && !pl.matches)
            continue;
        std::string_view line = pl.conditional ? pl.body : std::string_view(e_line);

        for (auto key : knownKeys)
            parseTypos(lineNo, filename, line, key);

        if (!currentBlock.empty()) {
            if (line.find("}") != std::string_view::npos) {
                currentBlock.clear();
                continue;
            }
            std::string value = cfg.var.parseVar(std::string(trim_view(line)), cfg.v_Errors);
            if (value.empty())
                continue;

            if (currentBlock == "InputFiles")
                appendInputFiles(target, value);
            else if (currentBlock == "Includes")
                appendIncludes(target, value);
            else if (currentBlock == "Flags")
                appendFlags(target, value);
            else if (currentBlock == "Fetch") {
                if (cfg.check)
                    continue;
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
            else if (currentBlock == "Gen")
                target.v_Gens.push_back(value);
            else if (currentBlock == "Argument")
                parseArgument(target, value);

            continue;
        }

        bool blockMatched = false;
        for (const auto &[pat, name] : blockStarts) {
            if (line.find(pat) != std::string_view::npos) {
                currentBlock = std::string(name);
                blockMatched = true;
                break;
            }
        }
        if (blockMatched)
            continue;

        if (isDependencySet) {
            if (line.find("}") != std::string_view::npos) {
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

        if (line.find("Dependency:") != std::string_view::npos) {
            if (!isRoot) {
                cfg.v_Errors.push_back(std::format(
                    "[HelpMake] Error: {}:{}: Dependency blocks are only valid at the root level",
                    filename.string(), lineNo));
                continue;
            }
            std::size_t colon = line.find(":");
            std::size_t brace = line.find("{");
            if (colon == std::string_view::npos || brace == std::string_view::npos || brace < colon) {
                cfg.v_Errors.push_back(std::format(
                    "[HelpMake] Error: {}:{}: Malformed Dependency line: '{}'",
                    filename.string(), lineNo, line));
                continue;
            }
            std::string_view name = trim_view(line.substr(colon + 1, brace - colon - 1));
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
            if (target.compiler.empty() || pl.conditional)
                target.compiler = *v;
            debugField("Compiler", target.compiler, section.configName);
        } else if (auto v = fieldValue(line, "Version:"); v) {
            if (target.version.empty() || pl.conditional)
                target.version = *v;
            debugField("Version", target.version, section.configName);
        } else if (auto v = fieldValue(line, "Output:"); v) {
            if (target.output.empty() || pl.conditional)
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
        } else if (line.find("Run:") != std::string_view::npos) {
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
            if (target.pch.empty() || pl.conditional)
                target.pch = *v;
            debugField("PCH", target.pch, section.configName);
        } else if (auto v = fieldValue(line, "Gen:"); v) {
            target.v_Gens.push_back(*v);
            debugField("Generator", *v);
        } else if (auto v = fieldValue(line, "Test:"); v) {
            if (isRoot) {
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: 'Test:' is only valid inside a Config block", filename.string(), lineNo));
                continue;
            }

            std::string val = to_lower(trim(*v));

            if (val == "true" || val == "1")
                target.test = true;
            else if (val == "false" || val == "0")
                target.test = false;
            else {
                cfg.v_Info[std::format("{} Warning: Unknown value '{}' for Test:, defaulting to false", filename.string(), val)] = hp::YELLOW;
                target.test = false;
            }

            debugField("Test", target.test, section.configName);
        } else if (auto v = fieldValue(line, "Depends:"); v) {
            std::string cleaned;
            for (char c : *v)
                cleaned += (c == ',') ? ' ' : c;
            for (const auto &name : splitArgs(cleaned))
                if (!name.empty())
                    target.v_Depends.push_back(name);
            debugField("Depends", cleaned, section.configName);
        } else if (line.find("Set {") != std::string_view::npos) {
            isVarSet = true;
        } else if (line.find("}") != std::string_view::npos && isVarSet) {
            isVarSet = false;
        } else if (isVarSet) {
            std::size_t pos = line.find(':');
            if (pos == std::string_view::npos) {
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Missing ':' in '{}'", filename.string(), lineNo, line));
                continue;
            }
            std::string key = std::string(trim_view(line.substr(0, pos)));
            if (key.empty())
                continue;
            if (auto it = cfg.v_Vars.find(key); it != cfg.v_Vars.end()) {
                target.var.defineVar(key, it->second);
                continue;
            }
            std::string value = target.var.parseVar(std::string(trim_view(line.substr(pos + 1))), cfg.v_Errors);
            target.var.defineVar(key, value);
        }
    }
    return 0;
}
