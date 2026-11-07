#include "hm/parser/parser.hpp"
#include "hm/process/process.hpp"
#include "hm/toolchain/compiler.hpp"

#include "hp/colors/color.hpp"
#include "hp/containers/containers.hpp"
#include "hp/other/other.hpp"

int Parser::execute() {
    Config local = cfg;
    if (resolveDependencies(local, "[HelpMake]") != 0)
        return 1;
    return executeConfig(local, "");
}

int Parser::buildCommand() {
    if (cfg.compiler.empty()) {
        hp::printlnCl("Error: No compiler specified.", hp::RED);
        return 1;
    }
    if (cfg.inputFile.empty()) {
        hp::printlnCl("Error: No input files specified.", hp::RED);
        return 1;
    }
    if (cfg.output.empty()) {
        if (!cfg.quiet)
            hp::printlnCl("Warning: No output specified. Using a.exe", hp::YELLOW);
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
    return execute();
}

int Parser::resolveDependencies(Config &c, const std::string &tag) {

    for (const auto &depName : c.v_Depends) {
        auto it = dependencies.find(depName);
        if (it == dependencies.end()) {
            hp::printlnCl(std::format("{} Error: Undefined dependency '{}' referenced by 'Depends:'", tag, depName), hp::RED);
            return 1;
        }

        auto comp = hm::makeCompiler(c.compiler);

        auto dep = it->second;
        if (cfg.frozen) {
            auto locked = lockEntries.find("dependency:" + depName);
            if (locked == lockEntries.end() || locked->second.url != dep.git || locked->second.commit.empty()) {
                hp::printlnCl(std::format("{} Error: No valid frozen lock entry for dependency '{}'. Run 'hm --update-lock'.", tag, depName), hp::RED);
                return 1;
            }
            if (dep.git.empty()) {
                hp::printlnCl(std::format("{} Error: Frozen builds currently require Git source for dependency '{}'.", tag, depName), hp::RED);
                return 1;
            }
            dep.commit = locked->second.commit;
        }
        std::string depRoot = "build/HelpMake/dep/" + dep.name;

        if (!c.dryRun && buildDependency(dep) != 0)
            return 1;

        for (const auto &inc : dep.includes) {
            std::string fullPath = depRoot + "/" + inc;
            c.includeFiles.push_back(fullPath);
            c.flags += " " + comp->includeFlag(fullPath);
        }

        for (const auto &ld : dep.linkDirs) {
            if (comp->isMSVC())
                c.flags += " /LIBPATH:" + depRoot + "/" + ld;
            else
                c.flags += " -L" + depRoot + "/" + ld;
        }

        std::string depLinkFlags;
        for (const auto &lf : dep.linkFlags)
            depLinkFlags += " " + lf;

        for (const auto &ln : dep.links)
            depLinkFlags += " " + depRoot + "/" + ln;

        if (!comp->isMSVC() && !depLinkFlags.empty()) {
            c.flags += " -Wl,--start-group " + depLinkFlags + " -Wl,--end-group";
        } else {
            c.flags += depLinkFlags;
        }

        c.v_Info[std::format("{} {} dependency '{}' from '{}'", tag, c.dryRun ? "Would link" : "Linked", depName, depRoot)] = hp::GREEN;
    }
    return 0;
}

int Parser::executeConfig(const Config &c, const std::string &name) {
    auto t0 = hp::startTimer();
    v_DepFiles.clear();
    auto comp = hm::makeCompiler(c.compiler);
    if (!comp) {
        hp::printlnCl(std::format("Error: Unknown compiler '{}'.", c.compiler), hp::RED);
        return 1;
    }

    const std::string compilerDir = "build/HelpMake/" + comp->name();

    baseName = std::filesystem::path(c.output).stem().string();

    r_logDir = compilerDir + "/logs/raw";
    logDir = compilerDir + "/logs";
    r_logPath = r_logDir + "/" + baseName + ".txt";
    logPath = logDir + "/" + baseName + ".txt";

    bool isClang = comp->isClang();
    bool showOutput = c.verbose || c.debug || c.dryRun;
    bool isConfig = !name.empty();

    std::string tag = isConfig ? std::format("[HelpMake] Config '{}':", name) : "[HelpMake]";

    if (c.json) {
        if (c.dryRun)
            hp::printlnCl("[HelpMake] Dry run: would generate compile_commands.json", hp::CYAN);
        else
            Parser::generateCompileCommands(c);
    }

    if (!c.Precmd.empty()) {
        if (showOutput)
            hp::printlnCl(std::format("{} {} pre-build command: {}", tag, c.dryRun ? "Would run" : "Running", c.Precmd), hp::CYAN);

        if (!c.dryRun) {
            auto pre = hm::Process::run(c.Precmd);

            if (pre.exitCode == -1) {
                hp::printlnCl("Error: Could not launch pre-build command.", hp::RED);
                return 1;
            }
            if (pre.exitCode != 0) {
                hp::printlnCl(std::format("Error: Pre-build command failed (exit code {}).", pre.exitCode), hp::RED);
                return 1;
            }
            if (showOutput)
                hp::printlnCl("Pre-build command executed successfully.", hp::GREEN);
        }
    }

    std::string version = normalizeVersion(c.version, c.compiler);

    if (!c.dryRun) {
        std::filesystem::create_directories(r_logDir);
        std::filesystem::create_directories(logDir);
    }

    std::string CommandFlags;

    if (!version.empty())
        CommandFlags = version;
    if (!c.v_Modules.empty() && comp->name() == "gcc" &&
        std::find(c.v_Modules.begin(), c.v_Modules.end(), "-fmodules") == std::ranges::end(c.v_Modules)) {
        if (!CommandFlags.empty())
            CommandFlags += ' ';
        CommandFlags += "-fmodules";
    }
    if (!c.flags.empty()) {
        if (!CommandFlags.empty())
            CommandFlags += ' ';
        CommandFlags += c.flags;
    }

    {
        const std::filesystem::path marker = compilerDir + "/.buildid";
        std::string current = comp->binary();
        for (auto &s : comp->subcommand())
            current += " " + s;
        if (!CommandFlags.empty())
            current += " " + CommandFlags;

        std::string previous;
        if (std::ifstream in(marker); in)
            std::getline(in, previous);

        if (!c.dryRun && previous != current) {
            if (!c.quiet && !previous.empty()) {
                std::size_t width = static_cast<std::size_t>(hp::platform::console_width());
                std::string separator(width, '-');
                hp::printlnCl(std::format("\n{}\n[HelpMake] Build flags changed, invalidating objects.", separator), hp::YELLOW);
                hp::printlnCl(std::format("[HelpMake] Previous:\n '{}'", previous), hp::YELLOW);
                hp::printlnCl(std::format("\n[HelpMake] Current:\n '{}'", current), hp::YELLOW);
                hp::printlnCl(separator + "\n", hp::YELLOW);
            }
            std::error_code ec;
            std::filesystem::remove_all(compilerDir + "/obj", ec);
            std::filesystem::remove_all("build/HelpMake/headers/" + comp->name(), ec);
            std::filesystem::create_directories(compilerDir);
            std::ofstream(marker) << current;
        }
    }

    std::string compileFlags, linkFlags;

    {
        std::istringstream iss(CommandFlags);
        std::string tok;
        while (iss >> tok) {
            bool isLinker = tok.rfind("-l", 0) == 0 || tok.rfind("/l", 0) == 0 || tok.ends_with(".a") || tok.ends_with(".lib") || tok.ends_with(".so");
            if (isLinker) {
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

    if (isConfig && !c.quiet) {
        hp::printlnCl(std::format("\n{} config: {}", c.dryRun ? "Would build" : "Building", name), hp::CYAN);
        if (showOutput) {
            std::string displayCommand = std::format("{} {} {} -o {}", comp->binary(), CommandFlags, c.inputFile, c.output);
            std::cout << std::format("Config '{}': Command: {}\n", name, displayCommand);
        }
    }

    int exitCode = 0;
    auto timer = hp::startTimer();

    std::string pchBinaryPath;

    if (!c.pch.empty()) {
        std::string pchHeader = c.pch;
        std::string pchBinary = "build/HelpMake/headers/" + comp->name() + "/" + (isConfig ? name + "/" : std::string{}) +
                                std::filesystem::path(pchHeader).filename().string() + comp->pchExtension();
        std::string pchDir = "build/HelpMake/headers/" + comp->name();
        if (!c.dryRun)
            std::filesystem::create_directories(pchDir);
        if (PchneedsRebuild(pchHeader, pchBinary, c.rebuild)) {
            if (!c.dryRun)
                std::filesystem::create_directories(std::filesystem::path(pchBinary).parent_path());

            std::vector<std::string> args = comp->pchCreateArgs(pchHeader, pchBinary);
            std::vector<std::string> full = {comp->binary()};
            for (auto &s : comp->subcommand())
                full.push_back(s);

            for (const auto &f : splitArgs(compileFlags))
                full.push_back(f);

            for (const auto &a : args)
                full.push_back(a);

            if (comp->pchNeedsSourceFile()) {
                std::string pchSrc = pchDir + "/pch.cpp";
                std::string pchObj = pchDir + "/pch.obj";
                if (!c.dryRun)
                    std::ofstream(pchSrc) << comp->pchSourceFileContent(pchHeader);
                full.push_back(pchSrc);
                full.push_back("/Fo:" + pchObj);
            }

            if (showOutput)
                hp::printlnCl(
                    std::format("{} PCH Command: {}", tag, hm::Process::join(full)), hp::YELLOW);

            if (c.dryRun) {
                pchBinaryPath = pchBinary;
            } else {
                auto result = hm::Process::run(hm::Process::join(full));

                if (result.exitCode != 0) {
                    hp::printlnCl(
                        std::format("{} PCH creation failed (exit code {}).", tag, result.exitCode),
                        hp::RED);
                    hp::printlnCl(result.output, hp::RED);
                    return 1;
                }
                pchBinaryPath = pchBinary;
            }
        } else {
            pchBinaryPath = pchBinary;
        }
    }

    if (!pchBinaryPath.empty() && !isClang) {
        std::string pchIncludeDir = std::filesystem::path(pchBinaryPath).parent_path().string();
        compileFlags = comp->includeFlag(pchIncludeDir) + " " + compileFlags;
    }

    if (c.seperate && comp->supportsSeparate()) {

        if (!c.dryRun)
            std::filesystem::create_directories(compilerDir + "/obj");
        if (isClang && !c.dryRun)
            std::filesystem::create_directories(compilerDir + "/modules");

        std::map<std::string, std::string> clangModules;
        if (isClang) {
            for (const auto &m : c.v_Modules) {
                if (!std::filesystem::exists(m))
                    continue;
                std::string moduleName = extractModule(m);
                if (moduleName.empty()) {
                    hp::printlnCl(std::format("{} Error: File '{}' is not a module", tag, m), hp::RED);
                    return 1;
                }
                std::string moduleBase = std::filesystem::path(m).stem().string();
                clangModules[moduleName] = compilerDir + "/modules/" + moduleBase + ".pcm";
            }
        }

        std::map<std::string, std::string> msvcModules;
        if (comp->isMSVC()) {
            for (const auto &m : c.v_Modules) {
                if (!std::filesystem::exists(m))
                    continue;
                std::string moduleName = extractModule(m);
                if (moduleName.empty()) {
                    hp::printlnCl(std::format("{} Error: File '{}' is not a module", tag, m), hp::RED);
                    return 1;
                }
                std::string moduleBase = std::filesystem::path(m).stem().string();
                msvcModules[moduleName] = compilerDir + "/modules/" + moduleBase + ".ifc";
            }
        }

        std::vector<std::string> expandedFiles = expandInputs(c);

        std::partition(expandedFiles.begin(), expandedFiles.end(), [&](const std::string &s) {
            return isModule(s) || isDeclaredModule(s, c.v_Modules);
        });

        if (showOutput) {
            hp::printlnCl(std::format("\n{} {} files to check:", tag, expandedFiles.size()), hp::YELLOW);
            for (const auto &file : expandedFiles) {
                std::string mtag = isModule(file) ? "[Module] " : "";
                hp::printlnCl(std::format("- {}{}", mtag, file), hp::YELLOW);
            }
            std::cout << '\n';
            if (c.rebuild)
                hp::printlnCl(std::format("{} Rebuilding forced, ignoring timestamps.", tag), hp::YELLOW);
        }

        std::vector<std::string> objects;
        std::vector<std::string> upToDate;
        std::vector<Job> jobs;

        for (const auto &file : expandedFiles) {
            std::filesystem::path srcPath(file);
            std::filesystem::path relPath = srcPath.is_absolute() ? srcPath.filename() : srcPath;
            std::string objPath = compilerDir + "/obj/" + relPath.generic_string() + comp->objectExt();
            if (!c.dryRun)
                std::filesystem::create_directories(std::filesystem::path(objPath).parent_path());

            std::vector<std::string> args;
            args.push_back(comp->binary());

            for (auto &s : comp->subcommand())
                args.push_back(s);

            std::istringstream iss(compileFlags);
            std::string tok;
            while (iss >> tok)
                args.push_back(tok);

            bool isModFile = isModule(file) || isDeclaredModule(file, c.v_Modules);
            std::string thisModuleName;
            if (isModFile)
                thisModuleName = extractModule(file);
            if (isModFile) {
                if (isClang) {
                    args.push_back("-xc++-module");
                    std::string moduleBase = std::filesystem::path(file).stem().string();
                    std::string pcmPath = compilerDir + "/modules/" + moduleBase + ".pcm";
                    args.push_back("-fmodule-output=" + pcmPath);
                }

                if (comp->isMSVC()) {
                    args.push_back("/interface");
                    args.push_back("/TP");

                    std::string moduleBase = std::filesystem::path(file).stem().string();
                    std::string ifcPath = compilerDir + "/modules/" + moduleBase + ".ifc";

                    if (ifcPath.find(' ') != std::string::npos) {
                        hp::printlnCl(std::format("{} Error: MSVC module paths cannot contain spaces ('{}'). "
                                                  "Move the project to a path without spaces, or use a different compiler.",
                                                  tag, ifcPath),
                                      hp::RED);
                        return 1;
                    }

                    std::filesystem::create_directories(compilerDir + "/modules");
                    args.push_back("/ifcOutput" + ifcPath);
                }
            }

            for (auto &a : comp->compileArgs(file))
                args.push_back(a);

            for (auto &a : comp->objectOutputArgs(objPath))
                args.push_back(a);

            for (auto &a : comp->depArgs())
                args.push_back(a);

            if (!pchBinaryPath.empty()) {
                for (auto &a : comp->pchUseArgs(c.pch, pchBinaryPath))
                    args.push_back(a);
            }

            if (isClang && !clangModules.empty()) {
                for (const auto &[modName, modPath] : clangModules) {
                    if (modName == thisModuleName)
                        continue;
                    args.push_back("-fmodule-file=" + modName + "=" + modPath);
                }
            }

            if (comp->isMSVC() && !msvcModules.empty()) {
                for (const auto &[modName, modPath] : msvcModules) {
                    if (modName == thisModuleName)
                        continue;

                    if (modPath.find(' ') != std::string::npos) {
                        hp::printlnCl(std::format("{} Error: MSVC module reference path cannot contain spaces ('{}').", tag, modPath), hp::RED);
                        return 1;
                    }

                    args.push_back("/reference");
                    args.push_back(modName + "=" + modPath);
                }
            }
            std::string objCmd = hm::Process::join(args);

            bool needsBuild = needsRebuild(file, objPath, c.rebuild);
            if (!needsBuild && isModFile) {
                std::string moduleBase = std::filesystem::path(file).stem().string();
                if (isClang) {
                    std::string pcmPath = compilerDir + "/modules/" + moduleBase + ".pcm";
                    if (!std::filesystem::exists(pcmPath))
                        needsBuild = true;
                } else if (comp->isMSVC()) {
                    std::string ifcPath = compilerDir + "/modules/" + moduleBase + ".ifc";
                    if (!std::filesystem::exists(ifcPath))
                        needsBuild = true;
                }
            }

            if (!needsBuild) {
                if (!c.quiet && c.verbose)
                    upToDate.push_back(std::format("{} Skip (up to date): {}", tag, file));
                objects.push_back(objPath);
                continue;
            }

            if (showOutput)
                hp::printlnCl(std::format("\n{} Object Command: {}", tag, objCmd), hp::YELLOW);

            std::string safeStem = relPath.generic_string();
            std::replace(safeStem.begin(), safeStem.end(), '/', '_');
            std::replace(safeStem.begin(), safeStem.end(), '\\', '_');
            std::string perFileLog = std::format("{}/{}_{}.raw", r_logDir, baseName, safeStem);
            jobs.push_back({file, objCmd, perFileLog});
            objects.push_back(objPath);
        }

        if (exitCode == 0) {
            if (!c.quiet && c.verbose && !upToDate.empty()) {
                std::cout << '\n';
                for (const auto &up : upToDate)
                    hp::printlnCl(up, hp::YELLOW);
            }

            std::vector<Job> moduleJobs, regularJobs;
            for (auto &j : jobs) {
                if (isModule(j.file) || isDeclaredModule(j.file, c.v_Modules))
                    moduleJobs.push_back(j);
                else
                    regularJobs.push_back(j);
            }

            int rc = 0;
            if (!c.dryRun) {
                for (auto &j : moduleJobs) {
                    std::vector<Job> single = {j};
                    if (runJobs(single, 1) != 0) {
                        rc = 1;
                        break;
                    }
                }

                if (rc == 0 && !regularJobs.empty())
                    rc = runJobs(regularJobs, static_cast<std::size_t>(c.jobs));
            }

            if (rc != 0)
                exitCode = rc;
        }

        std::vector<std::string> linkObjects;
        if (!pchBinaryPath.empty() && comp->pchNeedsSourceFile()) {
            std::string pchObj = std::filesystem::path(pchBinaryPath).parent_path().string() + "/pch.obj";
            if (std::filesystem::exists(pchObj)) {
                linkObjects.push_back(pchObj);
            }
        }

        if (exitCode == 0) {
            std::vector<std::string> args;
            std::string binary;

            if (c.kind == "static") {
                auto parent = std::filesystem::path(c.output).parent_path();
                if (!c.dryRun && !parent.empty())
                    std::filesystem::create_directories(parent);
                if (!c.dryRun)
                    std::filesystem::remove(c.output);

                binary = comp->archiveBinary();
                args.push_back("rcs");
                args.push_back(c.output);
                for (const auto &obj : objects)
                    args.push_back(obj);
            } else {
                binary = comp->binary();

                for (auto &s : comp->subcommand()) {
                    binary += " ";
                    binary += s;
                }

                std::istringstream iss(compileFlags);
                std::string tok;
                while (iss >> tok)
                    args.push_back(tok);

                for (const auto &obj : objects)
                    args.push_back(obj);

                std::istringstream iss2(linkFlags);
                while (iss2 >> tok)
                    args.push_back(tok);

                for (auto &a : comp->executableOutputArgs(c.output))
                    args.push_back(a);

                for (const auto &obj : linkObjects)
                    args.push_back(obj);
            }

            std::string linkCmd = binary + " " + hm::Process::join(args);

            if (linkCmd.size() > 30000) {
                std::string rspPath = std::format("{}/{}.rsp", compilerDir, baseName);
                std::string rspArg;
                if (!c.dryRun)
                    rspArg = hm::Process::writeResponseFile(args, rspPath);
                if (!rspArg.empty()) {
                    linkCmd = binary + " " + rspArg;
                    if (showOutput)
                        hp::printlnCl(std::format("\n{} Link command too long ({} chars); using response file: {}", tag, hm::Process::join(args).size(), rspPath), hp::YELLOW);
                }
            }

            if (showOutput)
                hp::printlnCl(std::format("\n{} Link Command: {}", tag, linkCmd), hp::YELLOW);

            if (!c.dryRun) {
                auto linkResult = hm::Process::run(linkCmd);
                exitCode = linkResult.exitCode;

                {
                    std::ofstream linkLog(r_logPath);
                    linkLog << linkResult.output;
                }
            }

            if (c.dryRun && c.run)
                hp::printlnCl(std::format("{} Would run: {}", tag, c.output), hp::CYAN);
            if (!c.dryRun && exitCode == 0 && c.run) {
                if (showOutput)
                    hp::printlnCl(std::format("{} Running: {}", tag, c.output), hp::CYAN);

                auto runResult = hm::Process::run("\"" + c.output + "\"");
                exitCode = runResult.exitCode;

                if (!runResult.output.empty()) {
                    std::cout << runResult.output;
                    if (runResult.output.back() != '\n' && c.verbose)
                        std::cout << '\n';
                }

                if (exitCode != 0)
                    hp::printlnCl(std::format("{} Exit code: {}", tag, exitCode), hp::YELLOW);
            }
        }

    } else {
        if (c.kind == "static") {
            hp::printlnCl("Error: 'Kind: static' requires separate compilation. Remove --no-sep.", hp::RED);
            return 1;
        }

        std::vector<std::string> args;
        args.push_back(comp->binary());

        for (auto &s : comp->subcommand())
            args.push_back(s);

        if (comp->isZig()) {
            args.push_back("build-exe");
        }

        std::istringstream iss(compileFlags);
        std::string tok;
        while (iss >> tok)
            args.push_back(tok);

        std::istringstream iss2(linkFlags);
        while (iss2 >> tok)
            args.push_back(tok);

        std::vector<std::string> inputs = splitArgs(c.inputFile);
        inputs = expandInputs(c);
        for (auto &a : comp->directBuildArgs(inputs, c.output))
            args.push_back(a);

        std::string command = hm::Process::join(args);

        if (command.size() > 30000) {
            std::string rspPath = std::format("{}/{}.rsp", compilerDir, baseName);
            std::string rspArg;
            if (!c.dryRun)
                rspArg = hm::Process::writeResponseFile(args, rspPath);
            if (!rspArg.empty()) {
                command = comp->binary();
                for (auto &s : comp->subcommand()) {
                    command += " ";
                    command += s;
                }
                command += " " + rspArg;
                if (showOutput)
                    hp::printlnCl(std::format("\n{} Command too long; using response file: {}", tag, rspPath), hp::YELLOW);
            }
        }

        if (showOutput)
            hp::printlnCl(std::format("\n{} Real Command: {}", tag, command), hp::YELLOW);

        if (!c.dryRun) {
            auto buildResult = hm::Process::run(command);
            exitCode = buildResult.exitCode;

            {
                std::ofstream buildLog(r_logPath);
                buildLog << buildResult.output;
            }
        }

        if (c.dryRun && c.run)
            hp::printlnCl(std::format("[HelpMake] Would run: {}", c.output), hp::CYAN);
        if (!c.dryRun && exitCode == 0 && c.run) {
            if (showOutput)
                hp::printlnCl(std::format("{} Running: {}", tag, c.output), hp::CYAN);

            auto runResult = hm::Process::run("\"" + c.output + "\"");
            exitCode = runResult.exitCode;

            if (!runResult.output.empty()) {
                std::cout << runResult.output;
                if (runResult.output.back() != '\n' && c.verbose)
                    std::cout << '\n';
            }

            if (exitCode != 0)
                hp::printlnCl(std::format("{} Exit code: {}", tag, exitCode), hp::YELLOW);
        }
    }
    double elapsed = hp::stopTimer(timer);

    if (c.dryRun) {
        auto info = c.v_Info;
        for (const auto &copy : c.v_Copy)
            info[std::format("{} Would perform copy: {}", tag, copy)] = hp::CYAN;
        if (!c.Postcmd.empty())
            info[std::format("{} Would run post-build command: {}", tag, c.Postcmd)] = hp::CYAN;
        hp::printlnCl(std::format("{} Dry run complete. No build commands were executed.", tag), hp::GREEN);
        if (c.time)
            info[std::format("{} Build Plan Time: {}s\n", tag, elapsed)] = hp::CYAN;

        if (!info.empty() && !c.quiet) {
            hp::printlnCl("\nInfo:", hp::CYAN);

            std::vector<const std::pair<const std::string, hp::Color> *> errors;
            std::vector<const std::pair<const std::string, hp::Color> *> warnings;
            std::vector<const std::pair<const std::string, hp::Color> *> successes;
            std::vector<const std::pair<const std::string, hp::Color> *> infos;
            std::vector<const std::pair<const std::string, hp::Color> *> debugs;

            for (const auto &el : info) {
                if (el.second == hp::RED)
                    errors.push_back(&el);
                else if (el.first.find("[Debug]") != std::string::npos && c.debug)
                    debugs.push_back(&el);
                else if (el.second == hp::YELLOW)
                    warnings.push_back(&el);
                else if (el.second == hp::GREEN && c.verbose)
                    successes.push_back(&el);
                else if (el.second == hp::CYAN)
                    infos.push_back(&el);

                else
                    infos.push_back(&el);
            }

            auto dump = [](auto &v, const std::string &sym) {
                if (v.empty())
                    return;
                std::cout << '\n';

                std::string prefix = "  " + sym + "  ";
                std::string indent = "     ";

                for (const auto *el : v) {
                    std::string text = el->first;
                    while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
                        text.pop_back();

                    std::string out;
                    std::size_t start = 0;
                    bool first = true;
                    while (true) {
                        std::size_t nl = text.find('\n', start);
                        std::string segment = text.substr(start, nl == std::string::npos ? nl : nl - start);
                        if (!first)
                            while (!segment.empty() && segment.front() == ' ')
                                segment.erase(segment.begin());

                        out += (first ? prefix : indent) + segment;
                        if (nl == std::string::npos)
                            break;

                        out += '\n';
                        start = nl + 1;
                        first = false;
                    }
                    hp::printlnCl(out, el->second);
                }
            };

            auto sortNL = [](auto &v) {
                std::stable_sort(v.begin(), v.end(), [](const auto *a, const auto *b) {
                    bool aNL = a->first.find('\n') != std::string::npos;
                    bool bNL = b->first.find('\n') != std::string::npos;
                    return aNL < bNL;
                });
            };

            sortNL(errors);
            sortNL(warnings);
            sortNL(successes);
            sortNL(infos);
            sortNL(debugs);

            if (!errors.empty())
                dump(errors, "✘");
            if (!warnings.empty())
                dump(warnings, "⚠");
            if (c.verbose) {
                dump(successes, "✔");
                dump(infos, "·");
            }
            if (c.debug)
                dump(debugs, "·");

            std::cout << '\n';
        }

        return 0;
    }

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

    if (!c.v_Info.empty() && !c.quiet) {
        auto info = c.v_Info;
        hp::printlnCl("\nInfo:", hp::CYAN);

        std::vector<const std::pair<const std::string, hp::Color> *> errors;
        std::vector<const std::pair<const std::string, hp::Color> *> warnings;
        std::vector<const std::pair<const std::string, hp::Color> *> successes;
        std::vector<const std::pair<const std::string, hp::Color> *> infos;
        std::vector<const std::pair<const std::string, hp::Color> *> debugs;

        for (const auto &el : info) {
            if (el.second == hp::RED)
                errors.push_back(&el);
            else if (el.first.find("[Debug]") != std::string::npos && c.debug)
                debugs.push_back(&el);
            else if (el.second == hp::YELLOW)
                warnings.push_back(&el);
            else if (el.second == hp::GREEN && c.verbose)
                successes.push_back(&el);
            else if (el.second == hp::CYAN)
                infos.push_back(&el);

            else
                infos.push_back(&el);
        }

        auto dump = [](auto &v, const std::string &sym) {
            if (v.empty())
                return;
            std::cout << '\n';

            std::string prefix = "  " + sym + "  ";
            std::string indent = "     ";

            for (const auto *el : v) {
                std::string text = el->first;
                while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
                    text.pop_back();

                std::string out;
                std::size_t start = 0;
                bool first = true;
                while (true) {
                    std::size_t nl = text.find('\n', start);
                    std::string segment = text.substr(start, nl == std::string::npos ? nl : nl - start);
                    if (!first)
                        while (!segment.empty() && segment.front() == ' ')
                            segment.erase(segment.begin());

                    out += (first ? prefix : indent) + segment;
                    if (nl == std::string::npos)
                        break;

                    out += '\n';
                    start = nl + 1;
                    first = false;
                }
                hp::printlnCl(out, el->second);
            }
        };

        auto sortNL = [](auto &v) {
            std::stable_sort(v.begin(), v.end(), [](const auto *a, const auto *b) {
                bool aNL = a->first.find('\n') != std::string::npos;
                bool bNL = b->first.find('\n') != std::string::npos;
                return aNL < bNL;
            });
        };

        sortNL(errors);
        sortNL(warnings);
        sortNL(successes);
        sortNL(infos);
        sortNL(debugs);

        if (!errors.empty())
            dump(errors, "✘");
        if (!warnings.empty())
            dump(warnings, "⚠");
        if (c.verbose) {
            dump(successes, "✔");
            dump(infos, "·");
        }
        if (c.debug)
            dump(debugs, "·");

        std::cout << '\n';
    }

    if (exitCode == 0) {
        if (c.notify)
            Parser::Notify(elapsed);

        if (!c.v_Copy.empty()) {
            for (const auto &copy : c.v_Copy) {
                std::size_t arrow = copy.find("->");

                if (arrow == std::string::npos) {
                    hp::printlnCl(std::format("\n{} Warning invalid copy value. Missing '->'", tag), hp::YELLOW);
                    continue;
                }

                std::string src = trim(copy.substr(0, arrow));
                std::string dest = trim(copy.substr(arrow + 2));
                if (dest.empty()) {
                    hp::printlnCl(std::format("\n{} Warning invalid copy value. Missing value after '->'", tag), hp::YELLOW);
                    continue;
                }

                if (src.empty()) {
                    hp::printlnCl(std::format("\n{} Warning invalid copy value. Missing value before '->'", tag), hp::YELLOW);
                    continue;
                }

                try {
                    if (src.find('*') != std::string::npos) {

                        std::filesystem::path p(src);
                        std::string dir = p.parent_path().string();
                        if (dir.empty())
                            dir = ".";

                        std::string ext = p.extension().string();

                        for (const auto &entry : std::filesystem::directory_iterator(dir)) {
                            if (!entry.is_regular_file())
                                continue;
                            if (!ext.empty() && entry.path().extension() != ext)
                                continue;

                            std::filesystem::copy_file(entry.path(), std::filesystem::path(dest) / entry.path().filename(),
                                                       std::filesystem::copy_options::overwrite_existing);
                        }

                    } else if (std::filesystem::is_directory(src)) {
                        std::filesystem::create_directories(dest);
                        std::filesystem::copy(src, dest, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
                    } else if (std::filesystem::exists(src)) {
                        std::filesystem::create_directories(dest);
                        std::filesystem::copy_file(src, std::filesystem::path(dest) / std::filesystem::path(src).filename(),
                                                   std::filesystem::copy_options::overwrite_existing);
                    } else {
                        hp::printlnCl(std::format("{} Error: Copy source '{}' does not exist", tag, src), hp::RED);
                        hp::printlnCl(std::format("{} Build Failed.", tag), hp::RED);
                        return 1;
                    }
                } catch (const std::filesystem::filesystem_error &e) {
                    hp::printlnCl(std::format("{} Copy failed: {}", tag, e.what()), hp::RED);
                    hp::printlnCl(std::format("{} Build Failed.", tag), hp::RED);
                    return 1;
                }
            }
        }

        if (isConfig && showOutput)
            hp::printlnCl(std::format("\nConfig '{}' compiled successfully. Output: {}", name, c.output), hp::GREEN);
        else if (showOutput)
            hp::printlnCl(std::format("\nCompilation successful. Output file: {}", c.output), hp::GREEN);

        if (!result.empty() && showOutput)
            std::cout << result << "\n";

        if (!c.Postcmd.empty()) {
            if (showOutput)
                hp::printlnCl(std::format("{} Running post-build command: {}", tag, c.Postcmd), hp::CYAN);

            auto post = hm::Process::run(c.Postcmd);

            if (post.exitCode == -1 && !c.quiet)
                hp::printlnCl("Warning: Could not launch post-build command.", hp::YELLOW);
            else if (post.exitCode != 0 && !c.quiet)
                hp::printlnCl(std::format("Warning: Post-build command failed (exit code {}).", post.exitCode), hp::YELLOW);
            else if (showOutput)
                hp::printlnCl("Post-build command executed successfully.", hp::GREEN);
        }
    } else {
        if (c.notify)
            Parser::Notify(elapsed, false);
#ifdef _WIN32
        std::string whichCmd = std::format("where {} 2>nul", c.compiler == "zig" ? "zig" : comp->binary());
        std::string test = hp::command(whichCmd);
        bool missing = test.find("INFO:") != std::string::npos;
#else
        std::string whichCmd = std::format("which {} 2>/dev/null", c.compiler == "zig" ? "zig" : comp->binary());
        bool missing = hm::Process::run(whichCmd).exitCode != 0;
#endif

        if (missing) {
            hp::printlnCl(std::format("Error: {} compiler not found. Please install it and add it to PATH.", c.compiler), hp::RED);
        } else {
            if (!cleanLog.empty())
                std::cout << result << "\n";
            if (isConfig)
                hp::printlnCl(std::format("Config '{}' compilation failed.", name), hp::RED);
            else
                hp::printlnCl("\nCompilation Failed", hp::RED);
            if (!c.quiet)
                hp::printlnCl(std::format("See Logs: {}", logPath), hp::YELLOW);
        }
        return 1;
    }

    addTime(cfg.v_Times, "Execute", t0);
    return 0;
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
    if (local.v_Copy.empty())
        local.v_Copy = cfg.v_Copy;
    if (local.v_Depends.empty())
        local.v_Depends = cfg.v_Depends;

    local.verbose = local.verbose || cfg.verbose;
    local.debug = local.debug || cfg.debug;
    local.dryRun = local.dryRun || cfg.dryRun;
    local.quiet = local.quiet || cfg.quiet;
    local.notify = local.notify || cfg.notify;
    local.rebuild = local.rebuild || cfg.rebuild;
    local.json = local.json || cfg.json;
    local.time = local.time || cfg.time;
    local.seperate = local.seperate && cfg.seperate;

    return local;
}

int Parser::executeConfigs(const std::vector<std::string> &names) {
    int rc = 0;
    for (const auto &name : names) {
        auto it = configs.find(name);
        if (it == configs.end()) {
            if (cfg.debug)
                hp::printlnCl(std::format("[Debug] Config name is: '{}'", name), hp::YELLOW);
            hp::printlnCl(std::format("Error: Config '{}' was either not found or is invalid.", name), hp::RED);
            hp::printlnCl("Available configs:", hp::YELLOW);
            for (const auto &[n, _] : configs)
                std::cout << std::format("- {}\n", n);
            continue;
        }

        Config local = inheritConfig(it->second);
        std::string tag = std::format("[HelpMake] Config '{}':", name);

        if (resolveDependencies(local, tag) != 0) {
            rc = 1;
            continue;
        }

        if (executeConfig(local, name) != 0)
            rc = 1;
    }
    return rc;
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

    hm::Process::run(notifyCmd);
}

int Parser::autoCompiler(Config &c, const std::string &tag) {
    if (!c.compiler.empty())
        return 0;

    struct Candidate {
        std::string base;
        std::string subcommand;
    };

    struct CompilerGroup {
        std::vector<std::string> extensions;
        std::vector<Candidate> candidates;
        std::string languageName;
    };

    static const std::vector<CompilerGroup> compilerGroups = {
        {{".c"}, {{"gcc", ""}, {"clang", ""}, {"zig", "cc"}}, "C"},
        {{".cpp", ".cxx", ".cc"}, {{"g++", ""}, {"clang++", ""}, {"cl", ""}, {"zig", "c++"}}, "C++"},
        {{".zig"}, {{"zig", ""}}, "Zig"},
        {{".go"}, {{"go", ""}}, "Go"},
        {{".rs"}, {{"rustc", ""}}, "Rust"},
        {{".py"}, {{"python3", ""}, {"python", ""}}, "Python"},
        {{".java"}, {{"javac", ""}}, "Java"},
        {{".cs"}, {{"csc", ""}, {"mcs", ""}}, "C#"},
        {{".ts"}, {{"tsc", ""}}, "TypeScript"},
        {{".js"}, {{"node", ""}}, "JavaScript"},
        {{".rb"}, {{"ruby", ""}}, "Ruby"},
        {{".php"}, {{"php", ""}}, "PHP"},
        {{".pl"}, {{"perl", ""}}, "Perl"},
        {{".swift"}, {{"swiftc", ""}}, "Swift"},
        {{".kt", ".kts"}, {{"kotlinc", ""}}, "Kotlin"},
        {{".hs"}, {{"ghc", ""}}, "Haskell"},
        {{".lua"}, {{"lua", ""}}, "Lua"},
        {{".r"}, {{"Rscript", ""}}, "R"},
        {{".jl"}, {{"julia", ""}}, "Julia"},
        {{".dart"}, {{"dart", ""}}, "Dart"},
        {{".scala"}, {{"scalac", ""}}, "Scala"},
        {{".clj"}, {{"clojure", ""}}, "Clojure"},
        {{".erl"}, {{"erlc", ""}}, "Erlang"},
        {{".ex", ".exs"}, {{"elixir", ""}}, "Elixir"},
        {{".fs", ".fsi", ".fsx"}, {{"fsharpc", ""}, {"dotnet", "fsi"}, {"dotnet", "fsc"}}, "F#"},
        {{".ml", ".mli"}, {{"ocamlc", ""}, {"ocamlopt", ""}}, "OCaml"},
        {{".vbs"}, {{"cscript", ""}}, "VBScript"},
        {{".bat", ".cmd"}, {{"cmd.exe", ""}}, "Batch Script"},
        {{".ps1"}, {{"powershell.exe", ""}, {"pwsh.exe", ""}}, "PowerShell"},
    };

    std::string firstExt;
    bool anyInput = false;

    if (c.v_inputFiles.empty()) {
        int rc = collectFiles(c, ".");
        if (rc != 0) {
            hp::printlnCl(std::format("{} Error: No input files found for auto-detecting compiler.", tag), hp::RED);
            return rc;
        }
    }

    for (const auto &file : c.v_inputFiles) {
        std::string ext = std::filesystem::path(file).extension().string();
        if (ext.empty())
            continue;

        if (!anyInput) {
            firstExt = ext;
            anyInput = true;
        }

        for (const auto &group : compilerGroups) {
            bool extMatches = false;
            for (const auto &e : group.extensions) {
                if (e == ext) {
                    extMatches = true;
                    break;
                }
            }
            if (!extMatches)
                continue;

            for (const auto &cand : group.candidates) {
                if (hm::Process::exists(cand.base)) {
                    c.compiler = cand.subcommand.empty() ? cand.base : (cand.base + " " + cand.subcommand);
                    c.v_Info[std::format("{} Warning: Compiler not specified. Auto-detected compiler: {}", tag, c.compiler)] = hp::YELLOW;
                    return 0;
                }
            }

            c.v_Errors.push_back(std::format("{} Error: No compiler found for '{}' files. Install one of: {}", tag, group.languageName,
                                             hm::Process::join([&] {
                                                 std::vector<std::string> names;
                                                 for (const auto &cand : group.candidates)
                                                     names.push_back(cand.subcommand.empty() ? cand.base : cand.base + " " + cand.subcommand);
                                                 return names;
                                             }())));
            return 1;
        }
    }

    if (!anyInput) {
        c.v_Info[std::format("{} Error: No input files to detect a compiler from.\n Set 'Compiler:' explicitly or add 'InputFiles {{ }}'.", tag)] = hp::RED;
        return 1;
    }

    c.v_Info[std::format("{} Error: No compiler known for extension '{}'. Set 'Compiler:' explicitly.", tag, firstExt)] = hp::RED;
    c.v_Errors.push_back(std::format("{} Error: No compiler known for extension '{}'. Set 'Compiler:' explicitly.", tag, firstExt));
    return 1;
}

int Parser::autoVersion(Config &c, const std::string &tag) {
    if (!c.version.empty())
        return 0;

    if (c.compiler == "cl" || c.compiler == "msvc") {
        c.version = "/std:c++latest";
        return 0;
    }

    bool isGcc = c.compiler == "gcc" || c.compiler == "g++";
    bool isClang = c.compiler == "clang" || c.compiler == "clang++";
    bool isZig = c.compiler == "zig cc" || c.compiler == "zig c++";

    if (!isGcc && !isClang && !isZig)
        return 0;

    std::string raw = trim(hp::command(std::format("{} -dumpversion", c.compiler)));
    if (raw.empty())
        return 0;

    int major = 0;
    std::sscanf(raw.c_str(), "%d", &major);
    if (major <= 0)
        return 0;

    bool isC = (c.compiler == "gcc" || c.compiler == "clang" || c.compiler == "zig cc");

    struct StdEntry {
        std::string flag;
        int gcc;
        int clang;
    };

    static constexpr std::array<StdEntry, 5> cxxMap = {{
        {"std=c++14", 5, 3},
        {"std=c++17", 8, 7},
        {"std=c++20", 11, 12},
        {"std=c++23", 14, 17},
        {"std=c++26", 16, 22},
    }};

    static constexpr std::array<StdEntry, 4> cMap = {{
        {"std=c99", 5, 3},
        {"std=c11", 8, 6},
        {"std=c17", 14, 18},
        {"std=c23", 14, 18},
    }};

    auto pick = [&](const auto &table, const std::string &fallback) -> std::string {
        std::string chosen = fallback;
        for (const auto &entry : table) {
            int required = isGcc ? entry.gcc : entry.clang;
            if (major >= required)
                chosen = entry.flag;
        }
        return chosen;
    };

    c.version = isC ? pick(cMap, "std=c99") : pick(cxxMap, "std=c++14");
    c.v_Info[std::format("{} Warning: Version not specified. Auto-detected version flag: {}", tag, c.version)] = hp::YELLOW;
    return 0;
}

int Parser::autoInputFiles(Config &c, const std::string &tag) {
    if (!c.v_inputFiles.empty())
        return 0;

    struct SourceEntry {
        std::vector<std::string> compilers;
        std::vector<std::string> extensions;
    };

    static const std::vector<SourceEntry> sourceExts = {
        {{"gcc", "clang", "zig cc"}, {".c"}},
        {{"g++", "clang++", "cl", "zig c++"}, {".cpp", ".cxx", ".cc"}},
        {{"zig"}, {".zig"}},
        {{"go"}, {".go"}},
        {{"rustc"}, {".rs"}},
        {{"python3", "python"}, {".py"}},
    };

    static constexpr std::array<std::string_view, 4> srcDirs{
        "src",
        "source",
        "code",
        "app",
    };

    static constexpr std::array<std::string_view, 32> excludedDirs = {
        ".git",
        ".hg",
        ".svn",
        ".vs",
        ".cache",
        ".idea",
        ".gitlab",
        ".github",
        "build",
        "bin",
        "obj",
        "out",
        "dist",
        "target",
        "node_modules",
        "vendor",
        "third_party",
        "external",
        "deps",
        "dep",
        "lib",
        "libs",
        "include",
        "includes",
        "test",
        "tests",
        "examples",
        "demos",
        "playground",
        "scratch",
        "tmp",
        "temp",
    };

    auto isSource = [&](const std::filesystem::path &p) {
        const auto ext = p.extension().string();
        return std::any_of(sourceExts.begin(), sourceExts.end(), [&](const SourceEntry &entry) {
            const bool compilerMatches = std::find(entry.compilers.begin(), entry.compilers.end(), c.compiler) != entry.compilers.end();
            const bool extensionMatches = std::find(entry.extensions.begin(), entry.extensions.end(), ext) != entry.extensions.end();
            return compilerMatches && extensionMatches;
        });
    };

    auto isExcluded = [&](const std::filesystem::path &p) {
        for (const auto &component : p)
            for (const auto &skip : excludedDirs)
                if (component.string() == skip)
                    return true;
        return false;
    };

    auto addFile = [&](const std::filesystem::path &p) {
        if (isSource(p) && !isExcluded(p))
            c.v_inputFiles.push_back(p.generic_string());
    };

    if (std::any_of(srcDirs.begin(), srcDirs.end(), [](std::string_view dir) -> bool {
            return !dir.empty() && std::filesystem::exists(dir) && std::filesystem::is_directory(dir);
        })) {

        std::error_code ec;
        for (const auto dirSrc : srcDirs) {
            if (dirSrc.empty())
                continue;
            for (auto it = std::filesystem::recursive_directory_iterator(dirSrc, std::filesystem::directory_options::skip_permission_denied, ec);
                 it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
                if (ec)
                    break;
                if (it->is_directory() && isExcluded(it->path())) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (it->is_regular_file())
                    addFile(it->path());
            }
        }

        for (const auto &entry : std::filesystem::directory_iterator(".", ec)) {
            if (ec)
                break;
            if (entry.is_regular_file())
                addFile(entry.path());
        }
    } else {
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(".", std::filesystem::directory_options::skip_permission_denied, ec);
             it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            if (ec)
                break;
            if (it->is_directory() && isExcluded(it->path())) {
                it.disable_recursion_pending();
                continue;
            }
            if (it->is_regular_file())
                addFile(it->path());
        }
    }

    if (!c.v_inputFiles.empty()) {
        std::vector<std::string> stems;
        for (const auto &f : c.v_inputFiles) {
            std::ifstream in(f);
            if (!in)
                continue;
            std::string line;
            while (std::getline(in, line)) {
                std::size_t pos = line.find("#include");
                if (pos == std::string::npos)
                    continue;

                std::size_t angleOpen = line.find('<', pos);
                std::size_t quoteOpen = line.find('"', pos);

                if (quoteOpen == std::string::npos)
                    continue;
                if (angleOpen != std::string::npos && angleOpen < quoteOpen)
                    continue;

                std::size_t q2 = line.find('"', quoteOpen + 1);
                if (q2 == std::string::npos)
                    continue;
                stems.push_back(std::filesystem::path(line.substr(quoteOpen + 1, q2 - quoteOpen - 1)).stem().string());
            }
        }
        hp::remove::duplicates(stems);

        if (!stems.empty()) {
            std::vector<std::string> original = c.v_inputFiles;
            std::error_code ec;
            for (auto it = std::filesystem::recursive_directory_iterator(".", std::filesystem::directory_options::skip_permission_denied, ec);
                 it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
                if (ec)
                    break;
                if (it->is_directory() && isExcluded(it->path())) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (!it->is_regular_file())
                    continue;
                if (!isSource(it->path()))
                    continue;
                if (std::ranges::find(stems, it->path().stem().string()) == stems.end())
                    continue;
                c.v_inputFiles.push_back(it->path().lexically_normal().generic_string());
            }
            hp::remove::duplicates(c.v_inputFiles);

            std::vector<std::string> added;
            for (const auto &f : c.v_inputFiles)
                if (std::ranges::find(original, f) == original.end())
                    added.push_back(f);

            if (!added.empty() && !c.quiet) {
                std::string msg = std::format("{} Warning: Auto-added companion sources:\n", tag);
                for (const auto &a : added)
                    msg += a;
                c.v_Info[msg] = hp::YELLOW;
            }
        }
    }

    if (c.v_inputFiles.empty()) {
        cfg.v_Info[std::format("{} Error: No input files found. Add an 'InputFiles {{ }}' block or place sources in 'src/'.", tag)] = hp::RED;
        c.v_Errors.push_back(std::format("{} Error: No input files found. Add an 'InputFiles {{ }}' block or place sources in 'src/'.", tag));
        return 1;
    }

    c.v_Info[std::format("{} Warning: Input Files not specified. Auto-detected Input Files.", tag)] = hp::YELLOW;

    return 0;
}

int Parser::collectFiles(Config &c, const std::string &folderPath) {
    if (!c.v_inputFiles.empty())
        return 0;

    std::filesystem::path root(folderPath);
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root))
        return 1;

    static constexpr std::string_view sourceExts[] = {
        ".c", ".cpp", ".cxx", ".cc", ".zig", ".go", ".rs", ".py", ".java", ".cs", ".ts", ".js",
        ".rb", ".php", ".pl", ".swift", ".kt", ".kts", ".hs", ".lua", ".r", ".jl", ".dart", ".scala",
        ".clj", ".erl", ".ex", ".exs", ".fs", ".fsi", ".fsx", ".ml", ".mli", ".vbs", ".bat", ".cmd", ".ps1"};

    static constexpr std::array<std::string_view, 32> excludedDirs = {
        ".git",
        ".hg",
        ".svn",
        ".vs",
        ".cache",
        ".idea",
        ".gitlab",
        ".github",
        "build",
        "bin",
        "obj",
        "out",
        "dist",
        "target",
        "node_modules",
        "vendor",
        "third_party",
        "external",
        "deps",
        "dep",
        "lib",
        "libs",
        "include",
        "includes",
        "test",
        "tests",
        "examples",
        "demos",
        "playground",
        "scratch",
        "tmp",
        "temp",
    };

    auto isExcludedDir = [&](const std::filesystem::path &p) {
        for (const auto &component : p) {
            auto name = to_lower(component.string());
            for (const auto &skip : excludedDirs)
                if (name == skip)
                    return true;
        }
        return false;
    };

    std::map<std::string, std::vector<std::string>> extGroupedFiles;

    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator(
             root, std::filesystem::directory_options::skip_permission_denied, ec);
         it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (ec)
            break;

        if (it->is_directory() && isExcludedDir(it->path())) {
            it.disable_recursion_pending();
            continue;
        }

        if (!it->is_regular_file())
            continue;

        std::string ext = to_lower(it->path().extension().string());
        if (std::ranges::find(sourceExts, ext) == std::end(sourceExts))
            continue;

        extGroupedFiles[ext].push_back(it->path().lexically_relative(root).generic_string());
    }

    if (extGroupedFiles.empty())
        return 1;

    auto maxExtIt = std::max_element(
        extGroupedFiles.begin(),
        extGroupedFiles.end(),
        [](const auto &a, const auto &b) {
            return a.second.size() < b.second.size();
        });

    size_t dominantCount = maxExtIt->second.size();
    const std::string dominantExt = maxExtIt->first;

    for (const auto &[ext, files] : extGroupedFiles) {
        if (ext == dominantExt || files.empty())
            continue;

        if ((dominantExt == ".c" || dominantExt == ".cpp" || dominantExt == ".cxx" || dominantExt == ".cc") &&
            (ext == ".c" || ext == ".cpp" || ext == ".cxx" || ext == ".cc"))
            continue;

        c.v_Info[std::format("[HelpMake] Warning: Found {} '{}' file{} alongside {} '{}' file{}. Compiling only '{}'. Use 'InputFiles {{ }}' to include specific files.",
                             files.size(), ext, files.size() == 1 ? "" : "s", dominantCount, dominantExt, dominantCount == 1 ? "" : "s",
                             dominantExt)] = hp::YELLOW;
    }

    c.v_inputFiles.insert(c.v_inputFiles.end(), maxExtIt->second.begin(), maxExtIt->second.end());
    return 0;
}

int Parser::autoIncludes(Config &c, const std::string &tag) {
    if (!c.includeFiles.empty())
        return 0;

    static constexpr std::array<std::string_view, 6> potentialIncludeDirs = {
        "include",
        "includes",
        "inc",
        "headers",
        "api",
        "public",
    };

    static constexpr std::array<std::string_view, 22> excludedDirs = {
        ".git",
        ".hg",
        ".svn",
        ".vs",
        ".cache",
        ".idea",
        ".gitlab",
        ".github",
        "build",
        "bin",
        "obj",
        "out",
        "dist",
        "target",
        "node_modules",
        "vendor",
        "third_party",
        "external",
        "deps",
        "dep",
        "test",
        "tests",

    };

    static constexpr std::array<std::string_view, 9> headerExts = {
        ".h",
        ".hpp",
        ".hxx",
        ".hh",
        ".inl",
        ".inc",
        ".ixx",
        ".cppm",
        ".tpp",
    };

    auto isExcludedDir = [&](const std::filesystem::path &p) {
        auto name = to_lower(p.filename().string());
        for (const auto &skip : excludedDirs)
            if (name == skip)
                return true;
        return false;
    };

    auto isNamedIncludeDir = [&](const std::filesystem::path &p) {
        auto name = to_lower(p.filename().string());
        for (const auto &inc : potentialIncludeDirs)
            if (name == inc)
                return true;
        return false;
    };

    auto hasHeaderExt = [&](const std::filesystem::path &p) {
        auto ext = to_lower(p.extension().string());
        for (const auto &e : headerExts)
            if (ext == e)
                return true;
        return false;
    };

    std::vector<std::string> headerDirs;

    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator(".", std::filesystem::directory_options::skip_permission_denied, ec); it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (ec)
            break;

        const auto &path = it->path();

        if (it->is_directory() && isExcludedDir(path)) {
            it.disable_recursion_pending();
            continue;
        }

        if (it->is_directory() && isNamedIncludeDir(path)) {
            c.includeFiles.push_back(path.lexically_relative(".").generic_string());
            it.disable_recursion_pending();
            continue;
        }

        if (it->is_regular_file() && hasHeaderExt(path))
            headerDirs.push_back(path.parent_path().lexically_relative(".").generic_string());
    }

    if (c.includeFiles.empty())
        c.includeFiles = headerDirs;

    c.includeFiles = hp::remove::duplicates(c.includeFiles);

    if (c.includeFiles.empty())
        return 0;

    auto comp = hm::makeCompiler(c.compiler);
    for (const auto &dir : c.includeFiles) {
        if (!c.flags.empty())
            c.flags += ' ';
        c.flags += comp->includeFlag(dir);
    }

    std::string info = std::format("{} Warning: Include dirs not specified. Auto-detected include dirs:\n", tag);
    for (const auto &dir : c.includeFiles)
        info += dir;

    c.v_Info[info] = hp::YELLOW;

    return 0;
}

