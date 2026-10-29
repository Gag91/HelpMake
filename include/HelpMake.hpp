#pragma once

#include "hm/generator/generator.hpp"
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

        void normalize() {
            std::string joinedInputs = joinVec(cfg.v_inputFiles);
            if (!joinedInputs.empty())
                cfg.inputFile = joinedInputs;

            std::string joinedFlags = joinVec(cfg.v_Flags);
            if (!joinedFlags.empty()) {
                if (!cfg.flags.empty())
                    cfg.flags += " " + joinedFlags;
                else
                    cfg.flags = joinedFlags;
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
        }

      public:
        HelpMake() = default;

        HelpMake &setCompiler(const std::string &v) {
            cfg.compiler = v;
            return *this;
        }

        HelpMake &setVersion(const std::string &v) {
            std::string s = v;
            if (s.rfind("std=", 0) != 0 && s.rfind("-std=", 0) != 0 && s.rfind("/std:", 0) != 0)
                s = "std=" + s;
            cfg.version = s;
            return *this;
        }

        HelpMake &setStandard(const std::string &v) {
            return setVersion(v);
        }

        HelpMake &setOutput(const std::string &v) {
            cfg.output = v;
            return *this;
        }

        HelpMake &setFlags(const std::string &v) {
            cfg.flags = v;
            return *this;
        }

        HelpMake &setInputFile(const std::string &v) {
            cfg.inputFile = v;
            return *this;
        }

        HelpMake &setModules(const std::string &v) {
            cfg.modules = v;
            return *this;
        }

        HelpMake &setPrecmd(const std::string &v) {
            cfg.Precmd = v;
            return *this;
        }

        HelpMake &setPostcmd(const std::string &v) {
            cfg.Postcmd = v;
            return *this;
        }

        HelpMake &setPch(const std::string &v) {
            cfg.pch = v;
            return *this;
        }

        HelpMake &setKind(const std::string &v) {
            cfg.kind = v;
            return *this;
        }

        HelpMake &setJobs(int n) {
            cfg.jobs = n;
            return *this;
        }

        HelpMake &setConfigFile(const std::filesystem::path &v) {
            filename = v;
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

        HelpMake &addFetch(const std::string &repo) {
            cfg.v_Github.push_back(repo);
            return *this;
        }

        HelpMake &addCopy(const std::string &entry) {
            cfg.v_Copy.push_back(entry);
            return *this;
        }

        HelpMake &addDepends(const std::string &dep) {
            cfg.v_Depends.push_back(dep);
            return *this;
        }

        HelpMake &addGen(const std::string &g) {
            cfg.v_Gens.push_back(g);
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

        HelpMake &addVar(const std::string &key, const std::string &value) {
            cfg.v_Vars[key] = value;
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

        HelpMake &setFetchList(std::vector<std::string> repos) {
            cfg.v_Github = std::move(repos);
            return *this;
        }

        HelpMake &setCopyList(std::vector<std::string> entries) {
            cfg.v_Copy = std::move(entries);
            return *this;
        }

        HelpMake &setDependsList(std::vector<std::string> deps) {
            cfg.v_Depends = std::move(deps);
            return *this;
        }

        HelpMake &setGenList(std::vector<std::string> gens) {
            cfg.v_Gens = std::move(gens);
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

        HelpMake &setVarMap(std::map<std::string, std::string> vars) {
            cfg.v_Vars = std::move(vars);
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

        HelpMake &setQuiet(bool v = true) {
            cfg.quiet = v;
            return *this;
        }

        HelpMake &setNotify(bool v = true) {
            cfg.notify = v;
            return *this;
        }

        HelpMake &setTime(bool v = true) {
            cfg.time = v;
            return *this;
        }

        HelpMake &setCheck(bool v = true) {
            cfg.check = v;
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

        HelpMake &setWatch(bool v = true) {
            cfg.watch = v;
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
        std::string getPch() const {
            return cfg.pch;
        }
        std::string getKind() const {
            return cfg.kind;
        }
        int getJobs() const {
            return cfg.jobs;
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
        const std::vector<std::string> &getFetch() const {
            return cfg.v_Github;
        }
        const std::vector<std::string> &getCopy() const {
            return cfg.v_Copy;
        }
        const std::vector<std::string> &getDepends() const {
            return cfg.v_Depends;
        }
        const std::vector<std::string> &getGens() const {
            return cfg.v_Gens;
        }
        const std::map<std::string, std::string> &getVars() const {
            return cfg.v_Vars;
        }
        const Config &getConfig() const {
            return cfg;
        }

        int build() {
            if (filename.empty())
                filename = "HelpMake.txt";

            normalize();

            Parser parser(filename, cfg);
            if (parser.parse() != 0)
                return 1;

            if (cfg.create)
                return 0;

            if (!cfg.allCfgs) {
                if (parser.execute() != 0)
                    return 1;
            }

            return 0;
        }

        int buildAndRun() {
            cfg.run = true;
            return build();
        }

        int buildConfig(const std::string &name) {
            if (filename.empty())
                filename = "HelpMake.txt";

            Parser parser(filename, cfg);
            if (parser.parse() != 0)
                return 1;

            if (parser.executeConfigs({name}) != 0)
                return 1;

            return 0;
        }

        int buildAllConfigs() {
            if (filename.empty())
                filename = "HelpMake.txt";

            normalize();

            Parser parser(filename, cfg);
            if (parser.parse() != 0)
                return 1;

            std::vector<std::string> names;
            for (const auto &[n, _] : parser.getConfigs())
                names.push_back(n);

            if (names.empty())
                return 0;

            return parser.executeConfigs(names) == 0 ? 0 : 1;
        }

        int test() {
            if (filename.empty())
                filename = "HelpMake.txt";

            normalize();

            Parser parser(filename, cfg);
            if (parser.parse() != 0)
                return 1;

            auto &configs = parser.getConfigs();
            std::vector<std::string> names;
            for (const auto &[name, c] : configs)
                if (c.test)
                    names.push_back(name);

            if (names.empty())
                return 0;

            int failed = 0;
            for (const auto &name : names) {
                Config local = parser.inheritConfig(configs[name]);
                local.run = true;
                local.quiet = true;
                if (parser.executeConfig(local, name) != 0)
                    failed++;
            }

            return failed == 0 ? 0 : 1;
        }

        int clean() {
            std::error_code ec;
            std::filesystem::remove_all("build/HelpMake", ec);
            if (ec)
                return 1;
            return 0;
        }

        int cleanAll() {
            std::error_code ec;
            std::filesystem::remove_all("build/HelpMake", ec);
            std::filesystem::remove_all("gcm.cache", ec);
            return ec ? 1 : 0;
        }

        int generate(const std::string &name) {
            if (filename.empty())
                filename = "HelpMake.txt";

            normalize();

            Parser parser(filename, cfg);
            if (parser.parse() != 0)
                return 1;

            auto gen = hm::gen::makeGenerators(name);
            if (!gen)
                return 1;

            Config genCfg = parser.getConfig();
            if (parser.resolveDependencies(genCfg, "[HelpMake]") != 0)
                return 1;

            return gen->generate(genCfg) ? 0 : 1;
        }
    };

} // namespace hp