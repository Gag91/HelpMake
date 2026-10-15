#include "parser.hpp"

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
            std::filesystem::path p(entry);
            std::string extension = p.extension().string();
            if (entry.find("**") != std::string::npos) {
                std::size_t pos = entry.find("**");
                std::filesystem::path folder = entry.substr(0, pos);
                for (const auto &f : std::filesystem::recursive_directory_iterator(folder)) {
                    if (f.is_regular_file() && f.path().extension() == extension) {
                        expandedFiles.push_back(f.path().generic_string());
                    }
                }
            } else if (entry.find('*') != std::string::npos) {
                std::string dir = p.parent_path().string();
                if (dir.empty())
                    dir = ".";

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
        std::vector<Job> jobs;

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

                    std::string pcmLog = std::format("build/HelpMake/logs/raw/{}.pcm.raw", baseName);
                    std::string redirectCmd = std::format("{} > \"{}\" 2>&1", pcmCmd, pcmLog);
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

            std::string perFileLog = std::format("build/HelpMake/logs/raw/{}.raw", baseName);
            jobs.push_back({file, objCmd, perFileLog});
            objects.push_back(objPath);
        }

        if (exitCode == 0) {
            int rc = runJobs(jobs, c.jobs);
            if (rc != 0)
                exitCode = rc;
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
                        hp::printlnCl(std::format("{} Error: Copy source '{}' does not exist", tag, src), hp::Color::RED);
                        hp::printlnCl(std::format("{} Build Failed.", tag), hp::RED);
                        exit(EXIT_FAILURE);
                    }
                } catch (const std::filesystem::filesystem_error &e) {
                    hp::printlnCl(std::format("{} Copy failed: {}", tag, e.what()), hp::Color::RED);
                    hp::printlnCl(std::format("{} Build Failed.", tag), hp::RED);
                    exit(EXIT_FAILURE);
                }
            }
        }

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
            std::cout << std::format("{} Build Time: {}s\n", tag, elapsed);
        else
            std::cout << std::format("[HelpMake] Build Time: {}s\n", elapsed);
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
    if (local.v_Copy.empty())
        local.v_Copy = cfg.v_Copy;

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
                    hp::printlnCl(std::format("[Debug] Dependency '{}' is newer than object '{}'. Rebuilding.", depFilePath, obj), hp::Color::YELLOW);
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

int Parser::runJobs(std::vector<Job> &jobs, std::size_t n) {
    if (jobs.empty())
        return 0;
    if (n == 0)
        n = std::thread::hardware_concurrency();
    if (n == 0)
        n = 1;
    n = std::min<std::size_t>(n, jobs.size());

    std::vector<int> results(n, 0);
    std::vector<double> times(jobs.size(), 0.0);
    std::vector<std::thread> pool;
    std::size_t chunk = (jobs.size() + n - 1) / n;

    for (std::size_t t = 0; t < n; ++t) {
        std::size_t begin = t * chunk;
        std::size_t end = std::min(begin + chunk, jobs.size());
        if (begin >= jobs.size())
            break;

        pool.emplace_back([&, t, begin, end]() {
            for (std::size_t i = begin; i < end; ++i) {
                auto &j = jobs[i];
                std::string redirect = std::format("{} > \"{}\" 2>&1", j.cmd, j.logPath);
                auto timer = hp::startTimer();
                int rc = std::system(redirect.c_str());
                double elapsed = hp::stopTimer(timer);

                times[i] = elapsed;

                if (rc != 0) {
                    results[t] = rc;
                    j.failed = true;

                    std::ifstream logFile(j.logPath);
                    if (logFile.is_open()) {
                        std::string content((std::istreambuf_iterator<char>(logFile)),
                                            std::istreambuf_iterator<char>());
                        if (!content.empty()) {
                            std::cerr << content;
                            if (content.back() != '\n')
                                std::cerr << '\n';
                        }
                    }
                    return;
                }
            }
        });
    }

    for (auto &th : pool)
        th.join();

    hp::printlnCl("\n[HelpMake] Compile phase:", hp::YELLOW);
    for (std::size_t i = 0; i < jobs.size(); ++i) {
        if (jobs[i].failed)
            hp::printlnCl(std::format("- {} failed to compile", jobs[i].file), hp::RED);
        else
            hp::printlnCl(std::format("- {} compiled in {}s", jobs[i].file, times[i]), hp::YELLOW);
    }

    for (int rc : results)
        if (rc != 0)
            return rc;

    return 0;
}