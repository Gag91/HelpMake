#pragma once

#include "hm/config.hpp"
#include "hm/dependecy.hpp"
#include "hm/job.hpp"
#include "hm/parser/var.hpp"
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

struct FileSection {
    std::vector<std::string> lines;
    std::string configName;
    std::size_t startLine = 0;
};

struct LockEntry {
    std::string url;
    std::string commit;
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
    bool isVarSet = false;
    bool isDependencySet = false;

    Config currentConfig;
    std::map<std::string, Config> configs;
    std::map<std::string, std::string> fetchRoots;
    int configBraceDepth = 1;
    static std::vector<std::string> splitArgs(const std::string &args);
    bool isGitCheckout(const std::string &path, const std::string &url) const;
    bool removeGitCheckout(const std::string &path, std::string &error) const;

    std::optional<std::string> fieldValue(std::string_view line, std::string_view key);
    void appendInputFiles(Config &c, const std::string &value);
    void appendIncludes(Config &c, const std::string &value);
    void appendFlags(Config &c, const std::string &value);
    void appendModules(Config &c, const std::string &value);
    void appendPreBuild(Config &c, const std::string &value);
    void appendPostBuild(Config &c, const std::string &value);
    void setKind(Config &c, const std::string &rawValue, std::size_t lineNo);
    void setJobs(Config &c, const std::string &rawValue, std::size_t lineNo);

    template <typename T>
    void debugField(std::string_view label, const T &value, std::string_view cfgName = "") const {
        if (!cfg.debug)
            return;
        if (cfgName.empty())
            hp::printlnCl(std::format("Found {}: '{}'", label, value), hp::YELLOW);
        else
            hp::printlnCl(std::format("  {}: '{}'", label, value), hp::YELLOW);
    }

  public:
    Parser(std::filesystem::path file, Config &config)
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
    int executeConfig(const Config &c, const std::string &name);
    int execute();
    int parse();
    int buildCommand();
    int parseSection(const FileSection &section, Config &target, bool isRoot);
    int buildDependency(const hm::Dependency &dep);
    int updateLock();
    int resolveDependencies(Config &c, const std::string &tag);
    int generateCompileCommands(const Config &c);
    int executeConfigs(const std::vector<std::string> &names);

    void install();
    void compileLib();
    void watch();
    void createConfig(Config &c);
    void Notify(const double &time, bool success = true);
    void parseArgument(Config &config, std::string_view value);
    void parseTypos(std::size_t num, std::filesystem::path filename, std::string_view line, std::string_view value);

    Config inheritConfig(const Config &child);

    std::vector<std::string> v_DepFiles;

    std::vector<std::string> getInputFile();
    std::vector<std::string> getInclude();
    std::vector<std::string> getFlags();
    std::vector<std::string> getModules();
    std::vector<std::string> getGithub();
    std::vector<std::string> getCopy();
    std::vector<std::string> getGens();
    std::vector<FileSection> splitIntoFileSections(std::istream &in);

    std::map<std::string, hm::Dependency> dependencies;
    std::map<std::string, LockEntry> lockEntries;
    std::map<std::string, Config> &getConfigs();
    std::map<std::string, std::string> &getVars();

    Config &getConfig();
    std::string getKind();
    std::string processGithubEntry(const std::string &value);
    std::string getCompiler();
    std::string getOutput();
    std::string getVersion();

    std::string r_logDir = "build/HelpMake/logs/raw";
    std::string logDir = "build/HelpMake/logs";

    std::string baseName = std::filesystem::path(cfg.output).stem().string();

    std::string r_logPath = r_logDir + "/" + baseName + ".txt";
    std::string logPath = logDir + "/" + baseName + ".txt";

    bool needsRebuild(const std::string &src, const std::string &obj, bool rebuild);
    bool PchneedsRebuild(const std::string &src, const std::string &obj, bool rebuild);
};