int Parser::autoLinkFlags(Config &c, const std::string &tag) {
    auto comp = hm::makeCompiler(c.compiler);
    if (!comp)
        return 0;

    bool useMsvc = comp->isMSVC();

    struct LinkEntry {
        std::string_view header;
        std::string_view gccFlag;
        std::string_view msvcFlag;
    };

    static constexpr std::array<LinkEntry, 11> linkTable = {{
        {"pthread.h", "-pthread", ""},
        {"thread", "-pthread", ""},
        {"mutex", "-pthread", ""},
        {"atomic", "-pthread", ""},
        {"condition_variable", "-pthread", ""},
        {"cmath", "-lm", ""},
        {"math.h", "-lm", ""},
        {"dlfcn.h", "-ldl", ""},
        {"zlib.h", "-lz", "zlib.lib"},
        {"curl/curl.h", "-lcurl", "libcurl.lib"},
        {"openssl/ssl.h", "-lssl -lcrypto", "libssl.lib libcrypto.lib"},
    }};

    if (c.v_inputFiles.empty())
        return 0;

    std::vector<std::string> newFlags;
    for (const auto &f : c.v_inputFiles) {
        std::ifstream in(f);
        if (!in)
            continue;
        std::string line;
        while (std::getline(in, line)) {
            std::size_t pos = line.find("#include");
            if (pos == std::string::npos)
                continue;

            std::size_t angleOpen = line.find('<', pos);
            std::size_t quoteOpen = line.find('"', pos);

            if (angleOpen == std::string::npos)
                continue;
            if (quoteOpen != std::string::npos && quoteOpen < angleOpen)
                continue;

            std::size_t angleClose = line.find('>', angleOpen + 1);
            if (angleClose == std::string::npos)
                continue;

            std::string value = line.substr(angleOpen + 1, angleClose - angleOpen - 1);
            for (const auto &e : linkTable) {
                if (e.header != value)
                    continue;
                std::string_view flag = useMsvc ? e.msvcFlag : e.gccFlag;
                if (flag.empty())
                    break;
                newFlags.push_back(std::string(flag));
                break;
            }
        }
    }

    hp::remove::duplicates(newFlags);

    std::vector<std::string> actuallyAdded;
    for (const auto &f : newFlags) {
        if (c.flags.find(f) != std::string::npos)
            continue;
        if (!c.flags.empty())
            c.flags += ' ';
        c.flags += f;
        c.v_Flags.push_back(f);
        actuallyAdded.push_back(f);
    }

    if (!actuallyAdded.empty() && !c.quiet) {
        std::string msg = std::format("{} Warning: Auto-added link flags:\n", tag);
        for (const auto &f : actuallyAdded)
            msg += f;
        c.v_Info[msg] = hp::YELLOW;
    }
    return 0;
}

