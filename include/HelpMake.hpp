#pragma once

#include "hm/parser/parser.hpp"

#include <concepts>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace hp {

    template <typename T>
    concept StringLike = std::convertible_to<T, std::string>;

    class HelpMake {
      private:
        std::filesystem::path filename;
        Config cfg;

        static std::string joinVec(const std::vector<std::string> &v) {
            std::string result;
            for (const auto &s : v) {
                if (!result.empty())
                    result += " ";
                result += s;
            }
            return result;
        }

      public:
        HelpMake() = default;

        template <StringLike T>
        HelpMake &setCompiler(T &&v) {
            cfg.compiler = std::string(std::forward<T>(v));
            return *this;
        }

        template <StringLike T>
        HelpMake &setVersion(T &&v) {
            std::string s = std::string(std::forward<T>(v));
            if (s.rfind("std=", 0) != 0)
                s = "std=" + s;
            cfg.version = s;
            return *this;
        }

        template <StringLike T>
        HelpMake &setStandard(T &&v) {
            return setVersion(std::forward<T>(v));
        }

        template <StringLike T>
        HelpMake &setOutput(T &&v) {
            cfg.output = std::string(std::forward<T>(v));
            return *this;
        }

        template <StringLike T>
        HelpMake &setFlags(T &&v) {
            cfg.flags = std::string(std::forward<T>(v));
            return *this;
        }

        template <StringLike T>
        HelpMake &setInputFile(T &&v) {
            cfg.inputFile = std::string(std::forward<T>(v));
            return *this;
        }

        template <StringLike T>
        HelpMake &setModules(T &&v) {
            cfg.modules = std::string(std::forward<T>(v));
            return *this;
        }

        template <StringLike T>
        HelpMake &setPrecmd(T &&v) {
            cfg.Precmd = std::string(std::forward<T>(v));
            return *this;
        }

        template <StringLike T>
        HelpMake &setPostcmd(T &&v) {
            cfg.Postcmd = std::string(std::forward<T>(v));
            return *this;
        }

        template <typename T>
            requires std::convertible_to<T, std::filesystem::path>
        HelpMake &setConfigFile(T &&v) {
            filename = std::filesystem::path(std::forward<T>(v));
            return *this;
        }

        HelpMake &addInputFile(const std::string &f) {
            cfg.v_inputFiles.push_back(f);
            return *this;
        }

        HelpMake &addFlag(const std::string &f) {
            cfg.v_Flags.push_back(f);
            return *this;
        }

        HelpMake &addInclude(const std::string &i) {
            cfg.includeFiles.push_back(i);
            return *this;
        }

        HelpMake &addModule(const std::string &m) {
            cfg.v_Modules.push_back(m);
            return *this;
        }

        HelpMake &addGithub(const std::string &repo) {
            cfg.v_Github.push_back(repo);
            return *this;
        }

        HelpMake &addPrecmd(const std::string &cmd) {
            cfg.v_Precmd.push_back(cmd);
            return *this;
        }

        HelpMake &addPostcmd(const std::string &cmd) {
            cfg.v_Postcmd.push_back(cmd);
            return *this;
        }

        HelpMake &setIncludeFiles(std::vector<std::string> include) {
            cfg.includeFiles = std::move(include);
            return *this;
        }

        HelpMake &setInputFiles(std::vector<std::string> files) {
            cfg.v_inputFiles = std::move(files);
            return *this;
        }

        HelpMake &setFlagList(std::vector<std::string> flagList) {
            cfg.v_Flags = std::move(flagList);
            return *this;
        }

        HelpMake &setModuleList(std::vector<std::string> mods) {
            cfg.v_Modules = std::move(mods);
            return *this;
        }

        HelpMake &setGithub(std::vector<std::string> repos) {
            cfg.v_Github = std::move(repos);
            return *this;
        }

        HelpMake &setPostcmd(std::vector<std::string> cmds) {
            cfg.v_Postcmd = std::move(cmds);
            return *this;
        }

        HelpMake &setPrecmd(std::vector<std::string> cmds) {
            cfg.v_Precmd = std::move(cmds);
            return *this;
        }

        HelpMake &setVerbose(bool v = true) {
            cfg.verbose = v;
            return *this;
        }

        HelpMake &setDebug(bool v = true) {
            cfg.debug = v;
            return *this;
        }

        HelpMake &setRun(bool v = true) {
            cfg.run = v;
            return *this;
        }

        HelpMake &setNoFile(bool v = true) {
            cfg.n_file = v;
            return *this;
        }

        HelpMake &setBuildSere(bool v = true) {
            cfg.buildSere = v;
            return *this;
        }

        HelpMake &setCreate(bool v = true) {
            cfg.create = v;
            return *this;
        }

        HelpMake &setAllCfgs(bool v = true) {
            cfg.allCfgs = v;
            return *this;
        }

        HelpMake &setJson(bool v = true) {
            cfg.json = v;
            return *this;
        }

        HelpMake &setSeparate(bool v = true) {
            cfg.seperate = v;
            return *this;
        }

        HelpMake &setRebuild(bool v = true) {
            cfg.rebuild = v;
            return *this;
        }

        std::string getCompiler() const {
            return cfg.compiler;
        }
        std::string getVersion() const {
            return cfg.version;
        }
        std::string getOutput() const {
            return cfg.output;
        }
        std::string getFlags() const {
            return cfg.flags;
        }
        std::string getInputFile() const {
            return cfg.inputFile;
        }
        const std::vector<std::string> &getInputFiles() const {
            return cfg.v_inputFiles;
        }
        const std::vector<std::string> &getFlagList() const {
            return cfg.v_Flags;
        }
        const std::vector<std::string> &getIncludeFiles() const {
            return cfg.includeFiles;
        }
        const std::vector<std::string> &getModules() const {
            return cfg.v_Modules;
        }
        const std::vector<std::string> &getGithub() const {
            return cfg.v_Github;
        }

        void build() {
            if (filename.empty())
                filename = "HelpMake.txt";

            std::string joinedInputs = joinVec(cfg.v_inputFiles);
            if (!joinedInputs.empty())
                cfg.inputFile = joinedInputs;

            std::string joinedFlags = joinVec(cfg.v_Flags);
            if (!joinedFlags.empty()) {
                if (!cfg.flags.empty()) {
                    cfg.flags += " " + joinedFlags;
                } else {
                    cfg.flags = joinedFlags;
                }
            }

            std::string joinedModules = joinVec(cfg.v_Modules);
            if (!joinedModules.empty()) {
                if (!cfg.modules.empty())
                    cfg.modules += " " + joinedModules;
                else
                    cfg.modules = joinedModules;
            }

            for (const auto &inc : cfg.includeFiles)
                cfg.flags += " -I" + inc;

            if (!cfg.v_Precmd.empty())
                cfg.Precmd = joinVec(cfg.v_Precmd);

            if (!cfg.v_Postcmd.empty())
                cfg.Postcmd = joinVec(cfg.v_Postcmd);

            Parser parser(filename, cfg);
            parser.parse();
            if (!cfg.create) {
                parser.executeConfigs({});
                if (!cfg.allCfgs) {
                    parser.execute();
                }
            }
        }

        void buildAndRun() {
            cfg.run = true;
            build();
        }

        void buildConfig(const std::string &name) {
            if (filename.empty())
                filename = "HelpMake.txt";

            Parser parser(filename, cfg);
            parser.parse();
            parser.executeConfigs({name});
        }
    };

} // namespace hp