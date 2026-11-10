# Changelog

All notable changes to HelpMake.

## [12.4.5] - 2026-11-10

### Added

- **Sanitizer Support** - You can now use `--sanitize=address,undefined` (or set `Sanitize: address` in `HelpMake.txt`). HelpMake automatically appends the correct `-fsanitize=...`, `-fno-omit-frame-pointer`, and `-g` flags for GCC/Clang, and `/fsanitize=...` for MSVC.
- **Native Binary Stripping** - Added `--strip` flag and `Strip: true` config field. Automatically runs `strip` on the final binary to dramatically reduce its size for release builds (ignored on MSVC).
- **Package Block** - You can now create `.zip` or `.tar.gz` release archives natively without any external tools or PostBuild scripts. 
  - On Windows, it uses native PowerShell (`Compress-Archive`).
  - On Linux/macOS, it uses native `zip` or `tar`.
- **Workspaces (Subsets)** - Added `Subset { }` block and `Subset: <path>` field. Allows building monorepos from a single root config by automatically entering subdirectories and running `hm -b`.

### Fixed

- **GNU `ld` Linker Ordering on Windows** - External dependencies (like Raylib) now correctly wrap their link flags in `-Wl,--start-group` and `-Wl,--end-group`. This prevents system libraries (like `-lgdi32` or `-lwinmm`) from being discarded by the linker before the dependency archive is processed.
- **Linker flags leaking into compile step** - `-L` and `-Wl,` flags are now correctly separated out and only passed to the linker, keeping the compile commands clean.

## [12.4.4] - 2026-11-08

### Added

- **`CC`, `CXX`, `CROSS_COMPILE` env vars** - auto-detection now honors these before falling back to probing. `CXX=clang++ hm -b` picks clang++; `CROSS_COMPILE=arm-none-eabi- hm -b` picks `arm-none-eabi-g++`. Explicit `Compiler:` and `-C=` still win over env vars.
- **Windows entry points** - `autoKind` now recognizes `WinMain`, `wmain`, and `_tmain`, not just `main`. Windows GUI projects are no longer silently misclassified as `Kind: static`.
- **Expanded link-flag table** - 20 system-library headers (was 11). Adds SDL2, GLFW, OpenGL, GLEW, gtest, fmt, sqlite3, and expat alongside the existing pthread, math, dl, zlib, curl, and OpenSSL entries.
- **`stripSource()`** - new string utility in `hm/util/strings.hpp` that blanks out line comments, block comments, and string/char literals in a single pass while preserving line structure. Used by `autoKind` and `autoLinkFlags`.

### Fixed

- **`--nofile` respects explicit CLI inputs** - `hm -b --nofile pro.cpp` now compiles only `pro.cpp`, even when the working directory contains other C/C++ projects. Previously the CLI input was stored in `config.inputFile` but not `config.v_inputFiles`, so auto-detection ran anyway and picked the dominant extension from a recursive walk of `.` - often compiling unrelated projects nested in the same parent directory.
- **`--nofile` skips the auto-detection chain** as documented. Only CLI-provided fields are used. `--nofile` now errors out with a clear message if `-C=<compiler>` or an input file is missing, instead of silently falling back to auto-detection.
- **Positional CLI args populate `config.v_inputFiles`** in addition to `config.inputFile`, keeping the two lists in sync. `InputFiles { }` parsing, `expandInputs`, and companion-source detection all read the same list of files.
- **`autoKind` ignores comments and string literals** - a `// int main()` line comment, a `/* int main() {} */` block comment, or a `std::string s = "main("` literal no longer misleads the executable/static decision.
- **`autoKind` detects multi-line signatures** - `int main\n(int, char**)` (signature broken across lines) is now recognized.
- **`autoLinkFlags` ignores quoted includes and comments** - `#include "pthread.h"` (local file), `/* #include <pthread.h> */`, `#includefoo`, and `#include_next <...>` no longer trigger link-flag injection. Only actual `#include <...>` lines in real code count.
- **`autoOutput` skips generic directory names** - running `hm -b` from inside `build/`, `out/`, `dist/`, `bin/`, `tmp/`, `temp/`, or `src/` no longer names the binary after the folder. Falls through to `main.*` detection or `a.out` / `a.exe` as the docs already promised.


## [12.4.3] - 2026-11-08

