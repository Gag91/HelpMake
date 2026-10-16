#include "hp/borderStyle/border.hpp"
#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"
#include "parser.hpp"
#include <filesystem>
#include <format>
#include <fstream>
#include <thread>

class Build {
  public:
    Build() = default;
    void help() const {
        constexpr std::string_view help = R"(Help_Make (hm) - Build System
Use : hm [options]

Options:
  -h, --help     Output this help message
  -v, --version  Output the program version
  -b, --build    Build using HelpMake.txt
  -l, --license  Output the license information
  -f, --file     Specify a custom HelpMake file
      --show     Show current configuration
      --dump     Print raw HelpMake.txt contents
      --create   Generate HelpMake.txt from build arguments
      --clean    Delete build artifacts

Build Options (use with -b):
  -Gcc           Use GNU Compiler Collection
  -Clang         Use LLVM Clang Compiler
  -MSVC          Use Microsoft Visual C++ Compiler, Windows only
  -Zig           Use Zig Compiler (C/C++ support)
  -Sere          Use Sere Compiler
  -C=<compiler>  Use any compiler (e.g., -C=gcc, -C=python)
  -V=<version>   Use any version (e.g., -V=std=c99)
  --Kind=<kind>  Set target kind: executable (default), static

Build Flags:
  -I<path>       Include a specified directory
  -L<path>       Link a specified library path
  -F<flags>      Add specified compiler flags
  -o<file>       Set output filename
  -r, --run      Run output file after compilation
  -j, --jobs N   Compile N files in parallel (default: all cores)
      --config   Build a specific config from HelpMake.txt
      --nofile   Build from command line only (no config file)
      --verbose  Show detailed build output
      --debug    Show debug information
      --rebuild  Force full rebuild (ignore timestamps)
      --json     Generate compile_commands.json
      --no-sep   Disable incremental build (one-shot compile)
      --notify   Desktop notification when the build finishes
      --quiet    Suppress warnings
      --clean    Clean build (delete build/HelpMake directory)

Config Blocks:
  Define multiple build configs in HelpMake.txt:
    Config: debug {
        Version: std=c++20
        Flags:   -g -O0 -DDEBUG
        Output:  build/debug.exe
    }
    Config: release {
        Flags:   -O3 -DNDEBUG
        Output:  build/release.exe
    }
  Then build with:
    hm -b --config debug
    hm -b --config release

Static Libraries:
  Set Kind: static in HelpMake.txt to build a static library:
    Kind:   static
    Output: build/libmath.a
  Link against it from another config:
    Config: app {
        Output: build/app.exe
        InputFiles { src/main.cpp }
        Flags {
            -Lbuild
            -lmath
        }
    }

Examples:
  Build from HelpMake.txt
    hm -b

  Build with a specific compiler
    hm -b -clang -o main.exe

  Build a config
    hm -b --config release

  Build a static library
    hm -b --Kind=static -o build/libmath.a

  Build from command line (no config file)
    hm -b --nofile -gcc main.cpp -o main.exe

  Build C code (any language works)
    hm -b --nofile -C=gcc -V=std=c99 main.c -o main.exe

  Generate a HelpMake.txt from arguments
    hm -b -Gcc -std=c++26 -F-Wall -Iinclude src/main.cpp -o main.exe --create

  Use a custom config file
    hm -b -f custom.txt

  Clean build artifacts
    hm --clean

  Show current config / raw file
    hm --show
    hm --dump
)";

        std::cout << help << "\n";
    }

    void version() const {
        constexpr std::string_view version = R"(Help_Make (hm) - Build System
Version: 8.0.0
Author : Xavi99
Website: https://github.com/Gag91/Help_Make
This program is licensed under the MIT License. See "--license" for details.
)";
        std::cout << version << "\n";
    }
    void license(hp::BorderStyle style) const {
        hp::BorderChars chars = hp::getBorderChars(style);
        std::cout << "Help_Make (hm) - License Information\n";
        constexpr std::string_view license = R"(MIT License

Copyright (c) 2026 Xavi99

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.)";
        std::cout << license << "\n";
    }
};

