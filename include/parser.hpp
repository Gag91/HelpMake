#pragma once

#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"
#include "hp/serializer/serialize.hpp"
#include "hp/system/command.hpp"
#include "hp/time/time.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

inline std::string_view trim_view(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.remove_suffix(1);
    return s;
}

inline bool similar(std::string_view first, std::string_view second) {
    if (first.size() != second.size())
        return false;
    if (first == second)
        return true;

    for (std::size_t i = 0; i < first.size(); i++) {
        if (static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(first[i]))) !=
            static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(second[i]))))
            return false;
    }
    return true;
}

inline std::string to_lower(std::string_view str) {
    std::string result(str.data(), str.size());
    std::ranges::transform(result, result.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

inline bool is_typo(std::string_view str1, std::string_view str2) {
    if (str1 == str2)
        return false;

    std::string a = to_lower(str1);
    std::string b = to_lower(str2);

    if (a == b)
        return true;

    if (a.size() == b.size()) {
        int errors = 0;
        for (std::size_t i = 0; i < a.size(); ++i)
            if (a[i] != b[i] && ++errors >= 2)
                return false;
        return errors == 1;
    }

    if (a.size() > b.size())
        std::swap(a, b);
    if (b.size() != a.size() + 1)
        return false;

    std::size_t i = 0, j = 0;
    bool skipped = false;
    while (i < a.size() && j < b.size()) {
        if (a[i] != b[j]) {
            if (skipped)
                return false;
            skipped = true;
            ++j;
        } else {
            ++i;
            ++j;
        }
    }
    return true;
}

inline bool isDeclaredModule(const std::string &file, const std::vector<std::string> &modules) {
    for (const auto &m : modules) {
        if (std::filesystem::path(m).lexically_normal() == std::filesystem::path(file).lexically_normal())
            return true;
    }
    return false;
}

inline std::string trim(const std::string &s) {
    std::size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    std::size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

inline std::string extractValue(const std::string &line, const std::string &key) {
    std::size_t pos = line.find(key);
    if (pos == std::string::npos)
        return "";
    return trim(line.substr(pos + key.size()));
}

inline std::string extractModule(const std::filesystem::path &filename) {
    std::ifstream file(filename);
    if (!file) {
        hp::printlnCl(std::format("[HelpMake] Error: Could not open file '{}'", filename.string()), hp::Color::RED);
        exit(EXIT_FAILURE);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("export module") == std::string::npos)
            continue;

        std::string value = extractValue(line, "export module");

        while (!value.empty() && (value.back() == ';' || std::isspace(static_cast<unsigned char>(value.back()))))
            value.pop_back();

        return trim(value);
    }

    hp::printlnCl(std::format("[HelpMake] Error: File '{}' is not a module", filename.string()), hp::Color::RED);
    exit(EXIT_FAILURE);
}

inline bool isModule(const std::string_view &s) {
    return s.ends_with(".cppm") || s.ends_with(".ixx") || s.ends_with(".mpp") ||
           s.ends_with(".cxxm") || s.ends_with(".c++m") || s.ends_with(".ccm");
}

inline bool gitAvailable() {
#ifdef _WIN32
    return hp::command("where git").find("INFO:") == std::string::npos;
#else
    return std::system("which git >/dev/null 2>&1") == 0;
#endif
}

inline std::string compilerCommand(const std::string &compiler) {
    if (compiler == "msvc")
        return "cl";
    return compiler;
}

inline constexpr std::array<std::string_view, 2> validKinds = {
    "executable",
    "static",
};

inline bool isValidKind(std::string_view key) {
    std::string k = to_lower(key);

    for (auto v : validKinds) {
        if (v == k)
            return true;
        if (is_typo(k, v)) {
            hp::printlnCl(std::format("[HelpMake] Error: Invalid Kind '{}'. Did you mean {} ?", k, v), hp::RED);
            exit(EXIT_FAILURE);
        }
    }
    return false;
}

struct Config {
    std::string name;

    std::string compiler;
    std::string version;
    std::string output;
    std::string flags;
    std::string inputFile;
    std::string includes;
    std::string modules;
    std::string Precmd;
    std::string Postcmd;
    std::string kind;

    std::vector<std::string> preBuild;
    std::vector<std::string> postBuild;

    std::vector<std::string> includeFiles;
    std::vector<std::string> v_inputFiles;
    std::vector<std::string> v_Flags;
    std::vector<std::string> v_Modules;
    std::vector<std::string> v_Github;
    std::vector<std::string> v_Postcmd;
    std::vector<std::string> v_Precmd;
    std::vector<std::string> v_Errors;
    std::vector<std::string> v_Copy;

    bool verbose = false;
    bool debug = false;
    bool run = false;
    bool n_file = false;
    bool buildSere = false;
    bool create = false;
    bool allCfgs = false;
    bool json = false;
    bool seperate = true;
    bool rebuild = false;
    bool quiet = false;
    bool notify = false;
    bool time = false;

    int jobs;
};

struct Job {
    std::string file;
    std::string cmd;
    std::string logPath;
    bool failed;
};

class Parser {
  private:
    std::filesystem::path filename;
    Config cfg;

    bool isInputFileSet = false;
    bool isIncludeSet = false;
    bool isFetchSet = false;
    bool isFlagsSet = false;
    bool isModulesSet = false;
    bool isPreBuildSet = false;
    bool isPostBuildSet = false;
    bool isConfigSet = false;
    bool isArgSet = false;
    bool isCopySet = false;

    Config currentConfig;
    std::map<std::string, Config> configs;
    int configBraceDepth = 1;
    static std::vector<std::string> splitArgs(const std::string &args);

  public:
    Parser(std::filesystem::path file, Config config)
        : filename(file), cfg(config) {}

    class Init {
      private:
        Config InitCfg;

        std::string detectFiles();
        std::string detectIncludes();
        std::string defaultOutput();
        void chooseLanguage();
        void showPreview();
        void previewAndSave();
        std::string guessStandard();

      public:
        Init();
        void run();
    };

    int runJobs(std::vector<Job> &jobs, std::size_t num);

    void parse();
    void execute();
    void buildCommand();
    void executeConfigs(const std::vector<std::string> &names);
    void generateCompileCommands();
    void install();
    void Notify(const double &time, bool success = true);
    void parseArgument(Config &config, std::string_view value);
    void parseTypos(std::size_t num, std::filesystem::path filename, std::string_view line, std::string_view value);
    void compileLib();
    void executeConfig(const Config &c, const std::string &name);

    Config inheritConfig(const Config &child);

    std::vector<std::string> getInputFile();
    std::vector<std::string> getInclude();
    std::vector<std::string> getFlags();
    std::vector<std::string> getModules();
    std::vector<std::string> getGithub();
    std::vector<std::string> getCopy();

    std::map<std::string, Config> &getConfigs();

    std::string getKind();
    std::string processGithubEntry(const std::string &value);
    std::string getCompiler();
    std::string getOutput();
    std::string getVersion();

    std::string r_logDir = "build/HelpMake/logs/raw";
    std::string logDir = "build/HelpMake/logs";

    std::size_t dotPos = cfg.output.find_last_of('.');
    std::string baseName = (dotPos != std::string::npos) ? cfg.output.substr(0, dotPos) : cfg.output;

    std::string r_logPath = r_logDir + "/" + baseName + ".txt";
    std::string logPath = logDir + "/" + baseName + ".txt";

    bool needsRebuild(const std::string &src, const std::string &obj, bool rebuild);
};