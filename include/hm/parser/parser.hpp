#pragma once

#include "hm/config.hpp"
#include "hm/job.hpp"
#include "hm/util/utils.hpp"

#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"
#include "hp/serializer/serialize.hpp"
#include "hp/string/inputs.hpp"
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
    void install();
    void compileLib();

    void createConfig(Config &c);
    void executeConfigs(const std::vector<std::string> &names);
    void generateCompileCommands(const Config &c);
    void Notify(const double &time, bool success = true);
    void parseArgument(Config &config, std::string_view value);
    void parseTypos(std::size_t num, std::filesystem::path filename, std::string_view line, std::string_view value);
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