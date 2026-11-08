# CLI reference

Complete list of `hm` command-line flags.

## Command Formats

HelpMake accepts flags in three flexible formats:

1. **Long flags:** `hm --build`
2. **Long flags with values:** `hm --verbose=true` or `hm --quiet=false`
3. **Short flags (Grouped):** `hm -br` (build and rebuild) or `hm -bC=g++` (build with GCC)

### Flag Grouping
You can combine multiple boolean short flags into a single argument to save keystrokes. For example, `hm -bvw` is equivalent to `hm -b -v -w` (build, verbose, watch). 

If you need to pass a value (like a compiler or output name) in a group, it **must be the last flag** in the group, followed immediately by the value, and it **must be uppercase**:
*   `hm -bC=clang++` (build with Clang)
*   `hm -bO app.exe` (build and set output to app.exe)
*   `hm -bJ4` (build with 4 jobs)

*Rule of thumb:* Lowercase short flags are toggles (`-v`, `-d`, `-q`). Uppercase short flags take values (`-C`, `-V`, `-O`, `-J`).

### Grouped Short Flags (used with `-b`)

When building, you can chain these lowercase boolean flags together. 
For example, `hm -bvd` means "build, verbose, debug". `hm -brq` means "build, rebuild, quiet".

| Short Flag | Long Flag | Description |
| :--- | :--- | :--- |
| `-v` | `--verbose` | Show detailed build output |
| `-d` | `--debug` | Show debug information (parser trace, flags) |
| `-q` | `--quiet` | Suppress warnings |
| `-r` | `--rebuild` | Force full rebuild (ignore timestamps) |
| `-w` | `--watch` | Watch for file changes and rebuild |
| `-j` | `--json` | Generate `compile_commands.json` |
| `-t` | `--time` | Print total build time |
| `-n` | `--notify` | Desktop notification on completion |
| `-y` | `--dry-run` | Preview commands without executing |
| `-c` | `--create` | Generate HelpMake.txt from arguments |
| `-x` | `--nofile` | Build from CLI only (no config file) |
| `-a` | `--config-all` | Build every config in the file |
| `-z` | `--frozen` | Require Git revisions from HelpMake.lock |

### Grouped Value Flags (Uppercase)

If you need to pass a value inside a group, use the uppercase letter at the end of the group, followed by the value.

| Short Flag | Long Flag | Example | Description |
| :--- | :--- | :--- | :--- |
| `-C` | `--compiler` | `hm -bC=clang++` | Set the compiler |
| `-V` | `--version` | `hm -bV=std=c++20` | Set the standard/version |
| `-O` | `--output` | `hm -bO=app.exe` | Set the output filename |
| `-J` | `--jobs` | `hm -bJ4` | Set the number of parallel jobs |
| `-K` | `--kind` | `hm -bK=static` | Set target kind (executable/static) |
| `-I` | `-I<path>` | `hm -bI=include` | Add an include directory |
| `-L` | `-L<path>` | `hm -bL=lib` | Add a library search path |
| `-F` | `-F<flag>` | `hm -bF=-O3` | Add a raw compiler flag |

### Subcommands vs Flags
HelpMake supports both subcommands and flags to fit different workflows:

| Action | Short Flag | Long Flag |
| :--- | :--- | :--- |
| Build | `hm -b` | `hm --build` |
| Test | `hm -t` | `hm --test` |
| Clean | `hm -c` | `hm --clean` |
| Watch | `hm -w` | `hm --watch` |
| Help | `hm -h` | `hm --help` |
| Version | `hm -v` | `hm --version` |
| License | `hm -l` | `hm --license` |

*Tip: Use long flags or subcommands in CI/CD scripts for readability, and short flags in your terminal for speed.*

## Commands

