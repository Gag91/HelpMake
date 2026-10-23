# HelpMake.txt reference

The complete specification of the `HelpMake.txt` file format.

- **`#`** starts a comment; everything after it on the line is ignored
- Keys are case-sensitive
- Both inline and block forms are accepted for list fields

## File structure

A `HelpMake.txt` file contains, in any order:

1. **Scalar fields** - `Compiler:`, `Version:`, `Output:`, etc.
2. **Blocks** - `InputFiles { }`, `Flags { }`, etc.
3. **Config blocks** - named subtargets that inherit from the root
4. **`Set { }` blocks** - variable definitions
5. **Dependency blocks** - external libraries to fetch and build (root-only)

Anything outside a block is parsed as a root-level field. Anything inside a `Config: name { }` block overrides the root for that config only.

## Scalar fields

| Field | Type | Default | Description |
|---|---|---|---|
| `Compiler:` | string | *(required)* | Any compiler name or path. See [Toolchains](04-toolchains.md). |
| `Version:` | string | *(empty)* | Standard flag passed as `-<value>`. Example: `std=c++20` becomes `-std=c++20`. |
| `Output:` | string | `a.exe` on Windows, `a.out` elsewhere | Path to the final binary or archive. |
| `Kind:` | `executable` or `static` | `executable` | See [Static libraries](08-static-libs.md). |
| `Run:` | bool | `false` | If true, run the binary after a successful build. |
| `PCH:` | string | *(empty)* | Path to a precompiled header. See [PCH](06-pch.md). |
| `Jobs:` | integer | `0` (auto) | Number of parallel compile jobs. `0` uses `std::thread::hardware_concurrency()`. |
| `Depends:` | list | *(empty)* | Comma-separated dependency names. Pulls in `Dependency: <name> { }` blocks. See [Dependencies](#dependency-blocks). |

Boolean fields can be set with `true` / `false`.

### Inline vs block

For list fields, the two forms are equivalent:

```txt
# Inline
Flags: -Wall -O2
Includes: include external/glfw/include

# Block (one item per line)
Flags {
    -Wall
    -O2
}
Includes {
    include
    external/glfw/include
}
```
Use the block form when you have more than two items, or when you want to add a comment next to each item.

### Platform Conditionals

Any line in a scalar field, block, or `Set { }` can be prefixed with a platform tag. If the tag matches the current platform at build time, the tag is stripped and the line is processed normally. Otherwise, the line is skipped entirely.

```txt
Flags {
    -Wall
    win: /W4
    linux: -Wextra -fPIC
    mac: -Wextra
    x64: -march=native
    !unix: -DWINDOWS_ONLY
}
```

#### Recognized Tags

| Tag | Matches |
| :--- | :--- |
| `win`, `windows` | Windows |
| `linux` | Linux |
| `mac`, `darwin`, `macos`, `osx` | macOS |
| `unix` | Any non-Windows platform |
| `x64`, `x86_64`, `amd64` | 64-bit x86 |
| `arm64`, `aarch64` | ARM64 |
| `x86`, `i386`, `i686` | 32-bit x86 |
| `!<tag>` | Negation (e.g., `!win:` means "anything but Windows") |

#### Supported Features

Platform tags work on:
* **Scalar fields:** `win: Output: build/app.exe`  `linux: Output: build/app`
* **Block entries:** `win: /W4` inside `Flags { }`
* **Set { } variables:** `win: PATH_SEP: `  `unix: PATH_SEP: `
* **Config: name { } blocks:** Any line inside them

#### Limitations & Details

* **Not supported:** Nested tags (e.g., `win: x64: ...`) and compound conditions (e.g., `win & x64:`).
* **Cross-compilation:** Tags check the platform executing `hm`, not the target triple. `hm --target=...` does not exist yet.

### Blocks
`InputFiles { }`
List of source files or glob patterns to compile.

```txt
InputFiles {
    src/*.cpp
    src/**/*.cc
    lib/vendor/*.c
}
```
## Glob rules

### Pattern	Matches
- src/*.cpp	Every .cpp directly under src/
- src/**/*.cpp	Every .cpp recursively under src/
- *.cpp	Every .cpp in the current directory
- src/main.cpp	That exact file
- ** is HelpMake's own recursive marker - it does not rely on shell globbing, so it works on Windows cmd, PowerShell, and bash identically.

Files are deduplicated automatically.
The file extensions are not restricted - anything matched by the glob is passed to the compiler as-is.

`Includes { }`
Directories added to the include search path.

```txt
Includes {
    include
    external/glfw/include
    .
}
```
Each entry is translated to -I<path> (or /I<path> on MSVC) in every compile command. The . entry adds the current directory.

`Flags { }`
Additional flags passed to both compile and link commands.

```txt
Flags {
    -Wall
    -Wextra
    -O2
    -DNDEBUG
}
```
Flags starting with -l or /l are separated out and only passed to the link step, so link order is always correct.

`Modules { }`
C++20 module interface units. See Modules for full details.

```txt
Modules {
    src/math.cppm
    src/graphics.ixx
}
```
Supported extensions: .cppm, .ixx, .mpp, .cxxm, .c++m, .ccm, and any file listed under Modules { } (HelpMake adds the right -xc++-module / -xc++ flag automatically).

`Fetch { }`
Clone a git repository at build time.

```txt
Fetch {
    https://github.com/user/repo.git -> include/repo
    https://github.com/glfw/glfw.git -> external/glfw/include
}
```
Left side: the clone URL
Right side: the subdirectory inside the cloned repo that should be added to the include path
Repos clone into build/HelpMake/dep/<repo-name>/
HelpMake adds -I<cloned-path>/<subdir> to the compile flags automatically
Requires git on PATH. Failing to find git is a fatal error.
**For dependencies that need to be built or linked, use [`Dependency: name { }`](#dependency-blocks) instead.**

`PreBuild { }` / `PostBuild { }`
Commands run before / after the build.

```txt
PreBuild {
    python3 tools/gen_version.py
    mkdir -p build/assets
}

PostBuild {
    strip build/app
    cp build/app ../dist/
}
```
Multiple entries are joined with && and run as a single shell command
A failing PreBuild aborts the build
A failing PostBuild is a warning - the binary is still reported as built

`Copy { }`
Copy files or directories into the output area after a successful build.

```txt
Copy {
    assets -> build/assets
    config/*.json -> build/config
    README.md -> build/
}
```
Syntax: <source> -> <destination>. Supports:
Single files: src -> dest - copied into dest/ if it's a directory
Directories: src/ -> dest/ - recursive copy
Globs: *.json -> dest/ - every matching file in the source directory
Missing sources are a fatal error.

`Set { }`
Variable definitions. See [Variables](05-variables.md).

```txt
Set {
    CXX: g++
    CXX_STD: c++20
}
```
`Arguments { }`
Per-config boolean flags (only valid inside a Config: block):

```txt
Config: dev {
    Arguments {
        verbose
        run
    }
}
```
Available: verbose, debug, run, time, json, rebuild, notify, quiet.

## Config blocks
Define multiple targets in one file:

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
    Compiler: clang++
    Flags: -O3 -DNDEBUG -flto
    Output: build/release.exe
}
```
Each config inherits every field from the root and overrides only what it explicitly sets. Fields inherited: Compiler, Version, Output, Flags, InputFiles, Includes, Modules, Kind, PreBuild, PostBuild, Copy, Arguments, PCH, Jobs.

Build them:

```bash
hm -b --config debug
hm -b --config release
hm -b --config debug --config release
hm -b --config-all
```
Config names may not contain spaces. Duplicate config names are a parse error.

### Typo detection
HelpMake recognizes common misspellings and rejects them at parse time:

```txt
Compilr: g++       # Error: Found 'Compilr'. Did you mean Compiler ?
InputFile {        # Error: Found 'InputFile {'. Did you mean InputFiles ?
Kind: exectuable   # Error: Invalid Kind 'exectuable'. Did you mean executable ?
```
The check runs at line granularity - one bad line never masks another.

### Common gotchas
- src/\*.cpp is not recursive. Use src/**/*.cpp for recursive.
- Empty blocks are fine. Flags { } on one line or empty on two lines does nothing.
- Comments inside blocks work. # not this file inside InputFiles { } skips that line.
- [[name]] is expanded in every field and block entry. See [Variables](05-variables.md).
- **`Dependency:` blocks are root-only.** They cannot appear inside `Config: { }`. Use `Depends:` on the config.
- **`Git:` and `Url:` are not interchangeable.** `git clone` fails on tarballs - use `Url:` for `.tar.gz` / `.zip`.
- **Put `Dependency:` blocks before any `Config:` block.** The splitter doesn't recognize `Dependency:` after a config section.