static void printList(const std::string &label, const std::vector<std::string> &items, int indent = 4) {
    if (items.empty())
        return;

    std::string pad(indent, ' ');
    std::cout << pad << label << ":\n";
    for (const auto &item : items)
        std::cout << pad << "    " << item << "\n";
    std::cout << "\n";
}

int main(int argc, char *argv[]) {
    hp::enableUTF8();

    Config config;
    std::filesystem::path filename = "HelpMake.txt";

    std::vector<std::string> selectedConfigs;
    Build build;

    if (argc < 2) {
        build.help();
        return 0;
    }

    std::string_view argument = argv[1];

    if (argument == "--help" || argument == "-h") {
        build.help();
        return 0;
    } else if (argument == "--install" || argument == "install") {
        Parser parser(filename, config);
        parser.install();
        return 0;
    } else if (argument == "--version" || argument == "-v") {
        build.version();
        return 0;
    } else if (argument == "--license" || argument == "-l") {
        build.license(hp::EXTENDED);
        return 0;
    } else if (argument == "--dump") {
        if (std::filesystem::exists(filename)) {
            std::ifstream file(filename);
            std::cout << file.rdbuf() << "\n";
        }
    } else if (argument == "--clean") {
        std::filesystem::remove_all("build/HelpMake");
        hp::printlnCl("[HelpMake] Cleaned build/HelpMake", hp::Color::GREEN);
    } else if (argument == "--clean-gcm") {
        std::filesystem::remove_all("gcm.cache");
        hp::printlnCl("[HelpMake] Cleaned gcm.cache", hp::Color::GREEN);
    } else if (argument == "--clean-all") {
        std::filesystem::remove_all("build/HelpMake");
        std::filesystem::remove_all("gcm.cache");
        hp::printlnCl("[HelpMake] Cleaned build/HelpMake and gcm.cache", hp::Color::GREEN);
    } else if (argument == "--init") {
        Parser::Init init;
        init.run();
        return 0;
    } else if (argument == "--show") {
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
            return 0;

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

        return 0;
    } else if (argument == "--build" || argument == "-b") {
        for (int i = 2; i < argc; i++) {
            std::string_view arg = argv[i];

            if (arg == "-Gcc" || arg == "-gcc") {
                config.compiler = "gcc";
            } else if (arg == "-Clang" || arg == "-clang") {
                config.compiler = "clang";
            } else if (arg == "-MSVC" || arg == "-msvc") {
                config.compiler = "msvc";
            } else if (arg == "-Zig" || arg == "-zig") {
                config.compiler = "zig";
            } else if (arg == "-sere" || arg == "-Sere") {
                config.compiler = "sere";
                config.buildSere = true;

            } else if (arg.rfind("-C=", 0) == 0) {
                std::string value = std::string(arg).substr(3);
                if (value.empty()) {
                    if (i + 1 < argc && argv[i + 1][0] != '-')
                        value = argv[++i];
                    else {
                        hp::printlnCl(std::format("[HelpMake] Error: Compiler not specified after '-C='."), hp::Color::RED);
                        exit(EXIT_FAILURE);
                    }
                }
                config.compiler = value;
                if (config.debug)
                    hp::printlnCl(std::format("[Debug] Special compiler: {}", config.compiler), hp::Color::YELLOW);

            } else if (arg.rfind("-V=", 0) == 0) {
                std::string value = std::string(arg).substr(3);
                if (value.empty()) {
                    if (i + 1 < argc && argv[i + 1][0] != '-')
                        value = argv[++i];
                    else {
                        hp::printlnCl(std::format("[HelpMake] Error: Version not specified after '-V='."), hp::Color::RED);
                        exit(EXIT_FAILURE);
                    }
                }
                if (value.front() != '-') {
                    value.insert(value.begin(), '-');
                }
                config.version = value;
                if (config.debug)
                    hp::printlnCl(std::format("[Debug] Special version: {}", config.version), hp::Color::YELLOW);
            } else if (arg == "-o" || arg == "--output") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    config.output = argv[++i];
                } else {
                    hp::printlnCl("[HelpMake] Error: Output file not specified after '" + std::string(arg) + "'.", hp::Color::RED);
                    exit(EXIT_FAILURE);
                }

            } else if (arg.rfind("-o", 0) == 0 && arg.size() > 2) {
                config.output = std::string(arg).substr(2);

            } else if (arg == "-F" || arg == "--Flag") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    if (!config.v_Flags.empty())
                        config.v_Flags.push_back(" ");
                    config.v_Flags.push_back(argv[++i]);
                } else {
                    hp::printlnCl("[HelpMake] Error: Additional flags not specified after '" + std::string(arg) + "'.", hp::Color::RED);
                    exit(EXIT_FAILURE);
                }

            } else if (arg.rfind("-F", 0) == 0 && arg.size() > 2) {
                std::string n_flags = std::string(arg).substr(2);
                if (!config.v_Flags.empty())
                    config.flags += " ";
                config.flags += n_flags;

            } else if (arg == "-I" || arg == "--Include") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    if (!config.v_Flags.empty())
                        config.flags += " ";
                    config.flags += "-I" + std::string(argv[++i]);
                } else {
                    hp::printlnCl("[HelpMake] Error: Include path not specified after '" + std::string(arg) + "'.", hp::Color::RED);
                    exit(EXIT_FAILURE);
                }

            } else if (arg.rfind("-I", 0) == 0 && arg.size() > 2) {
                std::string includePath = std::string(arg).substr(2);
                if (!config.v_Flags.empty())
                    config.flags += " ";
                config.flags += "-I" + includePath;

            } else if (arg == "-L" || arg == "--Library") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    if (!config.v_Flags.empty())
                        config.flags += " ";
                    config.flags += "-L" + std::string(argv[++i]);
                } else {
                    hp::printlnCl("[HelpMake] Error: Library path not specified after '" + std::string(arg) + "'.", hp::Color::RED);
                    exit(EXIT_FAILURE);
                }

            } else if (arg.rfind("-L", 0) == 0 && arg.size() > 2) {
                std::string libPath = std::string(arg).substr(2);
                if (!config.v_Flags.empty())
                    config.flags += " ";
                config.flags += "-L" + libPath;

            } else if (arg == "-f" || arg == "--file") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    filename = std::string(argv[++i]);
                } else {
                    hp::printlnCl("[HelpMake] Error: filename not specified after '" + std::string(arg) + "'.", hp::Color::RED);
                    exit(EXIT_FAILURE);
                }

            } else if (arg == "-j" || arg == "--jobs") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    try {
                        config.jobs = std::stoul(argv[++i]);
                    } catch (...) {
                        hp::printlnCl(std::format("Error: Invalid value for '{}'", arg), hp::Color::RED);
                        exit(EXIT_FAILURE);
                    }
                } else {
                    config.jobs = std::thread::hardware_concurrency();
                    if (config.jobs == 0)
                        config.jobs = 1;
                }
            } else if (arg.rfind("-j", 0) == 0 && arg.size() > 2) {
                try {
                    config.jobs = std::stoul(std::string(arg).substr(2));
                } catch (...) {
                    hp::printlnCl(std::format("Error: Invalid value for '-j{}'", std::string(arg).substr(2)), hp::Color::RED);
                    exit(EXIT_FAILURE);
                }
            } else if (arg == "--config") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    selectedConfigs.push_back(std::string(argv[++i]));
                } else {
                    hp::printlnCl("[HelpMake] Error: Config name not specified after '" + std::string(arg) + "'.", hp::Color::RED);
                    exit(EXIT_FAILURE);
                }
            } else if (arg.rfind("--config=", 0) == 0 && arg.size() > 9) {
                std::string configName = std::string(arg).substr(9);
                selectedConfigs.push_back(configName);
            } else if (arg == "--Kind") {
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    config.kind = to_lower(argv[++i]);
                } else {
                    hp::printlnCl(std::format("[HelpMake] Error: Kind not specified after '{}'.", arg), hp::Color::RED);
                    exit(EXIT_FAILURE);
                }
            } else if (arg.rfind("--Kind=", 0) == 0 && arg.size() > 9) {
                config.kind = to_lower(arg.substr(7));
            } else if (arg.rfind("--config-all", 0) == 0 && arg.size() > 9) {
                config.allCfgs = true;
            } else if (arg.rfind("-f", 0) == 0 && arg.size() > 2) {
                filename = std::string(arg).substr(2);
            } else if (arg == "-v" || arg == "--verbose") {
                config.verbose = true;
            } else if (arg == "-r" || arg == "--run") {
                config.run = true;
            } else if (arg == "--debug") {
                config.debug = true;
            } else if (arg == "--nofile") {
                config.n_file = true;
            } else if (arg == "--create") {
                config.create = true;
            } else if (arg == "--json") {
                config.json = true;
            } else if (arg == "--no-sep") {
                config.seperate = false;
            } else if (arg == "--rebuild") {
                config.rebuild = true;
            } else if (arg == "--quiet") {
                config.quiet = true;
            } else if (arg == "--notify") {
                config.notify = true;
            } else if (arg == "--time") {
                config.time = true;
            } else if (!arg.empty() && (arg.back() == '/' || arg.back() == '\\')) {
                filename = std::string(arg) + "HelpMake.txt";
                if (config.verbose)
                    std::cout << "Build file: " << filename << std::endl;
            } else if (arg.find(".txt") != std::string::npos && std::filesystem::is_regular_file(arg)) {
                filename = std::string(arg);
                if (config.verbose)
                    std::cout << "Build file: " << filename << std::endl;
            } else if (std::filesystem::is_directory(arg)) {
                filename = std::string(arg) + "/HelpMake.txt";
                if (config.verbose)
                    std::cout << "Build file: " << filename << std::endl;
            } else if (arg.rfind("-", 0) != 0) {
                if (!config.inputFile.empty())
                    config.inputFile += " ";
                config.inputFile += std::string(arg);
            }
        }

        if (!std::filesystem::exists(filename) && !config.n_file && !config.create) {
            if (filename != "HelpMake.txt" && std::filesystem::exists("HelpMake.txt")) {
                hp::printlnCl("Build file not found. Using default HelpMake.txt", hp::Color::YELLOW);
                filename = "HelpMake.txt";
            }
        }

        if (std::filesystem::exists(filename) && !config.n_file && !config.create) {
            Parser parser(filename, config);
            parser.parse();
            if (!config.create) {
                parser.executeConfigs(selectedConfigs);
                if (!selectedConfigs.empty() && config.allCfgs && !config.quiet) {
                    hp::printlnCl("Warning: Both --config and --config-all specified. Ignoring --config.", hp::Color::YELLOW);
                }
                if (selectedConfigs.empty()) {
                    parser.execute();
                }
            }
        } else {
            if (!config.n_file && !config.create) {
                hp::printlnCl("Build file not found: " + filename.string(), hp::Color::YELLOW);
            }
            if (config.verbose)
                hp::printlnCl("Building from command-line arguments only\n", hp::Color::CYAN);

            if (config.compiler.empty()) {
                hp::printlnCl("[HelpMake] Error: No compiler specified. Use -Gcc, -Clang, -MSVC, -Zig, -Sere or -C=<compiler>", hp::Color::RED);
                exit(EXIT_FAILURE);
            }

            if (config.inputFile.empty()) {
                hp::printlnCl("[HelpMake] Error: No input files specified.", hp::Color::RED);
                exit(EXIT_FAILURE);
            }

            if (config.output.empty()) {
                std::string d_output;
#ifdef _WIN32
                d_output = "a.exe";
                config.output = "a.exe";
#else
                d_output = "a.out";
                config.output = "a.out";
#endif
                if (!config.quiet)
                    hp::printlnCl("Warning: No output file specified. Using default: " + d_output, hp::Color::YELLOW);
            }

            Parser parser(filename, config);
            parser.parse();
            if (!config.create)
                parser.execute();
        }
    } else {
        hp::printlnCl("[HelpMake] Error: Invalid command. Use \"--help\" for available commands.", hp::Color::RED);
    }

    return 0;
}
