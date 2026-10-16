#include "hm/cli/cli.hpp"
#include "hm/parser/parser.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace hm::cli {

    static void printList(const std::string &label, const std::vector<std::string> &items, int indent = 4) {
        if (items.empty())
            return;

        std::string pad(indent, ' ');
        std::cout << pad << label << ":\n";
        for (const auto &item : items)
            std::cout << pad << "    " << item << "\n";
        std::cout << "\n";
    }

    void show(const std::filesystem::path &filename, const Config &config) {
        Parser parser(filename, config);
        parser.parse();

        std::cout << "Compiler: " << parser.getCompiler() << "\n";
        std::cout << "Version:  " << parser.getVersion() << "\n";
        std::cout << "Output:   " << parser.getOutput() << "\n";

        std::string kind = parser.getKind();
        if (!kind.empty())
            std::cout << "Kind:     " << kind << "\n";

        std::cout << "\n";

        printList("InputFiles", parser.getInputFile());
        printList("IncludeFiles", parser.getInclude());
        printList("Flags", parser.getFlags());
        printList("Modules", parser.getModules());
        printList("Fetch", parser.getGithub());
        printList("Copy", parser.getCopy());

        const auto &configs = parser.getConfigs();
        if (configs.empty())
            return;

        std::cout << "Configs:\n";
        for (const auto &[name, c] : configs) {
            std::cout << "    " << name << ":\n";

            if (!c.compiler.empty())
                std::cout << "        Compiler:   " << c.compiler << "\n";
            if (!c.version.empty())
                std::cout << "        Version:    " << c.version << "\n";
            if (!c.output.empty())
                std::cout << "        Output:     " << c.output << "\n";
            if (!c.kind.empty())
                std::cout << "        Kind:       " << c.kind << "\n";

            printList("InputFiles", c.v_inputFiles, 8);
            printList("Includes", c.includeFiles, 8);
            printList("Flags", c.v_Flags, 8);
            printList("Modules", c.v_Modules, 8);
            printList("PreBuild", c.v_Precmd, 8);
            printList("PostBuild", c.v_Postcmd, 8);

            std::cout << "\n";
        }
    }

} // namespace hm::cli