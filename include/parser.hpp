#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

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

    std::vector<std::string> preBuild;
    std::vector<std::string> postBuild;

    std::vector<std::string> includeFiles;
    std::vector<std::string> v_inputFiles;
    std::vector<std::string> v_Flags;
    std::vector<std::string> v_Modules;
    std::vector<std::string> v_Github;
    std::vector<std::string> v_Postcmd;
    std::vector<std::string> v_Precmd;

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

    void parse();
    void execute();
    void buildCommand();
    void executeConfigs(const std::vector<std::string> &names);
    void generateCompileCommands();
    void install();
    void Notify(const double &time, bool success = true);
    void parseArgument(Config &config, std::string_view value);
    void parseTypos(std::filesystem::path filename, std::string_view line, std::string_view value);

    std::vector<std::string> getInputFile();
    std::vector<std::string> getInclude();
    std::vector<std::string> getFlags();
    std::vector<std::string> getModules();
    std::vector<std::string> getGithub();

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