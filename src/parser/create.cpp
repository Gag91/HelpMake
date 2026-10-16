#include "hm/parser/parser.hpp"

inline std::string stringifyArgs(std::vector<std::string> arg) {
    std::string result = "\n";
    if (!arg.empty()) {
        for (const auto &a : arg) {
            result += "    " + a;
        }
    }
    return result;
}

void Parser::createConfig(Config &c) {
    hp::File file;

    c.v_inputFiles = splitArgs(cfg.inputFile);
    c.v_Flags = splitArgs(cfg.flags);
    c.includeFiles = splitArgs(cfg.includes);
    c.v_Modules = splitArgs(cfg.modules);

    if (file.exists(filename.string()) && !cfg.quiet) {
        hp::printlnCl(std::format("Warning: {} already exists. Overwrite? (y/n)", filename.string()), hp::Color::YELLOW);
        char verif = hp::get<char>("", "Invalid choice");
        if (verif != 'y') {
            hp::printlnCl("File was not overwritten.", hp::Color::GREEN);
            return;
        }
    }

    hp::save(filename.string(), {{"Compiler", cfg.compiler}, {"Version", cfg.version}, {"Output", cfg.output}});
    std::ofstream make(filename.string(), std::ios::app);

    std::map<std::string_view, std::vector<std::string>> arg = {
        {"InputFiles {", c.v_inputFiles},
        {"Includes {", c.includeFiles},
        {"Flags {", c.v_Flags},
        {"Modules {", c.v_Modules}};

    for (const auto [key, value] : arg) {
        if (!value.empty())
            make << key << stringifyArgs(value) << "\n}\n";
    }
    make.close();
    hp::printlnCl(std::format("{} was created successfully.", filename.string()), hp::Color::GREEN);
    return;
}