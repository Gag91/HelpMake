#include "hm/parser/parser.hpp"
#include "hm/generator/generator.hpp"
#include "hm/process/process.hpp"
#include "hm/util/bools.hpp"
#include "hm/util/strings.hpp"
#include "hm/util/system.hpp"
#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"

#include <cctype>
#include <cstdlib>
#include <filesystem>
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
    auto t0 = hp::startTimer();
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
            if (configs.contains(sec.configName)) {
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: Duplicate config name '{}' found. Config names must be unique.", sec.configName));
                continue;
            }

            Config c;
            c.name = sec.configName;
            parseSection(sec, c, false);
            configs[sec.configName] = c;
        }
    }

    if (cfg.compiler.empty()) {
        int rc = autoCompiler(cfg, "[HelpMake]");
        if (rc != 0)
            return rc;
    }

    if (cfg.version.empty()) {
        int rc = autoVersion(cfg, "[HelpMake]");
        if (rc != 0)
            return rc;
    }

    if (cfg.v_inputFiles.empty()) {
        int rc = autoInputFiles(cfg, "[HelpMake]");
        if (rc != 0)
            return rc;
    }

    if (cfg.includeFiles.empty()) {
        int rc = autoIncludes(cfg, "[HelpMake]");
        if (rc != 0)
            return rc;
    }

    if (cfg.v_Flags.empty()) {
        int rc = autoLinkFlags(cfg, "[HelpMake]");
        if (rc != 0)
            return rc;
    }

    if (cfg.kind.empty()) {
        int rc = autoKind(cfg, "[HelpMake]");
        if (rc != 0)
            return rc;
    }

    if (cfg.output.empty()) {
        int rc = autoOutput(cfg, "[HelpMake]");
        if (rc != 0)
            return rc;
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

    normalizeOutput(cfg);
    for (auto &[name, config] : configs) {
        normalizeOutput(config);
    }

    addTime(cfg.v_Times, "Parse", t0);
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

    std::filesystem::path lockDir = "build/HelpMake/dep/lock";
    std::filesystem::path lockPath = lockDir / "HelpMake.lock";

    std::error_code ec;
    std::filesystem::create_directories(lockDir, ec);
    if (ec) {
        hp::printlnCl(std::format("[HelpMake] Error: Cannot create lock directory '{}': {}", lockDir.string(), ec.message()), hp::RED);
        return 1;
    }

    if (std::filesystem::exists(lockPath, ec)) {
        auto status = std::filesystem::status(lockPath, ec);
        if (!ec && (status.permissions() & std::filesystem::perms::owner_write) == std::filesystem::perms::none) {
            std::filesystem::permissions(lockPath, std::filesystem::perms::owner_write,
                                         std::filesystem::perm_options::replace, ec);
            ec.clear();
        }

        std::filesystem::remove_all(lockPath, ec);
        if (ec) {
            hp::printlnCl(std::format("[HelpMake] Error: Existing lockfile '{}' cannot be removed: {}", lockPath.string(), ec.message()), hp::RED);
            hp::printlnCl("  Close any editor or process holding the file, then retry.", hp::YELLOW);
            return 1;
        }
    }

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

    static constexpr std::array<std::string_view, 33> knownKeys = {
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
        "PCH:",
        "Gen:",
        "Test:",
        "Subset:",
        "Sanitize:",
        "Strip:",
        "Copy {",
        "InputFiles {",
        "Includes {",
        "Flags {",
        "Fetch {",
        "Modules {",
        "PreBuild {",
        "PostBuild {",
        "Arguments {",
        "Set {",
        "Dependency:",
        "Depends:",
        "Gen {",
        "Subset {",
        "Package {",
        "Sanitize {",
    };

    static constexpr std::array<std::pair<std::string_view, std::string_view>, 13> blockStarts = {{
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
        {"Subset {", "Subset"},
        {"Package {", "Package"},
        {"Sanitize {", "Sanitize"},
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
            else if (currentBlock == "Subset")
                target.v_Subsets.push_back(value);
            else if (currentBlock == "Package")
                appendPackage(target, value);
            else if (currentBlock == "Sanitize")
                appendSanitize(target, value);

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

            if (auto gitValue = fieldValue(line, "Git:"); gitValue) {
                currentDep.git = *gitValue;
                debugField("Dependency Git", *gitValue);
            } else if (auto urlValue = fieldValue(line, "Url:"); urlValue) {
                currentDep.url = *urlValue;
                debugField("Dependency Url", *urlValue);
            } else if (auto pathValue = fieldValue(line, "Path:"); pathValue) {
                currentDep.path = *pathValue;
                debugField("Dependency Path", *pathValue);
            } else if (auto tagValue = fieldValue(line, "Tag:"); tagValue) {
                currentDep.tag = *tagValue;
                debugField("Dependency Tag", *tagValue);
            } else if (auto commitValue = fieldValue(line, "Commit:"); commitValue) {
                currentDep.commit = *commitValue;
                debugField("Dependency Commit", *commitValue);
            } else if (auto hashValue = fieldValue(line, "Hash:"); hashValue) {
                currentDep.hash = *hashValue;
                debugField("Dependency Hash", *hashValue);
            } else if (auto buildValue = fieldValue(line, "Build:"); buildValue) {
                currentDep.builds.push_back(*buildValue);
                debugField("Dependency Build", *buildValue);
            } else if (auto includeValue = fieldValue(line, "Include:"); includeValue) {
                currentDep.includes.push_back(*includeValue);
                debugField("Dependency Include", *includeValue);
            } else if (auto linkValue = fieldValue(line, "Link:"); linkValue) {
                currentDep.links.push_back(*linkValue);
                debugField("Dependency Link", *linkValue);
            } else if (auto linkDirValue = fieldValue(line, "LinkDir:"); linkDirValue) {
                currentDep.linkDirs.push_back(*linkDirValue);
                debugField("Dependency LinkDir", *linkDirValue);
            } else if (auto linkFlagsValue = fieldValue(line, "LinkFlags:"); linkFlagsValue) {
                currentDep.linkFlags.push_back(*linkFlagsValue);
                debugField("Dependency LinkFlags", *linkFlagsValue);
            } else if (auto dependencyNamesValue = fieldValue(line, "Depends:"); dependencyNamesValue) {
                std::string cleaned;
                for (char c : *dependencyNamesValue)
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

        if (auto argumentsValue = fieldValue(line, "Arguments:"); argumentsValue) {
            parseArgument(target, *argumentsValue);
        } else if (auto compilerValue = fieldValue(line, "Compiler:"); compilerValue) {
            if (target.compiler.empty() || pl.conditional)
                target.compiler = *compilerValue;
            debugField("Compiler", target.compiler, section.configName);
        } else if (auto versionValue = fieldValue(line, "Version:"); versionValue) {
            if (target.version.empty() || pl.conditional)
                target.version = *versionValue;
            debugField("Version", target.version, section.configName);
        } else if (auto outputValue = fieldValue(line, "Output:"); outputValue) {
            if (target.output.empty() || pl.conditional)
                target.output = *outputValue;
            debugField("Output", target.output, section.configName);
        } else if (auto flagsValue = fieldValue(line, "Flags:"); flagsValue) {
            appendFlags(target, *flagsValue);
            debugField("Flags", *flagsValue, section.configName);
        } else if (auto inputFilesValue = fieldValue(line, "InputFiles:"); inputFilesValue) {
            appendInputFiles(target, *inputFilesValue);
        } else if (auto includesValue = fieldValue(line, "Includes:"); includesValue) {
            target.includes += (target.includes.empty() ? "" : " ") + *includesValue;
            appendIncludes(target, *includesValue);
        } else if (line.find("Run:") != std::string_view::npos) {
            target.run = true;
            debugField("Run", "true", section.configName);
        } else if (auto kindValue = fieldValue(line, "Kind:"); kindValue) {
            setKind(target, *kindValue, lineNo);
            debugField("Kind", target.kind, section.configName);
        } else if (auto jobsValue = fieldValue(line, "Jobs:"); jobsValue) {
            setJobs(target, *jobsValue, lineNo);
            debugField("Jobs", target.jobs, section.configName);
        } else if (auto modulesValue = fieldValue(line, "Modules:"); modulesValue) {
            appendModules(target, *modulesValue);
        } else if (auto pchValue = fieldValue(line, "PCH:"); pchValue) {
            if (target.pch.empty() || pl.conditional)
                target.pch = *pchValue;
            debugField("PCH", target.pch, section.configName);
        } else if (auto generatorValue = fieldValue(line, "Gen:"); generatorValue) {
            target.v_Gens.push_back(*generatorValue);
            debugField("Generator", *generatorValue);
        } else if (auto SanitizeValue = fieldValue(line, "Sanitize:"); SanitizeValue) {
            if (!target.sanitize.empty())
                target.sanitize += " ";
            target.sanitize += *SanitizeValue;
            target.v_Sanitize.push_back(*SanitizeValue);
            debugField("Sanitize", *SanitizeValue, section.configName);
        } else if (auto StripValue = fieldValue(line, "Strip:"); StripValue) {
            if (to_lower(*StripValue) == "true")
                target.strip = true;
            else if (to_lower(*StripValue) == "false")
                target.strip = false;
            else if (is_typo(*StripValue, "true")) {
                target.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Unknown Strip value '{}'. Did you mean true ?", filename.string(), lineNo, *StripValue));
                continue;
            } else if (is_typo(*StripValue, "false")) {
                target.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Unknown Strip value '{}'. Did you mean false ?", filename.string(), lineNo, *StripValue));
                continue;
            } else if (is_typo(*StripValue, "true")) {
                target.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Unknown Strip value '{}'", filename.string(), lineNo, *StripValue));
                continue;
            }
            debugField("Strip", *StripValue, section.configName);
        } else if (auto SubsetsValue = fieldValue(line, "Subset:"); SubsetsValue) {
            if (!std::filesystem::exists(*SubsetsValue)) {
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: Subset '{}' isnt a recognized path", filename.string(), lineNo, *SubsetsValue));
                continue;
            }
            target.v_Subsets.push_back(*SubsetsValue);
            debugField("Subset", *SubsetsValue, section.configName);
        } else if (auto testValue = fieldValue(line, "Test:"); testValue) {
            if (isRoot) {
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: 'Test:' is only valid inside a Config block", filename.string(), lineNo));
                continue;
            }

            std::string val = to_lower(trim(*testValue));

            if (val == "true" || val == "1")
                target.test = true;
            else if (val == "false" || val == "0")
                target.test = false;
            else {
                cfg.v_Info[std::format("{} Warning: Unknown value '{}' for Test:, defaulting to false", filename.string(), val)] = hp::YELLOW;
                target.test = false;
            }

            debugField("Test", target.test, section.configName);
        } else if (auto configDependenciesValue = fieldValue(line, "Depends:"); configDependenciesValue) {
            std::string cleaned;
            for (char c : *configDependenciesValue)
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

void Parser::setDependencies(const std::map<std::string, hm::Dependency> &deps) {
    for (const auto &[name, d] : deps) {
        if (!dependencies.contains(name))
            dependencies[name] = d;
    }
}
