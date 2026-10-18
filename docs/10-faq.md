# FAQ

## Why is my project rebuilding everything on every run?

Four common causes, in order of likelihood:

1. **You changed `Compiler:` or flags.** HelpMake invalidates the entire per-compiler `obj/` directory when it detects a toolchain or flag change. Check `build/HelpMake/<compiler>/obj/` - if it's empty and you didn't `--clean`, that's why. Once rebuilt, subsequent runs with the same flags skip recompilation.

2. **Your `.d` files are missing or corrupted.** Header tracking relies on depfiles. If `build/HelpMake/<compiler>/obj/main.d` is missing, `main.cpp` recompiles. Run `hm --verbose` and look for `Warning: Could not open dependency file`.

3. **A header is being touched on every build.** Some code generators rewrite headers with new timestamps even when content is identical. Check which header with:
   ```bash
   hm -b --debug 2>&1 | grep -i "newer than"
   ```
   The debug output names the header that triggers the rebuild.

4. **You're running `hm --clean` in CI or a script.** `--clean` deletes `build/HelpMake/` including all objects. Look at your build script.

If all four are ruled out, run `hm -b --debug --rebuild` once, then `hm -b --debug`. The second build should show `Skip (up to date)` for every file. If not, paste the output in a GitHub issue.

## Why does changing Compiler: trigger a full rebuild?

Because object files from different compilers are not link-compatible. `g++` objects contain symbols mangled for `libstdc++`; `zig c++` objects contain symbols mangled for `libc++`. Linking them together either fails with hundreds of undefined references, or (worse) links but crashes.

HelpMake prevents this by construction: each compiler writes to `build/HelpMake/<compiler>/obj/`. When you switch compilers, the new `obj/` is empty, so every file recompiles. This is intentional, not a bug.

You can safely mix toolchains across configs, because each config picks its own compiler and gets its own object tree. You cannot mix them within a single config.

## Why doesn't src/**/*.cpp work when I pass it to g++ directly?

Because `**` is not shell globbing. In bash it's only valid with `globstar` enabled:

```bash
shopt -s globstar
g++ -std=c++20 src/**/*.cpp -o app -Iinclude
```

In `cmd` and PowerShell it never works. In `g++` itself, it's just a literal filename.

Inside `HelpMake.txt`, `**` works regardless of shell:

```txt
InputFiles {
    src/**/*.cpp
}
```
HelpMake expands the glob internally, so it doesn't matter whether you're on Windows `cmd`, PowerShell, or Linux.

## What's the difference between -ZigCpp and -ZigCC?

Both use Zig's LLVM-based toolchain:

- `-ZigCpp` invokes `zig c++` - C++ mode, links against Zig's bundled `libc++`, uses `-x c++-header` for PCH
- `-ZigCC` invokes `zig cc` - C mode, links against Zig's bundled `libc`, uses `-x c-header` for PCH

Use `-ZigCpp` for `.cpp` files and `-ZigCC` for `.c` files. If you mix extensions in one project, pick whichever handles most of your sources correctly and use `.cpp`/`.c` extensions that match.

## Why does Kind: static fail with --no-sep?

`--no-sep` tells HelpMake to pass all sources to the compiler in one shot:

```text
g++ -c a.cpp b.cpp c.cpp -o app
```
That produces one binary directly, with no intermediate `.o` files. A static library archive is built from object files, so `Kind: static` requires separate compilation (each `.cpp` gets its own `.o`).

HelpMake rejects this combination with:

```text
Error: 'Kind: static' requires separate compilation. Remove --no-sep.
```
Remove `--no-sep` for static library builds.

## MSVC PCH: "unexpected end of file while looking for precompiled header"

MSVC requires the PCH header to be the first `#include` in every `.cpp` file that uses it:

```cpp
#include "pch.hpp"    // ← must be first
#include <iostream>   // ← this comes after
```

If any `#include` precedes `pch.hpp`, MSVC emits:

```text
fatal error C1010: unexpected end of file while looking for precompiled header
```
HelpMake generates the synthetic `pch.cpp` with `#include "pch.hpp"` on the first line, but it cannot reorder your sources. Ensure every `.cpp` in the project follows the first-include rule, or remove `PCH:` from the config.

## Can I build the same project with different compilers simultaneously?

Yes, using configs:

```txt
Compiler: g++
InputFiles { src/*.cpp }

Config: native {
    Compiler: clang++
    Output: build/native.exe
}

Config: portable {
    Compiler: zig c++
    Output: build/portable.exe
}
```

Each config has its own compiler and its own object tree (`build/HelpMake/gcc/`, `build/HelpMake/clang/`, `build/HelpMake/zig-c++/`). Build them:

```bash
hm -b --config native
hm -b --config portable
```
The two builds never interfere. Both binaries are produced from the same source.

## Why are there "undefined reference to std::__1::basic_string" errors?