### Added

- **VS Code Extension** - Official syntax highlighting for `HelpMake.txt` and `.helpmake` files. [Available on the VS Code Marketplace](https://marketplace.visualstudio.com/items?itemName=Xavi99.helpmake) inside the website.
- **Workspaces (Subsets)** - You can now make multi-project builds from a single root `HelpMake.txt` using the `Subset { }` block or `Subset: <path>` field. HelpMake will automatically build the config file that's in the subset directory. It supports directories (looking for `HelpMake.txt` or using zero-config) and specific file paths (using `-f`).
- **Output Normalization** - HelpMake now respects platform-specific naming conventions for `Output:` if you omit the file extension.
  - Executables automatically get `.exe` on Windows and no extension on Linux/macOS.
  - Static libraries automatically get `lib` prefix and `.a` on GCC/Clang/MinGW, and `.lib` on MSVC.
  - Explicit extensions (like `app.bin` or `math.dll`) are left untouched.

### Fixed

- **Linker flags leaking into compile step** - Flags like `-Wl,--start-group` and `-L` are now correctly separated and only passed to the linker, not the compiler.

## [12.4.2] - 2026-11-07
- **Binaries** - Download HelpMake for [Windows](https://github.com/Gag91/Help_Make/releases/latest/download/hm-windows-x86_64.zip), [Linux](https://github.com/Gag91/Help_Make/releases/latest/download/hm-linux-x86_64.tar.gz) or [MacOs](https://github.com/Gag91/Help_Make/releases/latest/download/hm-macos-x86_64.tar.gz).

## [12.4.1] - 2026-11-07

### Added

- **Flag grouping** - Short flags can now be combined. `hm -bvd` is parsed as `-b -v -d`. Value flags like `-bC=clang++` are supported.
- **`--flag=value` syntax** - Long boolean flags now explicitly accept values, e.g., `--verbose=true` or `--quiet=false`.
- **`-t` / `--test`** - Testing is now a standard flag to match `-b` / `--build`.
- **`-c` / `--clean`** - Cleaning is now a standard flag.

### Changed

- **Short flag convention** - Lowercase short flags are strictly for booleans (`-v`, `-d`, `-r`), while uppercase short flags are for values (`-C`, `-V`, `-O`, `-J`). The `-r` short flag now means `rebuild` (use `--run` to run the output).

### Fixed

- **Linker dependency ordering** - `Depends: libpng, zlib` no longer fails if the user writes them in the wrong order. HelpMake now wraps generated link flags in `-Wl,--start-group` and `-Wl,--end-group` for GCC/Clang to force the linker to resolve circular dependencies.
- **CLI Inconsistency** - Previously, building was a flag (`-b`) but testing was a subcommand (`test`). The CLI is now fully consistent, using flags for all major actions (`-b`, `-t`, `-c`).

- **Config-to-Config dependencies** - `Depends:` can now reference internal `Config:` blocks. HelpMake automatically resolves the build order, detects circular dependencies, and builds the dependencies first.
- **Duplicate Config names** - HelpMake now throws a parse error if two configs share the same name.

## [12.4.0] - 2026-11-05

### Added

- **Zero-config builds** - `hm -b` now builds a project with **no `HelpMake.txt` at all**. Every field is inferred at parse time; user-specified values always win. See [Auto-detection](11-auto-detection.md).
- **`autoLinkFlags`** - scans `#include <...>` lines and adds the right link flags automatically:
  - `<thread>`, `<mutex>`, `<atomic>`, `<condition_variable>`, `<pthread.h>` → `-pthread`
  - `<cmath>`, `<math.h>` → `-lm`
  - `<dlfcn.h>` → `-ldl`
  - `<zlib.h>` → `-lz` / `zlib.lib` (MSVC)
  - `<curl/curl.h>` → `-lcurl` / `libcurl.lib` (MSVC)
  - `<openssl/ssl.h>` → `-lssl -lcrypto` / `libssl.lib libcrypto.lib` (MSVC)
  - POSIX-only flags are skipped on MSVC. Existing flags are never duplicated.
- **`autoKind`** - scans every source for a valid `main(` declaration (`main()`, `main(int argc, char** argv)`, `main(void)`, `int main (int argc , ...)`) with word-boundary checks (rejects `domain(`, `mainHelper(`, `main_loop(`) and picks `Kind: executable` or `Kind: static` accordingly. Falls back to `executable` for non-C-family projects (Zig, Go, Rust, Python, etc.).
- **`autoOutput`** - infers `Output:` from twelve different sources, in priority order: `Cargo.toml`, `go.mod`, `package.json`, `pyproject.toml`, `setup.py`, `build.zig`, `meson.build`, `CMakeLists.txt`, `pom.xml`, `build.gradle` / `build.gradle.kts`, `*.csproj` / `*.fsproj` / `*.vbproj`, `main.*` source file, then the directory name. Falls back to `a.out` / `a.exe` with a warning.
- **Companion source detection** - when a source includes `"foo.hpp"` and `foo.cpp` exists anywhere in the tree but isn't in `InputFiles { }`, `foo.cpp` is added automatically. Zero extra cost for projects that already list all sources.

### Changed

- **`Info:` block formatting** - multi-line warnings (`Auto-added link flags:`, `Auto-detected include dirs:`, `Auto-added companion sources:`) sort to the bottom of their category so single-line warnings aren't interleaved; continuation lines align under the warning text instead of flush-left.

### Fixed

- **`autoIncludes` never emitted `-I` flags** - the function detected include directories correctly but only stored them in `includeFiles`, never appended `-I<dir>` to `flags`. Auto-detected projects failed with `fatal error: hm/parser/parser.hpp: No such file or directory` even though the warning claimed the includes were found. Both `flags` and `includeFiles` are now populated.
- **`autoIncludes` over-included header parents** - every directory containing a header was added as an include root, producing dozens of useless entries like `./include/hm/parser` and `./include/hm/cli`. Recursion now stops at the first `include/`, `inc/`, `headers/`, `api/`, or `public/` directory found. Header parents remain the fallback only when no named include dir exists.
- **`collectFiles` had an uninitialized `bool haveMain;`** (UB) and only checked `line.find("main()")`, missing `main(int argc, char** argv)` and `main(void)` - the two most common real-world signatures. Replaced by `autoKind` with proper word-boundary scanning.
- **`autoIncludes` had a leftover debug print** (`std::cout << "reachedddddddddddd"`) on the failure path. Removed.
  
## [12.3.0] - 2026-10-30

### Added

- **MSVC C++20 modules** - `.ixx`, `.cppm`, `.mpp`, and any file in `Modules { }` now compile as module interface units with `/interface /TP`. The `.ifc` goes to `build/HelpMake/msvc/modules/` (via `/ifcOutput`), consumers import via `/reference <name>=<path>`. Works for single modules and cross-imports; partitions are still unsupported (need `/internalPartition` and per-partition `/reference`).
- **Dependency Locking** - `hm --update-lock` records exact Git revisions for Fetch { } and Dependencies, sources in `build/HelpMake/dep/lock/HelpMake.lock`; build with `--frozen` to use those revisions and fail if the lockfile is missing or incomplete. Archive and local-path dependencies aren’t lockable yet.
- **HelpMake API** - `#include "HelpMake.hpp` includes all methods to build specified files/modules using specified flags.
- **Dry Run** - `hm -b --dry-run` Outputs all commands whitout executing them.

### Fixed

- **Clang/MSVC self-reference in module flags** - the module currently being compiled was getting `-fmodule-file=` (Clang) or `/reference` (MSVC) pointing at its own `.pcm`/`.ifc` before that file existed. Both compilers tolerate it, but it's fragile. Now skipped when `modName == current module's name`.

## [12.2.0] - 2026-10-24

### Added

- **`hm test`** - build and run every config marked with `Test: true`
  - Prints pass/fail per test with timing, aggregates the result, exit code 1 on any failure (CI-ready)
  - `--verbose` / `--debug` show per-file compile output for each test; `--quiet` shows only the summary
  - Test binaries are actually **executed** (a config that builds but returns non-zero reports FAIL)
- **`Test: true`** field - marks a config as a test target. Valid only inside `Config: { }` blocks
- **Response files** - link commands longer than 30,000 characters (Windows caps at 32,767) now write args to `build/HelpMake/<comp>/<baseName>.rsp` and pass `@file.rsp`. Fixes silent argument truncation on 300+ file projects
- **`--verbose` on clean commands** - `hm --clean --verbose` prints each removed path

### Changed

- **`hm --clean-all` deletes `build/HelpMake/` and `gcm.cache/`**, not all of `build/`. User-placed files under `build/` (assets, configs) are preserved
- **Clean commands return exit code** - a failed clean exits 1 with a per-path error, instead of printing "Cleaned X" and exiting 0
- **`Fetch { }` is now idempotent** - clones only if the target directory doesn't exist. Failed clones clean up so retry works. Missing `-> subdir` paths abort at parse time
- **`hm test` output** - aligned columns, green/red status, reason column (`build failed` vs `exit code N`), failure log paths at the bottom

### Fixed

- **Clang modules: extension-independent support** - `.ixx`, `.mpp`, `.module`, and any file in `Modules { }` now compile correctly (was failing on `.cppm`-only paths)
- **Clang modules: `-fmodules` no longer applied** - that flag is Apple Clang's module system, not C++20 modules. Now GCC-only. Fixes MSVC-header compile errors on Clang
- **Clang modules: `.pcm` handling** - emitted to `build/HelpMake/<comp>/modules/`, forces rebuild when missing, compiles sequentially before consumers to prevent parallel races
- **`.buildid` now tracks effective compiler command** - fixes "Build flags changed" appearing on every build after adding `Modules { }`
- **Per-config logs use the config's output name** - `hm -b --config foo` writes to `logs/foo.txt`, not `logs/hmm.txt`. `See Logs: X` now points at the right file
- **`hm --clean` handles read-only files** - `git clone` marks `.git/objects/pack/*` read-only; Windows refuses to delete them. Clean commands clear the read-only bit, skip locked subdirectories, and report per-path failures instead of crashing
- **`-I` / `-L` correct on MSVC** - include and library paths now use `/I` and `/LIBPATH:` instead of the GCC flags
- **`--run` handles output paths with spaces** - output binary is quoted in the `&&` chain
- **`compile_commands.json` correct for all Clang module extensions** - `-xc++-module` emitted for `.ixx` and `.mpp`, not just `.cpp`
  
## [12.1.0] - 2026-10-23

### Added

- **Platform conditionals** - prefix any line with `win:`, `linux:`, `mac:`, `unix:`, `x64:`, `arm64:`, `x86:`, or `!<tag>` to include it only on matching platforms
  - Works for scalar fields (`win: Output: build/app.exe`), block entries (`linux: -pthread`), and `Set { }` variables
  - Recognized: `win`/`windows`, `linux`, `mac`/`darwin`/`macos`/`osx`, `unix`, `x64`/`x86_64`/`amd64`, `arm64`/`aarch64`, `x86`/`i386`/`i686`; negation via `!tag`
- **`--watch`** - rebuild automatically on source, header, or `HelpMake.txt` change. Combine with `-r` for the full dev loop. Alias: `hm --watch` ≡ `hm -b --watch`
- **CMake generator: Fetch / Modules / PreBuild / PostBuild / Copy** - the generator now emits `FetchContent_*`, `FILE_SET CXX_MODULES`, custom targets, and `POST_BUILD` copy commands. Closes the CMake-vs-Ninja feature gap
- **`Set { }` scoping per config** - a `Set { }` inside a `Config: { }` no longer leaks to other configs

### Changed

- **`--verbose` output cleaned up** - config fields print as a compact aligned block per config, not a flat stream of `X found in config 'Y'` lines
- **Generator warnings** route through the `Info:` block at the end of the build instead of printing mid-parse

### Fixed

- **`--gen` paths run `resolveDependencies` first** - fetched and dependency include paths are emitted correctly into generated CMake/Ninja/JSON files
- **Platform conditional overrides work** - `win: Output: ...` after a fallback `Output: ...` now overrides; previously the first write won and the conditional was silently dropped
- **`Set { }` uses the config's variable scope** - fixes values leaking across configs during parse
  
## [12.0.0] - 2026-10-23

### Added
- `--watch` - rebuild automatically when any source, header, or `HelpMake.txt` changes
  - Polls mtimes every 300ms; rebuilds on any detected difference
  - Watches `InputFiles` (globs expanded), all `Includes { }` directories recursively, and `HelpMake.txt` itself
  - Works with `--config` and `--config-all`
  - Top-level alias: `hm --watch` is equivalent to `hm -b --watch`

### Changed
- **Exit codes now propagate correctly.** `hm -b` returns non-zero on any failure (parse, compile, link, copy). Previously every path returned 0 - scripts using `if hm -b; then …; fi` silently passed on failed builds. This is the intended behavior per the CLI docs, but if you relied on the old always-zero behavior (e.g. ignoring compile failures in CI), adjust your scripts.
- **`-f<file>` (joined, no space) is now rejected.** Use `-f <file>` or `-f=<file>`. This fixes a collision where `-fsanitize=address`, `-fno-omit-frame-pointer`, and every other `-f*` compiler flag was silently treated as a build-file path.

### Fixed
- **`compile_commands.json` was corrupted** - every file began with the literal string `compile_commands.json` because the generator passed that string as the initial content of a `std::ostringstream`. Now valid JSON.
- **Parallel builds wrote to a single log file** - every job shared `<baseName>.raw`, so under `-j N` the compiler failed with "The process cannot access the file because it is being used by another process." Each job now writes to its own log.
- **Nested source directories failed to compile** - object subdirectories (e.g. `obj/src/toolchain/`) were not created before invoking the compiler, so `-MMD` failed with "opening dependency file: No such file or directory." Fixed.
- **`Copy { }` with a glob pattern** always failed because the code inspected the parent path of the whole `src -> dest` string. Fixed to use the source side only.
- **`-I<path>` and `-L<path>` concatenated with the previous flag** when `flags` was non-empty but `v_Flags` was empty (`-Wall` + `-Iinclude` → `-Wall-Iinclude`). Fixed.
- **MSVC `Version:` got a `-` prefix** - `/std:c++20` became `-/std:c++20`. Fixed to preserve `/`.
- **CLI prefix matches were too loose** - `--var` matched `--variant=`, `--config-all` matched `--config-all-xyz`, and `--Kind=` rejected single-character kinds. All tightened.
- **`--config-all` executed each config mid-parse**, inheriting a root config that wasn't finished yet. Now runs after all sections are parsed.
- **`--run` with spaces in the output path** produced `./my app.exe` unquoted and failed. Fixed.
- **`GenericCompiler::directBuildArgs` returned an empty vector**, so `hm --nofile -C=<unknown>` emitted commands with no inputs and no output. Fixed.
- **`init.cpp` always printed `true`** for the `compile_commands.json` prompt due to `<<` binding tighter than `?:`. Fixed.
- **`logs/` directory was not created for root builds**, so `See Logs: build/HelpMake/gcc/logs/<name>.txt` pointed at a nonexistent file.
- **PCH was not invalidated when flags changed** - `.buildid` trashed `obj/` but left `headers/<compiler>/` intact, producing stale precompiled headers after `-DDEBUG` toggles.
- **`v_DepFiles` accumulated across configs**, so a header touched by one config forced the PCH to rebuild in every subsequent config.
- **Ninja generator collided on identical stems** - `src/foo.cpp` and `foo.cpp` both produced `foo.o`. Now uses full relative paths.
- **`Parser::executeConfig` read `cfg.debug` and `cfg.v_Info`** instead of the per-config `c.debug` / `c.v_Info`, so `Arguments: debug` inside a `Config: { }` block was silently ignored, and `Gen { }` info lines for configs never printed.
- **`Arguments: verbose debug` was passed as one token** to `parseArgument`, so multi-word `Arguments:` lines were dropped. Now split.
- **MSVC got GCC flags in `compile_commands.json`** - `-fdiagnostics-color=always`, `-c`, `-o`, `-MMD`. Now emits `/nologo /c /Fo:`.
- **`-C` and `-V` bare forms** (`-C gcc`, `-V std=c++20`) were rejected; only `-C=` / `-V=` worked. Both accepted now.
- **`Fetch { }` shelled out to `git clone` even during `--check` and `--show`**, doing network I/O when the user only wanted to validate the file. Now skipped in `--check`.
- **`extractModule` called `std::exit(1)` on files without `export module`**, terminating the build instead of reporting an error. Now returns an empty string.
- **`parseVar` broke out of the parsing loop on the first undefined variable**, leaving the rest of the line unparsed. Now continues and still reports all errors.
- **`Set { }` variables leaked across configs** - a `[[x]]` in config B could resolve to a value set in config A depending on section order. Now scoped per config.
  
## [11.2.0] - 2026-10-21

### Added
- `hm --gen cmake` - emit `CMakeLists.txt` for the selected config
- `hm --gen json` / `hm --gen compile_commands` - emit `compile_commands.json` (previously only via `--json`)
- `--gen <name> --config <name>` - generate build files for a specific config, not just the root

### Fixed
- `--gen json` now respects `--config` (previously generated for the root config)

### Known issues
- `LinkFlags:` order relative to archive paths may need adjustment for static libraries with unresolved system symbol dependencies
- `Dependency:` blocks must appear before any `Config:` block
- `Depends:` between dependencies (transitive ordering) is parsed but not yet executed
  
## [11.1.0] - 2026-10-21

### Added
- `Dependency: name { }` blocks - fetch, build, and link external libraries
  - `Git:`, `Url:`, `Path:` sources
  - `Tag:`, `Commit:`, `Hash:` version pinning
  - `Build:` shell command list (repeatable) with `.buildid` cache
  - `Include:`, `Link:`, `LinkDir:`, `LinkFlags:` integration
  - Cache lives at `build/HelpMake/dep/<name>/`
- `Depends:` field on configs - pulls dependency blocks into a build
- Go toolchain (`go build`) with `-Go` CLI flag
- `hm --gen ninja` - emit a `build.ninja` file
- `hm --gen` with no argument lists available generators

### Changed
- GCC/Clang now select `gcc`/`clang` for `.c` sources and add `-xc` (previously all sources were forced into C++ mode via `g++ -xc++`)
- `executeConfig` treats `.a`, `.lib`, and `.so` path arguments as link inputs
- `Parser::parse()` refactored into a section splitter plus a per-section parser

### Fixed
- CLI flags (`-o`, `-C=`, `-V=`, `--Kind=`, `--var`) now correctly override values in `HelpMake.txt` - previously the file value always won
- Interrupted builds no longer leave stale objects that produce confusing "undefined reference" errors on the next build (still requires `hm --clean` after a hard interrupt, but the failure is now obvious)

### Known issues
- `LinkFlags:` order relative to archive paths may need adjustment for static libraries with unresolved system symbol dependencies (GNU `ld` processes left-to-right)
- `Dependency:` blocks must appear before any `Config:` block in `HelpMake.txt`
- `Depends:` between dependencies (transitive ordering) is parsed but not yet executed
- Dependency cache assumes a clone succeeded; corrupt clones from interrupted `git clone` need a manual `rm -rf build/HelpMake/dep/<name>`

## [11.0.0] - 2026-18-10
### Added
- MSVC, Zig, Zig C++, Zig CC toolchains
- Precompiled headers (all toolchains)
- `Set { }` variables and `--var` CLI override
- `Copy { }` block

### Changed
- Objects now live under `build/HelpMake/<compiler>/obj/`
- Changing compiler or flags forces a clean rebuild

### Fixed
- Worker threads no longer abort chunks on first failure
- Per-file logs land in per-compiler log dirs

## [10.x] - earlier
### Added
- Zig support (build-exe)
- `--jobs` parallel compilation
- C++20 modules (GCC + Clang)

## [9.x] - earlier
### Added
- Static libraries (`Kind: static`)
- `--init` interactive wizard
- `--install` automatic setup

## [8.x] - earlier
### Added
- `Config: name { }` blocks
- `--json` compile_commands.json generation
- `--create` from CLI args

## [7.x] - earlier
### Added
- `--quiet`, `--notify`, `--clean-gcm`
- Header tracking (incremental via `.d`)

## [6.x] - earlier
### Added
- C++20 modules (first version)
- `-C=<compiler>` and `-V=<version>` any-compiler support

## [5.x] - earlier
### Added
- `Fetch { }` git cloning
- PreBuild / PostBuild hooks

## [4.x] - earlier
### Added
- `Includes { }`, `Flags { }` blocks
- `--run`, `--debug`, `--verbose`

## [3.x] - earlier
### Added
- Zig support (first pass)
- GitHub link fetching

## [2.0.0] - 
### Added
- `Flags { }` block, inline forms
- Version tracking

## [1.x] - 
### Added
- Initial release: `Compiler:`, `Version:`, `Output:`, `InputFiles:`
