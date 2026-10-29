#include "hm/cli/cli.hpp"

#include <iostream>
#include <string_view>

namespace hm::cli {

    void help() {
        constexpr std::string_view help = R"HELP(Help_Make (hm) - Build System
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
  --update-lock  Resolve Git dependencies and write HelpMake.lock
  --gen ninja    Generates Ninja File: build.ninja

Build Options (use with -b):
  -Gcc           Use GNU Compiler Collection
  -Clang         Use LLVM Clang Compiler
  -MSVC          Use Microsoft Visual C++ Compiler, Windows only
  -Zig           Use Zig Compiler (C/C++ support)
  -ZigCC         Use Zig Compiler (C mode, 'zig cc')
  -ZigCpp        Use Zig Compiler (C++ mode, 'zig c++')
  -Sere          Use Sere Compiler
  -Go            Use Go Compiler ('go build')
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
      --dry-run  Show build commands without running them
      --rebuild  Force full rebuild (ignore timestamps)
      --frozen   Require Git revisions from HelpMake.lock
      --json     Generate compile_commands.json
      --no-sep   Disable incremental build (one-shot compile)
      --notify   Desktop notification when the build finishes
      --quiet    Suppress warnings
      --clean    Clean build (delete build/HelpMake directory)
)HELP";

        std::cout << help << "\n";
    }

} // namespace hm::cli