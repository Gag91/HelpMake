# CLI reference

Complete list of `hm` command-line flags.

## Commands

| Flag | Description |
|---|---|
| `-h`, `--help` | Show the help message |
| `-v`, `--version` | Show version, author, license link |
| `-l`, `--license` | Show the MIT license text |
| `-b`, `--build` | Build using `HelpMake.txt` |
| `--watch` | Watch mode - rebuild on file change. Equivalent to `hm -b --watch` |
| `-f <file>` / `-f=<file>` | Use a custom config file instead of `HelpMake.txt`. `-f<file>` (no space, no `=`) is not accepted. |
| `--init` | Interactive project wizard that generates `HelpMake.txt` |
| `--install` | Clone + build + add `hm` to PATH automatically |
| `--show` | Parse and print the current configuration |
| `--dump` | Print the raw `HelpMake.txt` contents |
| `--create` | Generate `HelpMake.txt` from CLI arguments |
| `--clean` | Delete `build/HelpMake/` |
| `--clean-gcm` | Delete `gcm.cache/` |
| `--clean-all` | Delete both |
| `--gen <name>` | Emit a build file for another tool. See [Generators](#generators). |

## Build options

### Picking a toolchain

| Flag | Toolchain |
|---|---|
| `-Gcc` | GCC (`g++`) |
| `-Clang` | Clang (`clang++`) |
| `-MSVC` | MSVC (`cl.exe`) - Windows only |
| `-Zig` | Zig (`zig build-exe`) |
| `-ZigCpp` | Zig C++ mode (`zig c++`) |
| `-ZigCC` | Zig C mode (`zig cc`) |
| `-Go` | Go (`go build`) |
| `-Sere` | Sere compiler |
| `-C=<compiler>` | Any compiler on `PATH` |

`-C=` accepts the same names as `HelpMake.txt`. Quoted names with spaces work:

```bash
hm -b -C="zig c++"
hm -b -C="zig cc"
```

### Other build options

| Flag | Description |
|---|---|
| `-V=<version>` | Standard flag. `-V=std=c++20` becomes `-std=c++20` in the compile command. A leading `-` is added automatically if absent. |
| `--Kind=<kind>` | `executable` (default) or `static`. See [Static libraries](08-static-libs.md). |
| `--var NAME=VALUE` | Override a `Set { }` variable. See [Variables](05-variables.md). |
| `-I<path>` | Add `-I<path>` to compile flags (same as `Includes { }` inline). |
| `-L<path>` | Add `-L<path>` to link flags. |
| `-F<flags>` | Add raw flags to compile+link. |
| `-o<file>` | Set the output filename. |

### Build flags

| Flag | Description |
|---|---|
| `-j`, `--jobs N` | Parallel compilation with N jobs. `-j` alone uses all cores. |
| `--config <name>` | Build only that config. May be repeated. |
| `--config-all` | Build every config in the file. |
| `--nofile` | Build from command-line arguments only, no config file required. |
| `-r`, `--run` | Run the output binary after a successful build. |
| `--verbose` | Show the actual command lines and per-file timings. |
| `--debug` | Show parser trace plus every command (implies `--verbose` output). |
| `--rebuild` | Force a full rebuild, ignoring timestamps and dep files. |
| `--json` | Generate `compile_commands.json` alongside the build. |
| `--no-sep` | One-shot compile - pass all sources to the compiler in one command. Disables incremental builds, PCH, and `Kind: static`. |
| `--notify` | Desktop notification on build completion (Windows Toast / notify-send on Linux). |
| `--quiet` | Suppress warnings. Errors still print. |
| `--time` | Print the total build time even without `--verbose`. |
| `--watch` | Rebuild automatically when any source, header, or `HelpMake.txt` changes. Ctrl+C to exit. Combine with `--run` for the full dev loop. |

## Common patterns

### Dev loop
```bash
hm -b --watch         # rebuild on save
hm -b --watch -r      # rebuild and rerun on save
### Build one config
```bash
hm -b --config debug
```

### Build every config (CI)
```bash
hm -b --config-all --json --quiet
```

### Override compiler without editing the file
```bash
hm -b -C=clang++ -V=std=c++23
```

### Build from CLI only (no file)
```bash
hm -b --nofile -C=gcc -o hello src/main.c
```

### Cross-platform script
```bash
# Linux
hm -b -C=clang++ --var OUT=build/app --var OPT=-O3

# Windows
hm -b -MSVC --var OUT=build\app.exe --var OPT=/O2
```

### Debug a build failure
```bash
hm -b --verbose
```

Then open the log:
```text
build/HelpMake/gcc/logs/<output-name>.txt
```
The log has ANSI color codes stripped, so `grep` and `less` work cleanly.

## Generators

`hm --gen <name>` emits a build file for another tool. `hm --gen` (no
argument) lists available generators.

| Generator | Emits | Requires |
|---|---|---|
| `ninja` | `build.ninja` | ninja ≥ 1.10 |
| `cmake` | `CMakeLists.txt` | CMake ≥ 4.0 (3.28+ for modules) |
| `json` / `compile_commands` | `compile_commands.json` | any clangd-compatible tool |

### CMake generator support

| Feature | Status |
|---|---|
| Separate compilation | ✅ |
| Includes / Flags / Link flags / Static libs | ✅ |
| PCH | ✅ `target_precompile_headers` |
| Modules | ✅ `FILE_SET CXX_MODULES` (CMake ≥ 3.28) |
| PreBuild | ✅ `add_custom_target` + `add_dependencies` |
| PostBuild | ✅ `add_custom_command(POST_BUILD)` |
| Copy | ✅ glob / file / directory, expanded at generation time |
| Fetch | ✅ `FetchContent_Declare` + `FetchContent_MakeAvailable` |
| Depends | ⚠️ warned and skipped (use `FetchContent` manually) |
| MSVC | ⚠️ missing flag translation; use `hm -b` on Windows |

### Ninja generator support

| Feature | Status |
|---|---|
| Separate compilation | ✅ |
| Includes / Flags / Link flags / Static libs | ✅ |
| Copy | ⚠️ skipped |
| PCH | ⚠️ skipped |
| PreBuild / PostBuild | ⚠️ skipped |
| Modules | ⚠️ compiled as regular sources |
| Fetch | ⚠️ skipped |
| MSVC | ⚠️ skipped |

Regenerate after any `HelpMake.txt` change — the file is a snapshot, not
a live view.

## Exit codes

| Code | Meaning |
|---|---|
| `0` | Build succeeded |
| `1` | Build failed (compiler error, missing file, invalid config, unknown flag, etc.) |

Scripts can use `if hm -b; then …; fi` safely.

## Environment

HelpMake does not read any environment variables for its behavior - everything is on the command line or in `HelpMake.txt`. The compilers and tools it invokes may read their own (`CXXFLAGS`, `CC`, etc.), but HelpMake itself doesn't.

## Aliases in Compiler:

The `Compiler:` field (and `-C=`) accepts multiple spellings for the same toolchain:

| Toolchain | Accepted spellings |
|---|---|
| GCC | `gcc`, `g++` |
| Clang | `clang`, `clang++` |
| MSVC | `msvc`, `cl` |
| Zig | `zig` |
| Zig C++ | `zig c++`, `zig-c++`, `zigcpp`, `zig-cpp` |
| Zig CC | `zig cc`, `zig-cc`, `zigcc`, `zig-c` |
| Go | `go`, `golang` |
| Unknown | passed through as-is to `makeCompiler` (falls back to `GenericCompiler`) |
