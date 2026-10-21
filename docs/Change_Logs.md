# Changelog

All notable changes to HelpMake.

## [11.1.0] - 2026-10-21

### Added
- `Dependency: name { }` blocks — fetch, build, and link external libraries
  - `Git:`, `Url:`, `Path:` sources
  - `Tag:`, `Commit:`, `Hash:` version pinning
  - `Build:` shell command list (repeatable) with `.buildid` cache
  - `Include:`, `Link:`, `LinkDir:`, `LinkFlags:` integration
  - Cache lives at `build/HelpMake/dep/<name>/`
- `Depends:` field on configs — pulls dependency blocks into a build
- Go toolchain (`go build`) with `-Go` CLI flag
- `hm --gen ninja` — emit a `build.ninja` file
- `hm --gen` with no argument lists available generators

### Changed
- GCC/Clang now select `gcc`/`clang` for `.c` sources and add `-xc` (previously all sources were forced into C++ mode via `g++ -xc++`)
- `executeConfig` treats `.a`, `.lib`, and `.so` path arguments as link inputs
- `Parser::parse()` refactored into a section splitter plus a per-section parser

### Fixed
- CLI flags (`-o`, `-C=`, `-V=`, `--Kind=`, `--var`) now correctly override values in `HelpMake.txt` — previously the file value always won
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
