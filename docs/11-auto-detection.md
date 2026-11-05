# Auto-detection

HelpMake can build a project **with no `HelpMake.txt` at all**. Every field is inferred from context. This page documents what's detected, in what order, and how to override it.

## Quick example

```bash
cd your-existing-project
hm -b
```

Output on first run (abbreviated):

```txt
Info:

  ⚠  [HelpMake] Warning: Compiler not specified. Auto-detected compiler: g++
  ⚠  [HelpMake] Warning: Version not specified. Auto-detected version flag: std=c++26
  ⚠  [HelpMake] Warning: Auto-added link flags:
      -pthread
  ⚠  [HelpMake] Warning: Input Files not specified. Auto-detected Input Files.
  ⚠  [HelpMake] Warning: Include dirs not specified. Auto-detected include dirs:
      include
  ⚠  [HelpMake] Warning: Output not specified. Auto-detected build/yourapp

Compilation successful. Output file: build/yourapp
```

Every detected value is listed under `Info:` so you know exactly what was inferred. To pin a value, add the field to `HelpMake.txt` - a user-specified value always wins over detection.

## Summary table

| Field | Detection strategy | Fallback |
| :--- | :--- | :--- |
| **Compiler:** | probes candidates by source extension | error |
| **Version:** | runs `<compiler> -dumpversion` | empty |
| **InputFiles { }** | walks `src/` (or `.`) recursively + follows includes | error |
| **Kind:** | scans sources for `main(` | executable |
| **Includes { }** | finds `include/`, `inc/`, `headers/`, `api/`, `public/` | empty |
| **link flags** | scans `#include <...>` lines | empty |
| **Output:** | reads 12 manifest formats + `main.*` + dir name | `a.out` / `a.exe` |

## Per-field details

### Compiler
Scans `InputFiles { }` (or auto-detected sources) for extensions, then probes candidates for that language group:

| Extension | Probe order |
| :--- | :--- |
| `.c` | gcc, clang, zig cc |
| `.cpp` / `.cxx` / `.cc` | g++, clang++, cl, zig c++ |
| `.zig` | zig |
| `.go` | go |
| `.rs` | rustc |
| `.py` | python3, python |
| `.java` | javac |
| `.cs` | csc, mcs |
| `.ts` | tsc |
| `.js` | node |
| `.rb` | ruby |
| `.php` | php |
| `.pl` | perl |
| `.swift` | swiftc |
| `.kt` / `.kts` | kotlinc |
| `.hs` | ghc |
| `.lua` | lua |
| `.r` | Rscript |
| `.jl` | julia |
| `.dart` | dart |
| `.scala` | scalac |
| `.clj` | clojure |
| `.erl` | erlc |
| `.ex` / `.exs` | elixir |
| `.fs` / `.fsi` / `.fsx` | fsharpc, dotnet fsi, dotnet fsc |
| `.ml` / `.mli` | ocamlc, ocamlopt |
| `.vbs` | cscript |
| `.bat` / `.cmd` | cmd.exe |
| `.ps1` | powershell.exe, pwsh.exe |

The first candidate that exists on `PATH` wins.

### Version
Runs `<compiler> -dumpversion`, parses the major version, and picks the newest supported standard flag:

**C++ (GCC/Clang minimums):**

| Flag | GCC | Clang |
| :--- | :--- | :--- |
| `std=c++14` | 5 | 3 |
| `std=c++17` | 8 | 7 |
| `std=c++20` | 11 | 12 |
| `std=c++23` | 14 | 17 |
| `std=c++26` | 16 | 22 |

**C:**

| Flag | GCC | Clang |
| :--- | :--- | :--- |
| `std=c99` | 5 | 3 |
| `std=c11` | 8 | 6 |
| `std=c17` | 14 | 18 |
| `std=c23` | 14 | 18 |

* For MSVC, defaults to `/std:c++latest`.
* For Zig toolchains, detects the LLVM version that Zig ships and uses the same Clang minimums.

### Input files
If no `src/`, `source/`, `code/`, or `app/` directory exists, walks `.` recursively. Otherwise walks the first one found (plus the project root for top-level sources).

**Excluded directories:** `.git`, `.hg`, `.svn`, `.vs`, `.cache`, `.idea`, `.gitlab`, `.github`, `build`, `bin`, `obj`, `out`, `dist`, `target`, `node_modules`, `vendor`, `third_party`, `external`, `deps`, `dep`, `lib`, `libs`, `include`, `includes`, `test`, `tests`, `examples`, `demos`, `playground`, `scratch`, `tmp`, `temp`.

Sources are filtered by the detected compiler's language group - a `g++` build only picks up C++ extensions, a `gcc` build only picks up C, etc.

**Companion sources.** After the initial scan, HelpMake reads every `#include "..."` from each source and searches the tree for a matching `X.cpp` (or `.cc`, `.cxx`, `.c`, `.cppm`). If found and not already listed, it's added. *Example:* `src/main.cpp` includes `"utils.hpp"`, and `tools/utils.cpp` exists - `tools/utils.cpp` is added to the build.

