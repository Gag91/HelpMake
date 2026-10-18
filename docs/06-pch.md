# Precompiled headers (PCH)

Precompiled headers are the fastest incremental-build optimization for C++ projects that include heavy headers (STL, boost, project-wide includes). HelpMake has first-class PCH support across GCC, Clang, MSVC, and both Zig C modes.

## Quick example

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
```

Then in every `.cpp`:

```cpp
#include "pch.hpp"
// … rest of the file
```

On the first build, HelpMake:

1. Compiles `include/pch.hpp` into `build/HelpMake/headers/gcc/pch.hpp.gch`
2. Adds `-include include/pch.hpp` to every subsequent compile command
3. Reuses the compiled header on every future build until it becomes stale

The savings grow with header size - for a header including `<vector>`, `<string>`, `<map>`, and `<regex>`, expect 2–5× faster compile times.

## How the cache works

Cache location:
```text
build/HelpMake/headers/<compiler-name>/<header-basename><pch-ext>
```

Examples:
```text
build/HelpMake/headers/gcc/pch.hpp.gch
build/HelpMake/headers/clang/pch.hpp.pch
build/HelpMake/headers/msvc/pch.hpp.pch
build/HelpMake/headers/zig-c++/pch.hpp.pch
build/HelpMake/headers/zig-cc/pch.hpp.pch
```
Per-toolchain, so switching `Compiler:` never reuses a foreign PCH. The old cache is left in place and skipped.

### Invalidation

HelpMake rebuilds the PCH when any of these are true:

- The header file (`PCH:` value) is newer than the compiled PCH
- Any dependency in the depfile for the PCH is newer
- `--rebuild` is passed
- `--clean` was run (deletes the entire `build/HelpMake/headers/` tree)

When it rebuilds, every source file that depends on the PCH also recompiles automatically (the object files are older than the new PCH).

## Per-toolchain extensions

| Toolchain | PCH extension | Injection flag | Notes |
|---|---|---|---|
| GCC | `.gch` | `-include <header>` | Uses header path, not PCH path |
| Clang | `.pch` | `-include-pch <pch>` | Uses PCH binary |
| MSVC | `.pch` | `/Yu<header> /Fp<pch>` | Source-file-based, requires first-include rule |
| Zig C++ | `.pch` | `-include-pch <pch>` | Same as Clang |
| Zig CC | `.pch` | `-include-pch <pch>` | Same as Clang, but `-x c-header` |
| Zig | - | - | Not supported (one-shot) |
| Generic | - | - | Not supported |

## Common pitfalls

### MSVC requires the header to be the first include
MSVC's PCH model is stricter than GCC/Clang. Every `.cpp` that uses the PCH must begin with:

```cpp
#include "pch.hpp"
```
with no `#include` before it. Otherwise the compile fails with:

```text
fatal error C1010: unexpected end of file while looking for precompiled header
```
HelpMake generates a synthetic `build/HelpMake/headers/msvc/pch.cpp` that includes the header, and passes `/Yc` + `/Yu` correctly - but the first-include rule is on your sources, not on HelpMake.

### GCC uses -include <header>, not -include-pch <pch>
For GCC, `-include-pch` doesn't exist. Instead:

- PCH is compiled to `pch.hpp.gch` next to `pch.hpp`
- GCC automatically finds and uses it when you `-include pch.hpp`

HelpMake handles this by passing `-include include/pch.hpp` (the header path), not `-include-pch <pch binary>`. This is a GCC-specific quirk that "just works" if you trust the mechanism.

### Zig C++ / Zig CC cache the PCH differently from Clang
Zig's toolchain is LLVM-based but its PCH validation is stricter. If you switch Zig's target triple, or update Zig itself, cached PCHs can become incompatible. If you see "PCH was compiled with a different compiler," run:

```bash
hm --clean
hm -b
```

### Changing flags invalidates the PCH
Adding `-DNDEBUG` or changing `-std=` invalidates the PCH because the preprocessor state changed. HelpMake tracks flag changes (via the compiler fingerprint) and rebuilds the PCH when it sees one.

## How to tell if PCH is working

Run with `--verbose`:

```bash
hm -b --verbose
```

You'll see:
```text
[HelpMake] PCH Command: g++ -std=c++20 -Iinclude -Wall -x c++-header include/pch.hpp -o build/HelpMake/headers/gcc/pch.hpp.gch
```
on the first build, or after invalidation. On subsequent builds you'll see the PCH step skipped and every source compile get `-include include/pch.hpp` added.

## When not to use PCH

- **One-shot builds (`--no-sep`):** PCH adds no value if you recompile everything anyway
- **Very small projects:** setup overhead (~1s) exceeds savings
- **Projects with many disjoint headers:** if no single header is included by >50% of sources, split into multiple PCH files or skip it

The best PCH covers 80%+ of your sources and includes only stable, rarely-changed headers (STL, third-party libraries). Project headers that change frequently belong in regular includes, not the PCH.
