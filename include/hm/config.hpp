#pragma once

#include "hm/dependecy.hpp"
#include "hm/parser/var.hpp"
#include "hp/colors/color.hpp"
#include <string>
#include <vector>

struct Config {
    hm::Var var;
    hm::Dependency dep;

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
    std::string pch;

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
    std::vector<std::string> v_Depends;
    std::vector<std::string> v_Gens;

    std::map<std::string, hp::Color> v_Info;
    std::map<std::string, std::string> v_Vars;

    bool verbose = false;
    bool debug = false;
    bool dryRun = false;
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
    bool check = false;
    bool watch = false;
    bool test = false;
    bool frozen = false;
    bool updateLock = false;

    int jobs = 0;
};