#include "hm/cli/cli.hpp"
#include "hm/parser/parser.hpp"

#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace hm::cli {

    static void printList(const std::string &label, const std::vector<std::string> &items, int indent = 0) {
        if (items.empty())
            return;

        std::string pad(indent, ' ');
        std::string inner(indent + 4, ' ');
        std::cout << pad << label << ":\n";
        for (const auto &item : items)
            std::cout << inner << item << "\n";
        std::cout << "\n";
    }

    static void printMap(const std::string &label, const std::map<std::string, std::string> &items, int indent = 4) {
        if (items.empty())
            return;

        std::string pad(indent, ' ');
        std::string inner(indent + 4, ' ');
        std::cout << pad << label << ":\n";
        for (const auto &[key, value] : items)
            std::cout << inner << std::left << std::setw(10) << (key + ":") << value << "\n";
        std::cout << "\n";
    }

    static void print(const std::string &label, const std::string &item, int indent = 4) {
        if (item.empty())
            return;

        std::string pad(indent, ' ');
        std::cout << pad << std::left << std::setw(10) << (label + ":") << item << "\n";
    }

    void show(const std::filesystem::path &filename, Config &config) {
        Parser parser(filename, config);
        parser.parse();

        std::cout << "Compiler: " << parser.getCompiler() << "\n";
        std::cout << "Version:  " << parser.getVersion() << "\n";
        std::cout << "Output:   " << parser.getOutput() << "\n";

        std::string kind = parser.getKind();
        if (!kind.empty() && kind != "executable")
            std::cout << "Kind:     " << kind << "\n";

        const Config &root = parser.getConfig();
        if (!root.pch.empty())
            std::cout << "PCH:      " << root.pch << "\n";
        if (root.run)
            std::cout << "Run:      true\n";
        if (root.jobs > 0)
            std::cout << "Jobs:     " << root.jobs << "\n";

        std::cout << "\n";

        printMap("Variables", parser.getVars(), 0);
        printList("InputFiles", parser.getInputFile());
        printList("IncludeFiles", parser.getInclude());
        printList("Flags", parser.getFlags());
        printList("Modules", parser.getModules());
        printList("Depends", root.v_Depends);
        printList("PreBuild", root.v_Precmd);
        printList("PostBuild", root.v_Postcmd);
        printList("Fetch", parser.getGithub());
        printList("Copy", parser.getCopy());
        printList("Gen", parser.getGens());

        if (!parser.dependencies.empty()) {
            std::cout << "Dependencies:\n\n";

            for (const auto &[name, dep] : parser.dependencies) {
                std::cout << "    " << name << ":\n";

                print("Git", dep.git, 8);
                print("Url", dep.url, 8);
                print("Tag", dep.tag, 8);
                print("Hash", dep.hash, 8);
                print("Path", dep.path, 8);
                print("Commit", dep.commit, 8);

                printList("Builds", dep.builds, 8);
                printList("Includes", dep.includes, 8);
                printList("Links", dep.links, 8);
                printList("LinkDirs", dep.linkDirs, 8);
                printList("LinkFlags", dep.linkFlags, 8);
                printList("Depends", dep.depends, 8);
            }
        }

        const auto &configs = parser.getConfigs();
        if (configs.empty())
            return;

        std::cout << "Configs:\n";

        for (const auto &[name, c] : configs) {
            std::cout << "    " << name << ":\n";

            if (c.test)
                std::cout << "        Test:       true\n";
            if (!c.compiler.empty())
                std::cout << "        Compiler:   " << c.compiler << "\n";
            if (!c.version.empty())
                std::cout << "        Version:    " << c.version << "\n";
            if (!c.output.empty())
                std::cout << "        Output:     " << c.output << "\n";
            if (!c.kind.empty() && c.kind != "executable")
                std::cout << "        Kind:       " << c.kind << "\n";
            if (!c.pch.empty())
                std::cout << "        PCH:        " << c.pch << "\n";
            if (c.run)
                std::cout << "        Run:        true\n";
            if (c.jobs > 0)
                std::cout << "        Jobs:       " << c.jobs << "\n";

            auto vars = c.var;
            printMap("Variables", vars.getVariables(), 8);
            printList("InputFiles", c.v_inputFiles, 8);
            printList("Includes", c.includeFiles, 8);
            printList("Flags", c.v_Flags, 8);
            printList("Modules", c.v_Modules, 8);
            printList("Depends", c.v_Depends, 8);
            printList("PreBuild", c.v_Precmd, 8);
            printList("PostBuild", c.v_Postcmd, 8);
            printList("Copy", c.v_Copy, 8);
            printList("Gen", c.v_Gens, 8);

            std::cout << "\n";
        }
    }

} // namespace hm::cli