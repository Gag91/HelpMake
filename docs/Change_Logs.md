# Changelog

All notable changes to HelpMake.

## [12.1.0] - 2026-10-23

### Added
- **Platform conditionals** in `HelpMake.txt` - prefix any line with `win:`, `linux:`, `mac:`, `unix:`, `x64:`, `arm64:`, `x86:`, or `!<tag>` to include it only on matching platforms
  - Works for scalar fields (`win: Output: build/app.exe`), block entries (`linux: -pthread`), and `Set { }` variables (`win: PATH_SEP: \`)
  - Recognized tags: `win`/`windows`, `linux`, `mac`/`darwin`/`macos`/`osx`, `unix`, `x64`/`x86_64`/`amd64`, `arm64`/`aarch64`, `x86`/`i386`/`i686`; negation via `!tag`
- **`--watch`** - rebuild automatically when any source, header, or `HelpMake.txt` changes
  - Combine with `-r` for the full edit → rebuild → run dev loop
  - Top-level alias: `hm --watch` ≡ `hm -b --watch`
- **`FetchContent` support in CMake generator** - `Fetch { }` blocks now emit `FetchContent_Declare` / `FetchContent_MakeAvailable` with proper `target_include_directories`
- **Modules in CMake generator** - `Modules { }` entries emit `target_sources(... FILE_SET CXX_MODULES FILES ...)`, letting CMake handle PCM generation and ordering automatically
- **PreBuild / PostBuild / Copy in CMake generator** - emitted as `add_custom_target` (PreBuild) and `add_custom_command(POST_BUILD)` (Copy, PostBuild)
- **`Set { }` scoping per config** - a `Set { }` inside `Config: { }` no longer leaks to other configs (previously wrote to the shared `Parser::cfg` object)
- **`Fetch { }` warnings in Ninja/CMake generators** route through `v_Info` instead of printing immediately, so they surface in the consolidated `Info:` block

### Fixed
- **`-I` / `-L` concatenation bug** - `appendIncludes` produced `-Ibuild/...-Iinclude` (single string) instead of two separate flags, breaking any project with both `Fetch { }` and `Includes { }`
- **`appendIncludes` dropped the include path on MSVC** - emitted `/I` with no trailing path
- **`resolveDependencies` used hardcoded `-I` / `-L`** - now uses `comp->includeFlag()` and `/LIBPATH:` for MSVC
- **`resolveDependencies` read `cfg.verbose`** instead of the config being resolved (`c.verbose`)
- **`Generator::generate` signature** - changed `const Config &` → `Config &` so generators can populate `v_Info` with warnings
- **`--gen ninja` / `--gen cmake` / `--gen json`** now run `resolveDependencies` before generation, so fetched and dependent library include paths are correctly emitted into the generated files
- **`parseSection` uses `target.var` for `Set { }`** instead of the shared `cfg.var`
- **`fieldValue` takes `string_view`** (was `const std::string &`) - avoids a heap allocation per field lookup
- **Platform conditionals bypass the "first write wins" guard** - `win: Output: ...` after a fallback `Output: ...` now correctly overrides (previously the override was silently dropped)
- **Duplicate `Output:` debug print** in `--debug` - the `Found Config X` block no longer repeats the same field value when a platform-conditional override is present

### Changed
- **`--verbose` output cleaned up** - the config field listing is now a compact block per config (`Found Config X:` header with aligned `  Field: 'value'` lines) instead of a flat stream of `X found in config 'Y': 'value'` lines
- **`Info:` block ordering** - the `Info:` section (generated files, dependency links, generator warnings) now appears before the final "Compilation successful" line, and after the link step
- **Generator warnings** (`[Ninja] PCH not emitted`, `[CMake] Depends not emitted`, etc.) are no longer printed inline during parse, only in the consolidated `Info:` block
  
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