You're linking `libc++`-compiled objects with a `libstdc++` linker (or vice versa). Symptom: every error mentions `std::__1::` (that's `libc++`, used by Clang and Zig) or `std::` without `__1` (that's `libstdc++`, used by GCC).

Cause: mixing objects from `g++` and `clang++` / `zig c++`. HelpMake prevents this by isolating object directories per compiler. If you - this error:

1. Run `hm --clean` to remove all objects
2. Run `hm -b` once and let it build with the selected compiler
3. Don't manually copy `.o` files between `build/HelpMake/gcc/obj/` and `build/HelpMake/clang/obj/`

If the error persists after `--clean`, check your `HelpMake.txt` for multiple `Compiler:` directives that might be overriding each other.

## How do I - the exact compiler command being run?

```bash
hm -b --verbose
```

Every compile and link command prints:

```text
[HelpMake] Object Command: g++ -std=c++20 -Iinclude -Wall -fdiagnostics-color=always -xc++ -c src/main.cpp -o build/HelpMake/gcc/obj/main.o -MMD
[HelpMake] Link Command: g++ -std=c++20 -Iinclude -Wall build/HelpMake/gcc/obj/main.o -o build/app.exe
```
Use `--debug` for even more detail (parser trace, config discovery, flag splits).

## How do I use HelpMake as a library?

`include/HelpMake.hpp` provides a `hp::HelpMake` class:

```cpp
#include "hp/help.hpp"

int main() {
    hp::HelpMake()
        .setCompiler("g++")
        .setStandard("c++20")
        .setOutput("build/app")
        .addInputFile("src/*.cpp")
        .addInclude("include")
        .addFlag("-Wall")
        .build();
    return 0;
}
```
The API mirrors every `HelpMake.txt` field. - `include/HelpMake.hpp` for the full list of `setX` and `addX` methods.

## What's the difference between --clean, --clean-gcm, and --clean-all?

| Command | Deletes |
|---|---|
| `hm --clean` | `build/HelpMake/` (objects, logs, PCH, deps) |
| `hm --clean-gcm` | `gcm.cache/` (GCC module cache) |
| `hm --clean-all` | Both |

`--clean-gcm` is separate because GCC puts `gcm.cache/` in the project root, not under `build/HelpMake/`. Most other toolchains don't need it.

## Can HelpMake compile C?

Yes. Two ways:

**Option 1: GCC/Clang (C mode)**
```txt
Compiler: gcc
Version: std=c17
InputFiles { src/*.c }
```

**Option 2: Zig CC**
```txt
Compiler: zig cc
Version: std=c17
InputFiles { src/*.c }
```
The `Version:` field accepts `std=c17`, `std=c23`, etc. HelpMake passes `-std=c17` to the compiler.

## How do I add a new toolchain?

- the `hm::Compiler` interface in `include/hm/toolchain/compiler.hpp`. Contributing requires:

1. Subclass `hm::Compiler` with your toolchain's flag shapes
2. Register it in `makeCompiler()` in `src/toolchain/toolchain.cpp`
3. Add a `-YourName` flag in `src/cli/build.cpp` if it deserves a shortcut
4. Document it in `docs/04-toolchains.md`

Open a PR - toolchain additions are welcome.

## How do I report a bug?

Open an issue on GitHub with:

- The exact `hm` command you ran
- The contents of `HelpMake.txt` (redact anything private)
- The `--verbose` output showing the failing command
- The log from `build/HelpMake/<compiler>/logs/<output>.txt`
- Your OS and compiler versions

Reproducible cases get fixed fastest. If you can, minimize the project to the smallest failing example.
Can HelpMake compile C?
Yes. Two ways:

**Option 1: GCC/Clang (C mode)**

```txt
Compiler: gcc
Version: std=c17
InputFiles { src/*.c }
```

**Option 2: Zig CC**

```txt
Compiler: zig cc
Version: std=c17
InputFiles { src/*.c }
```
The `Version:` field accepts `std=c17`, `std=c23`, etc. HelpMake passes `-std=c17` to the compiler.

## How do I add a new toolchain?
- the `hm::Compiler` interface in `include/hm/toolchain/compiler.hpp`. Contributing requires:

1. Subclass `hm::Compiler` with your toolchain's flag shapes
2. Register it in `makeCompiler()` in `src/toolchain/toolchain.cpp`
3. Add a `-YourName` flag in `src/cli/build.cpp` if it deserves a shortcut
4. Document it in `docs/04-toolchains.md`

Open a PR - toolchain additions are welcome.

## How do I report a bug?
Open an issue on GitHub with:

- The exact `hm` command you ran
- The contents of `HelpMake.txt` (redact anything private)
- The `--verbose` output showing the failing command
- The log from `build/HelpMake/<compiler>/logs/<output>.txt`
- Your OS and compiler versions

Reproducible cases get fixed fastest. If you can, minimize the project to the smallest failing example.
