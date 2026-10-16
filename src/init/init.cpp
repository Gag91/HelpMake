#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"
#include "hp/time/time.hpp"
#include "parser.hpp"

#include <array>
#include <filesystem>
#include <format>
#include <iostream>
#include <string_view>

static std::string ask(const std::string &label, const std::string &defaultVal) {
    std::cout << "  " << hp::getColorCode(hp::CYAN) << label << ": ";
    std::cout << hp::getColorCode(hp::BRIGHT_BLACK) << "[" << defaultVal << "] ";
    std::cout << hp::getColorCode(hp::RESET);

    std::string line;
    std::getline(std::cin, line);
    return line.empty() ? defaultVal : line;
}

static bool askYesNo(const std::string &label, bool defaultYes = true) {
    std::cout << "  " << hp::getColorCode(hp::CYAN) << label << " ";
    std::cout << hp::getColorCode(hp::BRIGHT_BLACK)
              << (defaultYes ? "(Y/n) " : "(y/N) ")
              << hp::getColorCode(hp::RESET);

    std::string line;
    std::getline(std::cin, line);
    if (line.empty())
        return defaultYes;
    return line[0] == 'y' || line[0] == 'Y';
}

Parser::Init::Init() = default;

std::string Parser::Init::detectFiles() {
    constexpr std::array<std::string_view, 6> extensions = {".cpp", ".c", ".cc", ".cxx", ".zig", ".rs"};

    for (const auto &ext : extensions) {
        std::size_t count = 0;
        if (std::filesystem::exists("src")) {
            for (const auto &entry : std::filesystem::recursive_directory_iterator("src")) {
                if (entry.is_regular_file() && entry.path().extension() == ext) {
                    count++;
                }
            }
            if (count > 0)
                return "src/*" + std::string(ext);
        }

        count = 0;
        for (const auto &entry : std::filesystem::directory_iterator(".")) {
            if (entry.is_regular_file() && entry.path().extension() == ext) {
                count++;
            }
        }
        if (count > 0)
            return "*" + std::string(ext);
    }

    return "src/*.cpp";
}

std::string Parser::Init::detectIncludes() {
    constexpr std::array<std::string_view, 4> candidates = {"include", "inc", "headers", "src"};
    for (const auto &dir : candidates) {
        if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
            return std::string(dir);
        }
    }
    return "";
}

std::string Parser::Init::defaultOutput() {
    std::string base = InitCfg.name.empty() ? "app" : InitCfg.name;
    if (InitCfg.compiler == "gcc" || InitCfg.compiler == "clang") {
        return "build/" + base + ".exe";
    }
    if (InitCfg.compiler == "zig") {
        return "build/" + base + ".exe";
    }
    return "build/" + base;
}

void Parser::Init::chooseLanguage() {
    constexpr std::string_view base = R"(   
   1) C++ (GCC)          g++ 
   2) C++ (Clang)        clang++  
   3) C   (GCC)          gcc  
   4) C   (Clang)        clang    
   5) Zig                zig   
   6) Custom

)";

    std::cout << base << "\n  " << hp::getColorCode(hp::CYAN)
              << "Choose language:" << hp::getColorCode(hp::RESET) << "\n"
              << hp::getColorCode(hp::CYAN) << "> " << hp::getColorCode(hp::RESET);

    int choice;
    std::cin >> choice;
    std::cin.ignore();

    switch (choice) {
        case 1:
            InitCfg.compiler = "g++";
            InitCfg.version = "std=c++20";
            break;
        case 2:
            InitCfg.compiler = "clang++";
            InitCfg.version = "std=c++20";
            break;
        case 3:
            InitCfg.compiler = "gcc";
            InitCfg.version = "std=c17";
            break;
        case 4:
            InitCfg.compiler = "clang";
            InitCfg.version = "std=c17";
            break;
        case 5:
            InitCfg.compiler = "zig";
            InitCfg.version = "";
            break;
        case 6:
            InitCfg.compiler = ask("Compiler", "gcc");
            InitCfg.version = ask("Version (empty for none)", "");
            break;
        default:
            hp::printlnCl("  Invalid choice. Try again.", hp::RED);
            hp::wait(1);
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            chooseLanguage();
    }
}

