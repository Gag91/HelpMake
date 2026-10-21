#include "hm/parser/parser.hpp"
#include "hm/process/process.hpp"
#include "hm/toolchain/compiler.hpp"
#include "hp/colors/color.hpp"
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

        const auto &dep = it->second;
        std::string depRoot = "build/HelpMake/dep/" + dep.name;

        if (buildDependency(dep) != 0)
            return 1;

        for (const auto &inc : dep.includes) {
            std::string fullPath = depRoot + "/" + inc;
            c.includeFiles.push_back(fullPath);
            c.flags += " -I" + fullPath;
        }

        for (const auto &ld : dep.linkDirs)
            c.flags += " -L" + depRoot + "/" + ld;

        for (const auto &lf : dep.linkFlags)
            c.flags += " " + lf;

        for (const auto &ln : dep.links)
            c.flags += " " + depRoot + "/" + ln;

        if (cfg.verbose)
            hp::printlnCl(std::format(
                              "{} Linked dependency '{}' from '{}'",
                              tag, depName, depRoot),
                          hp::GREEN);
    }
    return 0;
}

int Parser::executeConfig(const Config &c, const std::string &name) {
    auto comp = hm::makeCompiler(c.compiler);
    if (!comp) {
        hp::printlnCl(std::format("Error: Unknown compiler '{}'.", c.compiler), hp::RED);
        return 1;
    }

    const std::string compilerDir = "build/HelpMake/" + comp->name();

    {
        std::size_t d = c.output.find_last_of('.');
        baseName = (d != std::string::npos) ? c.output.substr(0, d) : c.output;
    }

    {
        const std::filesystem::path marker = compilerDir + "/.buildid";
        std::string current = comp->binary();
        for (auto &s : comp->subcommand())
            current += " " + s;
        if (!c.version.empty())
            current += " " + c.version;
        if (!c.flags.empty())
            current += " " + c.flags;

        std::string previous;
        if (std::ifstream in(marker); in)
            std::getline(in, previous);

        if (previous != current) {
            if (!c.quiet && !previous.empty())
                hp::printlnCl("[HelpMake] Build flags changed, invalidating objects.", hp::YELLOW);
            std::error_code ec;
            std::filesystem::remove_all(compilerDir + "/obj", ec);
            std::filesystem::create_directories(compilerDir);
            std::ofstream(marker) << current;
        }
    }

    r_logDir = compilerDir + "/logs/raw";
    logDir = compilerDir + "/logs";
    r_logPath = r_logDir + "/" + baseName + ".txt";
    logPath = logDir + "/" + baseName + ".txt";

    bool isClang = (comp->name() == "clang");
    bool showOutput = c.verbose || c.debug;
    bool isConfig = !name.empty();

    std::string tag = isConfig ? std::format("[HelpMake] Config '{}':", name) : "[HelpMake]";

    if (c.json) {
        Parser::generateCompileCommands(c);
    }

    if (!c.Precmd.empty()) {
        if (showOutput)
            hp::printlnCl(std::format("{} Running pre-build command: {}", tag, c.Precmd), hp::CYAN);

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

    if (isConfig) {
        hp::printlnCl(std::format("\nBuilding config: {}", name), hp::CYAN);
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
        std::string pchBinary = "build/HelpMake/headers/" + comp->name() + "/" + std::filesystem::path(pchHeader).filename().string() + comp->pchExtension();
        std::string pchDir = "build/HelpMake/headers/" + comp->name();
        std::filesystem::create_directories(pchDir);
        if (PchneedsRebuild(pchHeader, pchBinary, c.rebuild)) {
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
                std::ofstream(pchSrc) << comp->pchSourceFileContent(pchHeader);
                full.push_back(pchSrc);
                full.push_back("/Fo:" + pchObj);
            }

            if (showOutput)
                hp::printlnCl(
                    std::format("{} PCH Command: {}", tag, hm::Process::join(full)), hp::YELLOW);

            auto result = hm::Process::run(hm::Process::join(full));

            if (result.exitCode != 0) {
                hp::printlnCl(
                    std::format("{} PCH creation failed (exit code {}).", tag, result.exitCode),
                    hp::RED);
                hp::printlnCl(result.output, hp::RED);
                return 1;
            }
            pchBinaryPath = pchBinary;
        } else {
            pchBinaryPath = pchBinary;
        }
    }

    if (!pchBinaryPath.empty() && !isClang) {
        std::string pchIncludeDir = std::filesystem::path(pchBinaryPath).parent_path().string();
        compileFlags = "-I" + pchIncludeDir + " " + compileFlags;
    }

    if (c.seperate && comp->supportsSeparate()) {

        std::filesystem::create_directories(compilerDir + "/obj");
        if (isClang)
            std::filesystem::create_directories(compilerDir + "/modules");

        std::map<std::string, std::string> clangModules;
        if (isClang) {
            for (const auto &m : c.v_Modules) {
                if (!std::filesystem::exists(m))
                    continue;
                std::string moduleName = extractModule(m);
                std::string moduleBase = std::filesystem::path(m).stem().string();
                clangModules[moduleName] = compilerDir + "/modules/" + moduleBase + ".pcm";
            }
            if (showOutput && !clangModules.empty()) {
                std::cout << std::format("{} Clang modules detected:\n", tag);
                for (const auto &[modName, modPath] : clangModules)
                    std::cout << std::format("  {} -> {}\n", modName, modPath);
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
            std::string baseName = std::filesystem::path(file).stem().string();

            std::string objPath = compilerDir + "/obj/" + baseName + comp->objectExt();

            std::vector<std::string> args;
            args.push_back(comp->binary());

            for (auto &s : comp->subcommand())
                args.push_back(s);

            std::istringstream iss(compileFlags);
            std::string tok;
            while (iss >> tok)
                args.push_back(tok);

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
                    args.push_back("-fmodule-file=" + modName + "=" + modPath);
                }
            }

            std::string objCmd = hm::Process::join(args);

            if (!needsRebuild(file, objPath, c.rebuild)) {
                upToDate.push_back(std::format("{} Skip (up to date): {}", tag, file));
                objects.push_back(objPath);
                continue;
            }

            if (showOutput)
                hp::printlnCl(std::format("\n{} Object Command: {}", tag, objCmd), hp::YELLOW);

            std::string perFileLog = std::format("{}/{}.raw", r_logDir, baseName);
            jobs.push_back({file, objCmd, perFileLog});
            objects.push_back(objPath);
        }

        if (exitCode == 0) {
            std::cout << '\n';
            for (const auto &up : upToDate)
                hp::printlnCl(up, hp::YELLOW);

            int rc = runJobs(jobs, c.jobs);
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
                if (!parent.empty())
                    std::filesystem::create_directories(parent);
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
            if (c.run)
                linkCmd += " && " + c.output;

            if (showOutput)
                hp::printlnCl(std::format("\n{} Link Command: {}", tag, linkCmd), hp::YELLOW);

            auto linkResult = hm::Process::run(linkCmd);
            exitCode = linkResult.exitCode;

            {
                std::ofstream linkLog(r_logPath);
                linkLog << linkResult.output;
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
        for (auto &a : comp->directBuildArgs(inputs, c.output))
            args.push_back(a);

        std::string command = hm::Process::join(args);
        if (c.run)
            command += " && " + c.output;

        if (showOutput)
            hp::printlnCl(std::format("{} Real Command: {}", tag, command), hp::YELLOW);

        auto buildResult = hm::Process::run(command);
        exitCode = buildResult.exitCode;

        {
            std::ofstream buildLog(r_logPath);
            buildLog << buildResult.output;
        }
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

                        std::filesystem::path p(copy);
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

        if (isConfig)
            hp::printlnCl(std::format("\nConfig '{}' compiled successfully. Output: {}", name, c.output), hp::GREEN);
        else
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
            hp::printlnCl(std::format("See Logs: {}", logPath), hp::YELLOW);
        }
        return 1;
    }

    if (showOutput || c.time) {
        if (isConfig)
            std::cout << std::format("{} Build Time: {}s\n", tag, elapsed);
        else
            std::cout << std::format("[HelpMake] Build Time: {}s\n", elapsed);
    }
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
    local.quiet = local.quiet || cfg.quiet;
    local.notify = local.notify || cfg.notify;
    local.rebuild = local.rebuild || cfg.rebuild;
    local.json = local.json || cfg.json;
    local.time = local.time || cfg.time;
    local.seperate = local.seperate && cfg.seperate;

    return local;
}

void Parser::executeConfigs(const std::vector<std::string> &names) {
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