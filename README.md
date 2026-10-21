# HelpMake

**HelpMake (hm)** - a simple, lightweight, **language-agnostic** build system.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) [![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/) [![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-blue.svg)]()

## Features

- **Language-agnostic** - C, C++, Zig, Go, Assembly, or anything with a CLI compiler
- **Multiple built-in toolchains** - GCC, Clang, MSVC, Zig, Zig C++ (`zig c++`), Zig CC (`zig cc`), Go, Sere, plus any custom compiler via `-C=`
- **Precompiled headers** - first-class PCH support on GCC, Clang, MSVC, and Zig C/C++
- **Config blocks** - Define multiple build targets in one file
- **Dependency blocks** - `Dependency: name { }` fetches, builds, and links external libraries; opt in per-config with `Depends:`
- **Variables** - `Set { }` block + `[[name]]` expansion + `--var NAME=VALUE` CLI override
- **Static libraries** - `Kind: static` builds `.a` / `.lib` archives
- **C++20 modules** - GCC and Clang workflows, non-standard extensions supported
- **Incremental builds** - Only recompiles what changed, header tracking via `.d` files
- **Flag-change invalidation** - Changing `-O2` → `-O3` or switching compilers triggers an automatic clean rebuild
- **Per-compiler build isolation** - Each toolchain gets its own `obj/`, `modules/`, and `logs/` directory, so switching compilers never mixes object files
- **Parallel builds** - `-j N` compiles N files at once
- **Fetch dependencies** - `Fetch { }` clones git repos into `build/HelpMake/dep/`
- **Build file generators** - `hm --gen ninja` emits a `build.ninja` for editors, CI, and Ninja-native workflows
- **PreBuild / PostBuild** - Run commands before and after the build
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
```

## Installation

### Option A - clone and build manually:

```bash
git clone https://github.com/Gag91/Help_Make.git
cd Help_Make
g++ -std=c++20 src/cli/*.cpp src/executor/*.cpp src/fs/*.cpp src/generator/*.cpp src/getters/*.cpp src/init/*.cpp src/parser/*.cpp src/process/*.cpp src/toolchain/*.cpp src/main.cpp -o hm.exe -Iinclude
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

### Picking a toolchain

```bash
hm -b -Gcc             # GCC (g++ / gcc)
hm -b -Clang           # Clang (clang++ / clang)
hm -b -MSVC            # MSVC (cl.exe) - Windows only
hm -b -Zig             # Zig (build-exe)
hm -b -ZigCpp          # Zig in C++ mode (zig c++)
hm -b -ZigCC           # Zig in C mode  (zig cc)
hm -b -Go              # Go (go build)
hm -b -Sere            # Sere compiler
```

Or spell it out:

```bash
hm -b -C=zig-c++       # Same as -ZigCpp
hm -b -C="zig cc"      # Same as -ZigCC
hm -b -C=nasm          # Any compiler on PATH
```

### Variables

```bash
hm -b --var CXX=clang++ --var BUILD_TYPE=release
```

Overrides any `Set { }` entry of the same name. Useful for CI so you never edit `HelpMake.txt` per-platform.

### Static libraries

```bash
hm -b --Kind=static -o build/libmath.a
```

See the Static Libraries section below.

### Generate a `build.ninja`

```bash
hm --gen ninja
ninja
```

Emits `build.ninja` in the project root using the same compiler and flags that `hm -b` would use. Useful for editors, CI pipelines, and anyone who wants Ninja's speed without changing their `HelpMake.txt`.

Not all features are supported by the Ninja generator — PCH, `PreBuild`/`PostBuild`, `Copy`, `Fetch`, MSVC, and one-shot toolchains (Zig, Go) are warned and skipped. Use `hm -b` for the full feature set.

## CLI Reference

Run `hm --help` for the full list. Short summary:

```text
Options:
  -h, --help               Show this message
  -v, --version            Show version
  -b, --build              Build using HelpMake.txt
  -l, --license            Show license
  -f, --file               Use a custom config file
  -j, --jobs N             Compile N files in parallel (default: all cores)
  --show                   Show current configuration
  --dump                   Print raw HelpMake.txt
  --create                 Generate HelpMake.txt from args
  --gen <name>             Emit a build file for another tool (e.g. --gen ninja)
  --init                   Interactive project wizard
  --install                Install HelpMake to PATH
  --clean                  Delete build/HelpMake
  --clean-gcm              Delete gcm.cache
  --clean-all              Delete both

Build Options:
  -Gcc, -Clang, -MSVC,
  -Zig, -ZigCpp, -ZigCC,
  -Go, -Sere               Pick a built-in toolchain
  -C=<compiler>            Use any compiler (gcc, clang, msvc, zig, "zig c++", "zig cc", go, nasm...)
  -V=<version>             Set the standard (e.g. -V=std=c++20)
  --Kind=<kind>            Target kind: executable (default), static
  --var NAME=VALUE         Override a Set { } variable

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
Version: std=c++23    # Any standard
Output: build/app     # Output path
Kind: static          # executable (default) or static
Run: true             # Run the output after building
PCH: include/pch.hpp  # Precompiled header (optional)
Jobs: 8               # Parallel jobs (default: all cores)
Depends: raylib, lua  # Comma-separated dependency names
```

Inline forms also work:

```txt
Flags: -Wall -O2
Includes: include external/glfw/include
Depends: raylib, lua
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

Copy {
    assets -> build/assets
    config/*.json -> build/config
}

PreBuild {
    python3 tools/gen_version.py
    mkdir -p build/assets
}

PostBuild {
    strip build/app
}
```

*Note: `#` starts a comment. Lines inside blocks are one item per line. Inline forms like `Flags: -Wall -O2` are equivalent.*

### `Dependency: name { }`

Declare an external library once at the root, opt in per-config with `Depends:`.

```txt
Dependency: raylib {
    Git: https://github.com/raysan5/raylib
    Tag: 5.5
    Build: cmake -B build -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=OFF
    Build: cmake --build build --config Release
    Include: src
    Link: build/raylib/libraylib.a
    LinkFlags: -lm -lpthread -lwinmm -lgdi32 -lopengl32
}

Config: app {
    Depends: raylib
    InputFiles { src/main.c }
}
```

`hm -b --config app` clones raylib, runs the `Build:` commands, adds `-Ibuild/HelpMake/dep/raylib/src` to the compile command, and `libraylib.a` + the `LinkFlags` to the link command.

**Source** (pick one): `Git:` (clone), `Url:` (tarball/zip), `Path:` (local dir)
**Version pins** (Git only): `Tag:`, `Commit:`, `Hash:`
**Build** (repeatable): `Build:` — shell command run inside the dep directory
**Integration** (repeatable): `Include:`, `Link:`, `LinkDir:`, `LinkFlags:`

Cache lives at `build/HelpMake/dep/<name>/`. Delete the dir to force a re-fetch.

`Dependency:` blocks are **root-level only** — they cannot appear inside a `Config: { }`. Use `Depends:` on the config to opt in.

`Fetch { }` is a simpler alternative for header-only libraries that need no build step.

### Variables

Define reusable values with `Set { }`, expand with `[[name]]`:

```txt
Set {
    CXX: g++
    CXX_STD: std=c++20
    OUT_DIR: build
    OPT: -O2
}

Compiler: [[CXX]]
Version: std=[[CXX_STD]]
Output: [[OUT_DIR]]/app
Flags {
    -Wall
    [[OPT]]
}
```

Override from the CLI (works with CI, per-platform scripts, etc.):

```bash
hm -b --var CXX=clang++ --var OPT=-O3
```

Any name defined on the command line takes priority over the file. Undefined variables are reported as errors with `file:line`.

Escape a literal `[[` by writing `[[[[`.

### Precompiled Headers (PCH)

Point `PCH:` at a header that every source includes. HelpMake compiles it once, caches it, and reuses it until either the header or a transitive dependency changes:

```txt
Compiler: g++
Version: std=c++20
Output: build/app
PCH: include/pch.hpp

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

The cache lives at `build/HelpMake/headers/<compiler>/` and is invalidated when:

- The PCH source header changes
- Any header included by the PCH source changes
- `--rebuild` is passed
- The compiler or its flags change

### Supported toolchains

| Toolchain | Binary | PCH ext | Modules | Static libs | Separate compile |
| :--- | :--- | :--- | :---: | :---: | :---: |
| GCC | `g++` / `gcc` | `.gch` | ✅ | ✅ `ar` | ✅ |
| Clang | `clang++` / `clang` | `.pch` | ✅ | ✅ `ar` | ✅ |
| MSVC | `cl.exe` | `.pch` | ❌ | ✅ `lib.exe` | ✅ |
| Zig | `zig` | - | ❌ | ❌ | ❌ |
| Zig C++ | `zig c++` | `.pch` | ❌ | ✅ `ar` | ✅ |
| Zig CC | `zig cc` | `.pch` | ❌ | ✅ `ar` | ✅ |
| Go | `go` | - | ❌ | ❌ | ❌ |
| Generic | any | - | ❌ | ✅ `ar` | ✅ |

PCH mechanism per toolchain:

| Toolchain | PCH injection | Notes |
| :--- | :--- | :--- |
| GCC | `-include <header>` | GCC finds `.gch` next to the header |
| Clang | `-include-pch <pch>` | Uses the PCH binary path |
| MSVC | `/Yc` + `/Yu` | Source-file-based, requires the header to be first `#include` |
| Zig C++ / Zig CC | `-include-pch <pch>` | Same as Clang |
| Zig / Go | - | Not supported (one-shot toolchains) |

## Requirements

- A C++20 compiler to build HelpMake itself (GCC 11+, Clang 11+, MSVC 19.30+)
- The compiler for your project (GCC, Clang, MSVC, Zig, Go, or any CLI compiler)
- `ar` (GCC/Clang) or `lib.exe` (MSVC) for static libraries
- `git` — only needed for `Dependency: { Git: … }` and `Fetch { }`
- `curl` or `wget` + `tar` / `unzip` — only needed for `Dependency: { Url: … }`

## License

MIT - see [LICENSE](LICENSE) for details.

## Contributing

Feel free to open issues or PRs. Bugs, feature requests, and docs improvements are all welcome.

Full documentation lives in [docs/](docs/README.md).

## Contact

- **Discord:** xxavi640