int Parser::autoKind(Config &c, const std::string &tag) {
    if (c.kind == "static")
        return 0;
    if (c.v_inputFiles.empty())
        collectFiles(c, tag);

    static constexpr std::array<std::string_view, 10> extMain{
        ".c",
        ".cpp",
        ".cxx",
        ".cc",
        ".zig",
        ".go",
        ".rs",
    };

    auto hasMain = [](std::string_view line) -> bool {
        std::size_t pos = 0;
        while ((pos = line.find("main", pos)) != std::string_view::npos) {
            bool wordStart = (pos == 0) || (!std::isalnum(static_cast<unsigned char>(line[pos - 1])) && line[pos - 1] != '_');
            bool wordEnd = (pos + 4 >= line.size()) || (!std::isalnum(static_cast<unsigned char>(line[pos + 4])) && line[pos + 4] != '_');
            if (wordStart && wordEnd) {
                std::size_t p = pos + 4;
                while (p < line.size() && std::isspace(static_cast<unsigned char>(line[p])))
                    ++p;
                if (p < line.size() && line[p] == '(')
                    return true;
            }
            pos += 4;
        }
        return false;
    };

    bool hasCFamily = false;
    bool foundMain = false;

    for (const auto &file : expandInputs(c)) {
        std::string ext = to_lower(std::filesystem::path(file).extension().string());
        if (std::ranges::find(extMain, ext) == extMain.end())
            continue;
        hasCFamily = true;

        std::ifstream src(file);
        if (!src)
            continue;
        std::string line;
        while (std::getline(src, line)) {
            if (line.find("//") != std::string::npos)
                continue;
            if (hasMain(line)) {
                foundMain = true;
                break;
            }
        }
        if (foundMain)
            break;
    }

    if (!hasCFamily) {
        c.kind = "executable";
        return 0;
    }

    c.kind = foundMain ? "executable" : "static";
    if (!foundMain)
        c.v_Info[std::format("{} Warning: No 'main' found. Using Kind: static", tag)] = hp::YELLOW;
    return 0;
}

