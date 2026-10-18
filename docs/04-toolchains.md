# Supported toolchains

HelpMake has native support for five toolchain families plus a fallback for anything else.

## Summary matrix

| Toolchain | Binary | Object ext | PCH | Modules | Static libs | Separate compile |
|---|---|---|---|---|---|---|
| GCC | `g++` | `.o` | ✅ `.gch` | ✅ | ✅ `ar` | ✅ |
| Clang | `clang++` | `.o` | ✅ `.pch` | ✅ | ✅ `ar` | ✅ |
| MSVC | `cl.exe` | `.obj` | ✅ `.pch` | ❌ | ✅ `lib.exe` | ✅ |
| Zig | `zig` | *(none)* | ❌ | ❌ | ❌ | ❌ |
| Zig C++ | `zig c++` | `.o` | ✅ `.pch` | ❌ | ✅ `ar` | ✅ |
| Zig CC | `zig cc` | `.o` | ✅ `.pch` | ❌ | ✅ `ar` | ✅ |
| Generic | any | `.o` | ❌ | ❌ | ✅ `ar` | ✅ |

## Per-toolchain notes

### GCC

The default. Uses `g++`, writes `.o` objects, uses `-include` to inject PCH headers, uses `-MMD` for dependency tracking, produces static libs with `ar rcs`.

Compile command shape:
```text
g++ -std=c++20 -Iinclude -Wall -fdiagnostics-color=always -xc++ -c src/main.cpp -o build/HelpMake/gcc/obj/main.o -MMD
```

`-xc++` forces C++ mode regardless of extension (useful for module files with non-standard extensions).

### Clang

Identical to GCC except:

- `clang++` as binary
- `.pch` extension for PCH (not `.gch`)
- `-include-pch <file>` for PCH injection (not `-include <header>`)
- Modules use `-fmodule-file=<name>=<path>` (PCM cache at `build/HelpMake/clang/modules/`)
- No `-xc++` on regular source files (Clang infers C++ from `.cpp`)

### MSVC

Windows only. Uses `cl.exe` and `lib.exe`. Because MSVC's model is source-file-based (PCH requires generating a synthetic `.cpp`), MSVC uses `pchNeedsSourceFile() = true`:

- `pchCreateArgs` returns `{" /c", "/Yc<header>", "/Fp<pch>"}` and writes `build/HelpMake/headers/msvc/pch.cpp` containing `#include "<header>"`
- `pchUseArgs` returns `{"/Yu<header>", "/Fp<pch>"}` for every consumer source
- Object output uses `/Fo:<path>` (MSVC's colon form)
- Executable output uses `/Fe:<path>`
- No dep file (`/showIncludes` output isn't parsed — incremental builds rely on mtimes only)

MSVC incremental builds are less precise than GCC/Clang because header changes trigger rebuilds only via mtime comparison of the source file, not through a depfile. `--rebuild` is your friend when in doubt.

### Zig

Standalone Zig (`.zig` sources). One-shot only — `supportsSeparate() = false` because Zig's `build-exe` produces a binary directly with no intermediate object step.

Command shape:
```text
zig build-exe src/main.zig -femit-bin=build/app
```

No PCH, no modules, no static libraries. Use one of the C-family Zig modes below if you need those features.

### Zig C++

Zig's bundled C++ toolchain (`zig c++`). Behaviorally identical to Clang (Zig uses LLVM Clang under the hood) but links against Zig's bundled `libc++` instead of the system one.

**Important:** Zig C++ and GCC objects are **not** link-compatible — Zig's `libc++` has a different ABI from GNU's `libstdc++`. HelpMake isolates them per compiler (`build/HelpMake/zig-c++/obj/`), so switching `Compiler: g++` → `Compiler: zig c++` never accidentally links mixed objects.

Command shape:
```text
zig c++ -std=c++20 -Iinclude -Wall -fdiagnostics-color=always -c src/main.cpp -o build/HelpMake/zig-c++/obj/main.o -MMD
```

The `zig` binary is invoked with the `c++` subcommand — that's why `Compiler:` accepts `zig c++`, `zig-c++`, `zigcpp`, and `zig-cpp` as equivalent.

Cold-start builds are slow (10–60s on first invocation) while Zig populates its global cache (`%LOCALAPPDATA%\zig\` on Windows, `~/.cache/zig` on Linux). Subsequent builds are fast.

### Zig CC

Zig's C mode (`zig cc`). Same architecture as Zig C++, but uses `-x c-header` for PCH and links against libc.

Command shape:
```text
zig cc -std=c17 -Iinclude -Wall -fdiagnostics-color=always -c src/main.c -o build/HelpMake/zig-cc/obj/main.o -MMD
```

Same cold-start caveat as Zig C++.

### Generic

Any compiler not listed above falls through to `GenericCompiler`:

- Binary = the name you gave
- Object extension `.o`
- Standard `-c` + `-o` for compile and link
- No PCH, no dep files, no modules
- Static libraries use `ar rcs`

This is your escape hatch for `nasm`, `ghc`, `dmd`, `rustc`, `swiftc`, and anything else that respects a `-o` output flag. If your compiler's flags don't match, use `-F` to add custom ones or file an issue to add a proper toolchain.

## Choosing a toolchain

| Situation | Recommendation |
|---|---|
| Default C/C++ project | **GCC** or **Clang** |
| Windows, must ship `.exe` compatible with MSVC libraries | **MSVC** |
| Zero-dependency single binary, want latest LLVM | **Zig C++** |
| C project, want latest Clang without installing Clang | **Zig CC** |
| Writing Zig | **Zig** |
| Exotic language or custom compiler | `-C=<name>` (Generic) |

## Mixing toolchains

You **cannot** link objects produced by different toolchain families. A `.o` from `g++` will not link with a `.o` from `clang++` on Windows (different C++ runtime), nor with `zig c++` output (different standard library).

HelpMake prevents this by construction:

- Each toolchain writes to `build/HelpMake/<name>/obj/`
- Switching `Compiler:` finds an empty `obj/` and recompiles everything
- Logs also live per-compiler, so `build/HelpMake/gcc/logs/` never confuses with `build/HelpMake/zig-c++/logs/`

You **can** mix freely across `Config:` blocks — each config picks its own compiler and gets its own object tree:

```txt
Compiler: g++
InputFiles { src/*.cpp }

Config: portable {
    Compiler: zig c++
    Output: build/portable.exe
}

Config: native {
    Compiler: clang++
    Output: build/native.exe
}
```
Both configs share source files, produce separate object trees, and link into separate binaries.

## Adding a new toolchain

Contributing a toolchain requires implementing `hm::Compiler` (see `include/hm/toolchain/compiler.hpp`) and registering it in `makeCompiler()`. See the contributing guide.
