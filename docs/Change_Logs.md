# Changelog

All notable changes to HelpMake.

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