void Parser::Init::showPreview() {
    std::cout << "\n";
    hp::printlnCl("  Generated HelpMake.txt:", hp::CYAN);
    std::cout << "  " << std::string(45, '-') << "\n";

    if (!InitCfg.name.empty())
        std::cout << "  " << hp::getColorCode(hp::BRIGHT_BLACK) << "Name:     "
                  << hp::getColorCode(hp::RESET) << InitCfg.name << "\n";

    std::cout << "  " << hp::getColorCode(hp::BRIGHT_BLACK) << "Compiler: "
              << hp::getColorCode(hp::RESET) << InitCfg.compiler << "\n";

    if (!InitCfg.version.empty()) {
        std::cout << "  " << hp::getColorCode(hp::BRIGHT_BLACK) << "Version:  "
                  << hp::getColorCode(hp::RESET) << InitCfg.version << "\n";
    }

    std::cout << "  " << hp::getColorCode(hp::BRIGHT_BLACK) << "Output:   "
              << hp::getColorCode(hp::RESET) << InitCfg.output << "\n";

    std::cout << "  " << hp::getColorCode(hp::BRIGHT_BLACK) << "Inputs:   "
              << hp::getColorCode(hp::RESET) << InitCfg.inputFile << "\n";

    if (!InitCfg.includes.empty()) {
        std::cout << "  " << hp::getColorCode(hp::BRIGHT_BLACK) << "Includes: "
                  << hp::getColorCode(hp::RESET) << InitCfg.includes << "\n";
    }

    if (!InitCfg.flags.empty()) {
        std::cout << "  " << hp::getColorCode(hp::BRIGHT_BLACK) << "Flags:    "
                  << hp::getColorCode(hp::RESET) << InitCfg.flags << "\n";
    }
    std::cout << " " << hp::getColorCode(hp::BRIGHT_BLACK) << "  Generate compile_commands.json: "
              << hp::getColorCode(hp::RESET) << InitCfg.json
        ? "true\n"
        : "false\n";

    std::cout << "  " << std::string(45, '-') << "\n\n";
}

void Parser::Init::previewAndSave() {
    while (true) {
        showPreview();

        std::cout << "  " << hp::getColorCode(hp::CYAN) << "Actions: "
                  << hp::getColorCode(hp::BRIGHT_BLACK)
                  << "[S]ave  [E]dit  [C]ancel "
                  << hp::getColorCode(hp::RESET)
                  << hp::getColorCode(hp::CYAN) << "> "
                  << hp::getColorCode(hp::RESET);

        std::string action;
        std::getline(std::cin, action);

        char c = action.empty() ? 's' : action[0];

        if (c == 's' || c == 'S') {
            break;
        }
        if (c == 'c' || c == 'C') {
            hp::printlnCl("  Cancelled.", hp::YELLOW);
            return;
        }
        if (c == 'e' || c == 'E') {
            InitCfg.compiler = ask("Compiler", InitCfg.compiler);
            InitCfg.version = ask("Version", InitCfg.version);
            InitCfg.inputFile = ask("Input files", InitCfg.inputFile);
            InitCfg.output = ask("Output", InitCfg.output);
            InitCfg.includes = ask("Includes", InitCfg.includes);
            InitCfg.flags = ask("Flags", InitCfg.flags);
            InitCfg.json = askYesNo("Generate compile_commands.json?");
            continue;
        }
        hp::printlnCl("  Invalid action.", hp::RED);
    }

    InitCfg.create = true;
    Parser parser("HelpMake.txt", InitCfg);
    if (InitCfg.json)
        parser.execute();
    parser.parse();
}

void Parser::Init::run() {
    hp::printlnCl("\n  HelpMake Project Wizard", hp::CYAN);
    hp::printlnCl("  " + std::string(45, '-'), hp::BRIGHT_BLACK);
    std::cout << "\n";

    InitCfg.name = ask("Project name", "myproject");

    chooseLanguage();

    std::string detected = detectFiles();
    InitCfg.inputFile = ask("Input files", detected);
    InitCfg.output = ask("Output", defaultOutput());

    std::string detectedInc = detectIncludes();
    if (!detectedInc.empty()) {
        InitCfg.includes = ask("Includes", detectedInc);
    } else {
        InitCfg.includes = ask("Includes (empty for none)", "");
    }
    InitCfg.flags = ask("Flags", "-Wall");

    InitCfg.json = askYesNo("Generate compile_commands.json?");

    previewAndSave();
}