int Parser::autoOutput(Config &c, const std::string &tag) {
    if (!c.output.empty())
        return 0;

#ifdef _WIN32
    const std::string ext = ".exe";
#else
    const std::string ext = "";
#endif

    auto set = [&](const std::string &name) -> bool {
        if (name.empty() || name == "." || name == "/" || name == "\\" || name.front() == '.')
            return false;
        c.output = name + ext;
        return true;
    };

    if (std::filesystem::exists("Cargo.toml")) {
        std::ifstream in("Cargo.toml");
        std::string line;
        bool inPackage = false;
        while (std::getline(in, line)) {
            std::string t = trim(line);
            if (t == "[package]") {
                inPackage = true;
                continue;
            }
            if (!t.empty() && t.front() == '[')
                inPackage = false;
            if (!inPackage)
                continue;
            if (!t.starts_with("name ") && !t.starts_with("name="))
                continue;
            std::size_t q1 = t.find('"');
            if (q1 == std::string::npos)
                continue;
            std::size_t q2 = t.find('"', q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(t.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("go.mod")) {
        std::ifstream in("go.mod");
        std::string line;
        while (std::getline(in, line)) {
            if (!line.starts_with("module "))
                continue;
            std::string mod = trim(line.substr(7));
            std::size_t slash = mod.find_last_of('/');
            if (set(slash == std::string::npos ? mod : mod.substr(slash + 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("package.json")) {
        std::ifstream in("package.json");
        std::string line;
        while (std::getline(in, line)) {
            std::size_t pos = line.find("\"name\"");
            if (pos == std::string::npos)
                continue;
            std::size_t colon = line.find(':', pos);
            if (colon == std::string::npos)
                continue;
            std::size_t q1 = line.find('"', colon);
            if (q1 == std::string::npos)
                continue;
            std::size_t q2 = line.find('"', q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(line.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("pyproject.toml")) {
        std::ifstream in("pyproject.toml");
        std::string line;
        bool inProject = false;
        while (std::getline(in, line)) {
            std::string t = trim(line);
            if (t == "[project]" || t == "[tool.poetry]") {
                inProject = true;
                continue;
            }
            if (!t.empty() && t.front() == '[')
                inProject = false;
            if (!inProject)
                continue;
            if (!t.starts_with("name ") && !t.starts_with("name="))
                continue;
            std::size_t q1 = t.find_first_of("\"'");
            if (q1 == std::string::npos)
                continue;
            char quote = t[q1];
            std::size_t q2 = t.find(quote, q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(t.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("setup.py")) {
        std::ifstream in("setup.py");
        std::string line;
        while (std::getline(in, line)) {
            std::size_t pos = line.find("name=");
            if (pos == std::string::npos)
                continue;
            std::size_t q1 = line.find_first_of("\"'", pos);
            if (q1 == std::string::npos)
                continue;
            char quote = line[q1];
            std::size_t q2 = line.find(quote, q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(line.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("build.zig")) {
        std::ifstream in("build.zig");
        std::string line;
        while (std::getline(in, line)) {
            if (line.find("addExecutable") == std::string::npos)
                continue;
            std::size_t pos = line.find(".name");
            if (pos == std::string::npos)
                continue;
            std::size_t q1 = line.find('"', pos);
            if (q1 == std::string::npos)
                continue;
            std::size_t q2 = line.find('"', q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(line.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("meson.build")) {
        std::ifstream in("meson.build");
        std::string line;
        while (std::getline(in, line)) {
            std::size_t pos = line.find("project(");
            if (pos == std::string::npos)
                continue;
            std::size_t q1 = line.find('\'', pos);
            if (q1 == std::string::npos)
                continue;
            std::size_t q2 = line.find('\'', q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(line.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("CMakeLists.txt")) {
        std::ifstream in("CMakeLists.txt");
        std::string line;
        while (std::getline(in, line)) {
            std::size_t start = line.find("project(");
            if (start == std::string::npos)
                continue;
            start += 8;
            std::size_t end = line.find_first_of(" )", start);
            if (end == std::string::npos)
                continue;
            if (set(line.substr(start, end - start)))
                goto done;
        }
    }

    if (std::filesystem::exists("pom.xml")) {
        std::ifstream in("pom.xml");
        std::string line;
        while (std::getline(in, line)) {
            std::size_t start = line.find("<artifactId>");
            if (start == std::string::npos)
                continue;
            start += 12;
            std::size_t end = line.find("</artifactId>", start);
            if (end == std::string::npos)
                continue;
            if (set(line.substr(start, end - start)))
                goto done;
        }
    }

    if (std::filesystem::exists("build.gradle") || std::filesystem::exists("build.gradle.kts")) {
        std::ifstream in(std::filesystem::exists("build.gradle") ? "build.gradle" : "build.gradle.kts");
        std::string line;
        while (std::getline(in, line)) {
            std::size_t pos = line.find("rootProject.name");
            if (pos == std::string::npos)
                continue;
            std::size_t q1 = line.find_first_of("\"'", pos);
            if (q1 == std::string::npos)
                continue;
            char quote = line[q1];
            std::size_t q2 = line.find(quote, q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(line.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    {
        std::error_code ec;
        for (const auto &e : std::filesystem::directory_iterator(".", ec)) {
            if (ec)
                break;
            if (!e.is_regular_file())
                continue;
            std::string ext2 = e.path().extension().string();
            if (ext2 != ".csproj" && ext2 != ".fsproj" && ext2 != ".vbproj")
                continue;
            if (set(e.path().stem().string()))
                goto done;
        }
    }

    for (const auto &f : expandInputs(c)) {
        if (std::filesystem::path(f).stem() != "main")
            continue;
        if (set("main"))
            goto done;
    }

    if (set(std::filesystem::current_path().filename().string()))
        goto done;

#ifdef _WIN32
    c.output = "a.exe";
#else
    c.output = "a.out";
#endif
    if (!c.quiet)
        c.v_Info[std::format("{} Warning: Output not specified. Using default {}", tag, c.output)] = hp::YELLOW;
    return 0;

done:
    c.output = trim(c.output);
    if (!c.quiet)
        c.v_Info[std::format("{} Warning: Output not specified. Auto-detected {}", tag, c.output)] = hp::YELLOW;
    return 0;
}