| Flag | Description |
|---|---|
| `-h`, `--help` | Show the help message |
| `-v`, `--version` | Show version, author, license link |
| `-l`, `--license` | Show the MIT license text |
| `-b`, `--build` | Build using `HelpMake.txt` |
| `test` | Build and run every config marked with `Test: true`. Exit code 0 on all pass, 1 on any fail |
| `--watch` | Watch mode - rebuild on file change. Equivalent to `hm -b --watch` |
| `-f <file>` / `-f=<file>` | Use a custom config file instead of `HelpMake.txt`. `-f<file>` (no space, no `=`) is not accepted. |
| `--init` | Interactive project wizard that generates `HelpMake.txt` |
| `--install` | Clone + build + add `hm` to PATH automatically |
| `--show` | Parse and print the current configuration, including root and per-config variables |
| `--dump` | Print the raw `HelpMake.txt` contents |
| `--create` | Generate `HelpMake.txt` from CLI arguments |
| `--clean` | Delete `build/HelpMake/` |
| `--clean-gcm` | Delete `gcm.cache/` |
| `--clean-all` | Delete both |
| `--update-lock` | Resolve Git dependencies and write `HelpMake.lock` without building |
| `--gen <name>` | Emit a build file for another tool. See [Generators](#generators). |

## Testing

`hm test` builds and runs every config marked with `Test: true`:

```bash
hm test
hm test --verbose
hm test --frozen
```

Configs are discovered via the `Test: true` field - a name match (`test-*`) is not how membership is decided. Add `Test: true` to any `Config: { }` block to include it:

```txt
Config: smoke {
    Test: true
    Compiler: g++
    Output: build/smoke.exe
    InputFiles { tests/smoke.cpp }
    Flags { -Wall }
}
```

Output format:

```text
[HelpMake] Test suite: 3 configs

  test-clang-modules   PASS  0.42s
  test-gcc-modules     PASS  0.31s
  test-platform        PASS  0.15s

  3 passed, 0 failed, 0.88s total
```

A failing test binary (non-zero exit) shows `FAIL exit code N`, and a compile failure shows `FAIL build failed`. The failure block after the summary lists which tests failed and where their logs live:

```text
Failures:
  test-clang-modules   exit code 1

  See logs under build/HelpMake/*/logs/
```

Configs are run alphabetically (`std::map` iteration order). Each test binary is executed with the working directory of `hm` itself. `--quiet` prints only the summary and failures block.

## Clean commands

`--clean`, `--clean-gcm`, and `--clean-all` are safe to run any time. They don't touch source files, `HelpMake.txt`, or the final output binary (which lives wherever `Output:` points, typically `build/app.exe` outside `build/HelpMake/`).

All three clear the read-only bit on files before deletion. This matters for `Fetch { }` and `Dependency { }`: `git clone` marks `.git/objects/pack/*` as read-only on Windows, and `std::filesystem::remove_all` throws on read-only files.

Add `--verbose` to print each removed path:

```bash
hm --clean --verbose
```

Exit code is `0` on success, `1` if any path couldn't be removed (locked file, permissions). Failure messages name the path and suggest closing editors/terminals/antivirus holding files open.

## Response files

When a link command exceeds 30,000 characters (Windows caps `CreateProcessA` at 32,767), `hm` writes arguments to `build/HelpMake/<comp>/<baseName>.rsp` and passes `@file.rsp` to the compiler. This is transparent - nothing in your `HelpMake.txt` needs to change. Under `--verbose`, `hm` prints:

```text
[HelpMake] Link command too long (32450 chars); using response file: build/HelpMake/gcc/app.rsp
```

Response files are compatible with GCC, Clang, MSVC, and GNU ar. On Linux/macOS the limit is high enough that this rarely triggers (`ARG_MAX` is 2MB).

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
| `--nofile` | Strict CLI-only build. Skips the config file **and** the auto-detection chain. Requires `-C=<compiler>` and at least one input file on the command line. |
| `-r`, `--run` | Run the output binary after a successful build. |
| `--verbose` | Show the actual command lines and per-file timings. |
| `--debug` | Show parser trace plus every command (implies `--verbose` output). |
| `--dry-run` | Show planned build commands without running them or changing build outputs. Dependency and generator actions are skipped.
| `--rebuild` | Force a full rebuild, ignoring timestamps and dep files. |
| `--frozen` | Require every Git `Fetch` and `Dependency` revision to match `HelpMake.lock`; fail if the lock is missing or incomplete. |
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

### Reproducible Git dependencies

Resolve Git revisions into `HelpMake.lock`, then commit the lockfile with the project:

```bash
hm --update-lock
hm -b --frozen
```

`--update-lock` does not compile the project. `--frozen` checks out the recorded commits for Git sources and fails if a referenced Git source is not represented in the lockfile. If an existing dependency cache points to a different repository, HelpMake replaces that stale cache before fetching. This first lockfile version supports Git-backed `Fetch { }` and `Dependency:` entries; archive and local-path dependencies are not lockable.

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
| `.clangd` | `.clangd`| any clangd-compatible tool |

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

Regenerate after any `HelpMake.txt` change - the file is a snapshot, not
a live view.

## Exit codes

| Code | Meaning |
|---|---|
| `0` | Build succeeded |
| `1` | Build failed (compiler error, missing file, invalid config, unknown flag, etc.) |

Scripts can use `if hm -b; then …; fi` safely.

## Environment

HelpMake reads three environment variables, all used by auto-detection:

| Variable | Effect |
|---|---|
| `CXX` | Compiler for C++ sources when `Compiler:` is unset. Example: `CXX=clang++ hm -b`. |
| `CC` | Compiler for C sources when `Compiler:` is unset. Example: `CC=gcc-14 hm -b`. |
| `CROSS_COMPILE` | Prefix applied to the default compiler when `CXX`/`CC` aren't set. Example: `CROSS_COMPILE=arm-none-eabi- hm -b` → `arm-none-eabi-g++` / `arm-none-eabi-gcc`. |

Priority is **`Compiler:` field / `-C=` > `CXX`/`CC` > `CROSS_COMPILE` > probe**. Explicit config and CLI always win.

Everything else is on the command line or in `HelpMake.txt`. HelpMake does not read `CXXFLAGS`, `LDFLAGS`, `CPPFLAGS`, or any variable not listed above. The compilers and tools it invokes may read their own - that's outside HelpMake's control.

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
