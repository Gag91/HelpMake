# HelpMake

**HelpMake (hm)** - a simple, lightweight, **language-agnostic** build system.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) [![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/) [![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-blue.svg)]()

## Features

- **Language-agnostic** - C, C++, Zig, Rust, Assembly, anything with a CLI compiler
- **Any compiler** - `gcc`, `clang`, `msvc`, `zig`, `sere`, or any custom compiler
- **Config blocks** - Define multiple build targets in one file
- **Static libraries** - `Kind: static` builds `.a` archives
- **C++20 modules** - GCC and Clang workflows, non-standard extensions supported
- **Incremental builds** - Only recompiles what changed
- **Header tracking** - Rebuilds when headers change, via `.d` files
- **Parallel builds** - `-j N` compiles N files at once
- **Fetch dependencies** - `Fetch { }` clones git repos into `build/HelpMake/dep/`
- **PreBuild / PostBuild** - Run commands before and after the build
- **Per-config arguments** - `Arguments { }` toggles flags per config
- **`compile_commands.json`** - IDE / clangd integration
- **Color-coded logs** - Raw + clean logs, per-file timings
- **Typo detection** - Suggests the correct key when you misspell one
- **Cross-platform** - Windows and Linux
- **Self-hosting** - Builds itself with itself

## Quick Start

**1. Create `HelpMake.txt`:**

```txt
Compiler: g++
Version: std=c++20
Output: build/app.exe
InputFiles {
    src/*.cpp
}
Includes {
    include
}
Flags {
    -Wall
    -O2
}
```

**2. Build:**

```bash
hm -b
# Or hm --build for the long form.
```

## Installation

### Option A - clone and build manually:

```bash
git clone https://github.com/Gag91/Help_Make.git
cd Help_Make
g++ -std=c++20 src/*.cpp -o hm.exe -Iinclude
```

Then add the folder to PATH:

```bash
# Windows
setx PATH "%PATH%;C:\path\to\Help_Make"

# Linux / MSYS2
cp hm ~/.local/bin/
```

### Option B - use the built-in installer:

```bash
hm --install
```

This clones HelpMake, builds it, and adds it to PATH automatically. Restart your terminal afterwards.

## Usage

### Basic commands

```bash
hm -h                  # Show help
hm -v                  # Show version
hm -l                  # Show license
hm -b                  # Build using HelpMake.txt
hm -b --config debug   # Build a specific config
hm -b --config-all     # Build every config
hm -b --rebuild        # Force full rebuild
hm -b --json           # Generate compile_commands.json
hm -b --verbose        # Show commands and per-file timings
hm -b --debug          # Show parser trace and command lines
hm --clean             # Delete build/HelpMake
hm --show              # Print parsed config
hm --dump              # Print raw HelpMake.txt
hm --init              # Interactive project wizard
```

### Any compiler, any language

```bash
hm -b -C=zig                    # Use Zig
hm -b -C=nasm                   # Use NASM
hm -b -V=std=c99                # Any standard
hm -b --nofile -C=gcc main.c    # Build without a config file
```

### Static libraries

```bash
hm -b --Kind=static -o build/libmath.a
```

See the [Static Libraries](#static libraries) section below.

## CLI Reference

Run `hm --help` for the full list. Short summary:

```text
Options:
  -h, --help               Show this message
  -v, --version            Show version
  -b, --build              Build using HelpMake.txt
  -l, --license            Show license
  -f, --file               Use a custom config file
  -j, --jobs N   Compile N files in parallel (default: all cores)
  --show                   Show current configuration
  --dump                   Print raw HelpMake.txt
  --create                 Generate HelpMake.txt from args
  --init                   Interactive project wizard
  --install                Install HelpMake to PATH
  --clean                  Delete build/HelpMake
  --clean-gcm              Delete gcm.cache
  --clean-all              Delete both

Build Options:
  -Gcc, -Clang, -MSVC, -Zig, -Sere  Pick a compiler
  -C=<compiler>            Use any compiler
  -V=<version>             Set the standard
  --Kind=<kind>            Target kind: executable (default), static

Build Flags:
  -I<path>                 Include directory
  -L<path>                 Library directory
  -F<flags>                Additional compiler flags
  -o<file>                 Output filename
  -r, --run                Run output after build
  --config                 Build a specific config
  --config-all             Build every config
  --nofile                 Build from CLI only
  --verbose                Show commands and timings
  --debug                  Show parser trace
  --rebuild                Force full rebuild
  --json                   Generate compile_commands.json
  --no-sep                 One-shot compile (no incremental)
  --notify                 Desktop notification when done
  --quiet                  Suppress warnings
```

## HelpMake.txt Reference

### Scalar fields

```txt
Compiler: g++         # Any compiler name or path
Version: std=c++23     # Any standard
Output: build/app      # Output path
Kind: static           # executable (default) or static
Run: true              # Run the output after building
```

Inline forms also work:

```txt
Flags: -Wall -O2
Includes: include external/glfw/include
```

### Blocks

```txt
InputFiles {
    src/*.cpp
}

Includes {
    include
    external/glfw/include
}

Flags {
    -Wall
    -O2
}

Modules {
    math.cppm
    graphics.ixx
}

Fetch {
    https://github.com/user/repo.git -> include/repo
}

PreBuild {
    python3 tools/gen_version.py
    mkdir -p build/assets
}

PostBuild {
    strip build/app
}
```

`#` starts a comment. Lines inside blocks are one item per line.

### Configs

Multiple build targets in one file:

```txt
Compiler: g++
Version: std=c++20
Output: build/app.exe
InputFiles {
    src/*.cpp
}

Config: debug {
    Flags: -g -O0 -DDEBUG
    Output: build/debug.exe
}

Config: release {
    Compiler: clang
    Flags: -O3 -DNDEBUG -flto
    Output: build/release.exe
}
```

Build them:

```bash
hm -b --config debug
hm -b --config release
hm -b --config debug --config release  # both
hm -b --config-all                    # every config
```

Each config inherits from the root and overrides only what it sets.

### Arguments

Per-config boolean flags:

```txt
Config: dev {
    Output: build/dev.exe
    Arguments {
        verbose
        debug
        run
    }
}
```

Available: `verbose`, `debug`, `run`, `time`, `json`, `rebuild`, `notify`, `quiet`.

### Static Libraries

Set `Kind: static` to build an archive instead of an executable:

```txt
Compiler: g++
Version: std=c++20
Kind: static
Output: build/libmath.a
InputFiles {
    src/math/*.cpp
}
```

HelpMake runs `ar rcs` (GCC/Clang) to produce the static library. To link a consumer:

```txt
Config: lib {
    Output: app.exe
    InputFiles {
        src/main.cpp
    }
    Flags {
        -Lbuild
        -lmath
    }
}
```

HelpMake places `-l` flags after the object files automatically, so the link order is always correct.

### C++20 Modules

List module interface units under `Modules { }`:

```txt
Compiler: clang++
Version: std=c++20
Output: build/app.exe
InputFiles {
    src/main.cpp
}
Modules {
    src/math.cppm
    src/graphics.ixx
}
```

HelpMake knows the difference between compilers:
- **GCC** - one `-c` step produces both `.o` and `gcm.cache/<name>.gcm`
- **Clang** - `--precompile` produces `.pcm`, then `-c` produces `.o`

Modules compile before regular sources, so consumers always find the precompiled interface. Non-standard extensions like `.module` work: HelpMake adds `-xc++-module` for Clang and `-xc++` for GCC automatically.

## Requirements

- C++20 compiler
- GCC 11+ or Clang 11+ for module support
- `ar` (usually installed alongside GCC/Clang) for static libraries

## VS Code Extension

Syntax highlighting is available on the VS Code Marketplace. Search for "HelpMake".

## License

MIT - see LICENSE for details.

## Contributing

Feel free to open issues or PRs. Bugs, feature requests, docs improvements all welcome.

## Contact

- **Discord:** xxavi640
