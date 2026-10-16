#include "hm/parser/parser.hpp"

void Parser::generateCompileCommands(const Config &c) {
    std::ofstream json("compile_commands.json");
    if (!json.is_open()) {
        if (!cfg.quiet)
            hp::printlnCl("Warning: Could not write compile_commands.json", hp::Color::YELLOW);
        return;
    }

    std::string cwd = std::filesystem::current_path().generic_string();
    std::string comp = compilerCommand(c.compiler);
    bool isClang = (comp == "clang++" || comp == "clang");

    std::string version = c.version;
    if (!version.empty() && version.front() != '-')
        version.insert(version.begin(), '-');

    std::string CommandFlags;
    if (!version.empty())
        CommandFlags = version;

    if (!c.flags.empty())
        CommandFlags += (CommandFlags.empty() ? "" : " ") + c.flags;

    std::vector<std::string> files = expandInputs(c);

    json << "[\n";
    bool firstEntry = true;

    for (const auto &file : files) {
        std::string baseName = std::filesystem::path(file).stem().string();
        bool isModExt = isModule(file);
        bool isMod = isModExt || isDeclaredModule(file, c.v_Modules);
        std::string objPath = std::format("build/HelpMake/obj/{}.o", baseName);
        std::string command;

        if (isClang && isMod) {
            std::string moduleName = extractModule(file);
            std::string pcmPath = std::format("build/HelpMake/modules/{}.pcm", baseName);
            std::string langFlag = isModExt ? "" : "-xc++-module ";
            command = std::format("{} -fdiagnostics-color=always {}{} -c {} -o {} -MMD -fmodule-file={}={}", comp, langFlag, CommandFlags, file, objPath, moduleName, pcmPath);
        } else {
            std::string langFlag = (comp == "g++") ? "-xc++ " : "";
            command = std::format("{} -fdiagnostics-color=always {}{} -c {} -o {} -MMD", comp, langFlag, CommandFlags, file, objPath);
        }

        if (!firstEntry)
            json << ",\n";
        firstEntry = false;

        json << "  {\n";
        json << "    \"directory\": \"" << escapeJson(toUnixPath(cwd)) << "\",\n";
        json << "    \"command\": \"" << escapeJson(toUnixPath(command)) << "\",\n";
        json << "    \"file\": \"" << escapeJson(toUnixPath(file)) << "\"\n";
        json << "  }";
    }

    json << "\n]\n";
    json.close();

    if (c.verbose)
        hp::printlnCl("[HelpMake] Generated compile_commands.json", hp::Color::GREEN);
}