### Kind
Scans every C-family source for a valid `main(` declaration, with word-boundary checks (`domain(`, `mainHelper(`, `main_loop(` are rejected). Accepts `main()`, `main(int argc, char** argv)`, `main(void)`, `int main ( int argc , ... )`.

If found: `Kind: executable`. If not: `Kind: static`, with a warning. Non-C-family projects (Zig, Go, Rust, Python) always default to `executable`.

### Includes
Scans recursively for directories named `include`, `includes`, `inc`, `headers`, `api`, or `public`. Reason-based recursion stops at the first match per branch, so `include/hm/parser/` and `include/hm/cli/` are not added as separate include roots.

If no named include dir exists, falls back to adding the parent directory of every header file (`.h`, `.hpp`, `.hxx`, `.hh`, `.inl`, `.inc`, `.ixx`, `.cppm`, `.tpp`). This is the *"I have headers in src/"* case.

### Link flags
Reads every `#include <...>` line in every source and adds the corresponding flag:

| Header | GCC/Clang | MSVC |
| :--- | :--- | :--- |
| `<thread>`, `<mutex>`, `<atomic>`, `<condition_variable>`, `<pthread.h>` | `-pthread` | *(skip - CRT built-in)* |
| `<cmath>`, `<math.h>` | `-lm` | *(skip - CRT built-in)* |
| `<dlfcn.h>` | `-ldl` | *(skip - POSIX-only)* |
| `<zlib.h>` | `-lz` | - |
| `<curl/curl.h>` | `-lcurl` | - |
| `<openssl/ssl.h>` | `-lssl -lcrypto` | - |

Existing flags are never duplicated. Only the discovered flags appear under `Warning: Auto-added link flags:`.

MSVC `.lib` detection only works if the `.lib` is already in a directory `link.exe` searches - auto-detection tells you what to install, not where it is. Use `Flags { /LIBPATH:C:\vcpkg\lib }` to point at it.

### Output
Reads in priority order:
1. `Cargo.toml` - `[package] name = "..."`
2. `go.mod` - `module github.com/user/app` (last path component)
3. `package.json` - `"name": "..."`
4. `pyproject.toml` - `[project]` or `[tool.poetry] name = "..."`
5. `setup.py` - `name="..."`
6. `build.zig` - `addExecutable(.{ .name = "...", ... })`
7. `meson.build` - `project('...', ...)`
8. `CMakeLists.txt` - `project(...)`
9. `pom.xml` - `<artifactId>...</artifactId>`
10. `build.gradle` / `build.gradle.kts` - `rootProject.name = "..."`
11. `*.csproj` / `*.fsproj` / `*.vbproj` - file stem
12. `main.*` in `InputFiles { }` - uses `main`
13. **Directory name** - the current directory's basename
14. **Fallback** - `a.out` (Unix) or `a.exe` (Windows)

On Windows, `.exe` is appended. Output is placed in the current directory; use `Output: build/...` to change that.

## Overriding any field

Just write the field. Anything you set wins over auto-detection.

```txt
Output: build/app-release
Compiler: clang++
Version: std=c++23
```

You can also override from the CLI without touching `HelpMake.txt`:

```bash
hm -b -C=clang++ -V=std=c++23 -o build/app-release
```

## Disabling auto-detection entirely

Run with `--nofile`:

```bash
hm -b --nofile -C=g++ -V=std=c++20 -o app src/main.cpp
```

`--nofile` skips the config file and the auto-detection chain - only what you pass on the CLI is used.

## The Info: block

Every auto-detected value warns under `Info:`:

```txt
Info:

  ⚠  [HelpMake] Warning: Compiler not specified. Auto-detected compiler: g++
  ⚠  [HelpMake] Warning: Version not specified. Auto-detected version flag: std=c++26
  ⚠  [HelpMake] Warning: Auto-added link flags:
      -pthread
  ⚠  [HelpMake] Warning: Output not specified. Auto-detected build/app
```

Multi-line warnings (`Auto-added link flags:`, `Auto-detected include dirs:`, `Auto-added companion sources:`) sort to the bottom of the warning group. Continuation lines align under the message text.

Use `--quiet` to suppress everything except errors. Use `--verbose` to also see detected successes.

## When auto-detection fails

| Error | Cause | Fix |
| :--- | :--- | :--- |
| `No input files found` | no sources in `src/` or `.` | add `InputFiles { }` or place sources in `src/` |
| `No compiler known for extension '.xyz'` | unknown language | set `Compiler:` explicitly |
| `No compiler found for 'C++' files` | none of g++, clang++, cl, zig c++ on PATH | install one, or set `Compiler:` |
| `Undefined variable` | typo in `[[name]]` | fix the variable name or define it in `Set { }` |

For any other detection failure, run `hm -b --debug` to see which file and which check failed.
