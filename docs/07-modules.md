# C++20 modules

HelpMake supports C++20 modules with GCC and Clang (not MSVC, not the Zig toolchains). Modules compile **before** regular sources so consumers always find the precompiled interface.

## Quick example

```txt
Compiler: clang++
Version: std=c++20
Output: build/app.exe

InputFiles {
    src/main.cpp
}

Modules {
    src/math.cppm
    src/graphics.ixx
}
```

`src/math.cppm`:
```cpp
export module math;

export int add(int a, int b) {
    return a + b;
}
```

`src/main.cpp`:
```cpp
import math;
#include <iostream>

int main() {
    std::cout << math::add(2, 3) << "\n";
}
```

Build:
```bash
hm -b
```

## GCC workflow

GCC compiles a module interface unit in a single `-c` step that produces both:

- `build/HelpMake/gcc/obj/math.o` - the object file
- `gcm.cache/math.gcm` - the module interface cache

Subsequent consumers get `-fmodule-file=math=gcm.cache/math.gcm` implicitly via the default search path.

GCC requires `-fmodules-ts`; HelpMake adds it automatically when `Modules { }` is non-empty.

## Clang workflow

Clang uses a two-step process:

1. `--precompile` produces `build/HelpMake/clang/modules/math.pcm`
2. `-c` produces `build/HelpMake/clang/obj/math.o`

Consumers get `-fmodule-file=math=build/HelpMake/clang/modules/math.pcm` for every module in the config.

Clang PCM cache lives at `build/HelpMake/clang/modules/` and is per-config (different flags \(\rightarrow\) different PCMs).

## Supported extensions

Listed under `Modules { }`, any of these are recognized as module interface units:

- `.cppm`
- `.ixx`
- `.mpp`
- `.cxxm`
- `.c++m`
- `.ccm`

Any file listed under `Modules { }` is treated as a module, even if it has a `.cpp` extension. HelpMake adds the appropriate `-xc++-module` (Clang) or `-xc++` (GCC) flag automatically.

This means you can write:
```txt
Modules {
    src/math_interface.cpp   # treated as a module despite extension
}
```
and HelpMake passes `-xc++-module` so Clang parses it correctly.

## MSVC workflow

MSVC compiles `.ixx` files as module interface units with `/interface /TP`. Any extension works — `/TP` forces C++ mode regardless of filename, so `.cppm` and `.mpp` are fine too.

```bat
cl.exe /std:c++20 /interface /TP /ifcOutputbuild/HelpMake/msvc/modules/math.ifc ^
    /nologo /c test/math.ixx /showIncludes /Fo:build/.../math.ixx.obj
```
The .ifc goes to `build/HelpMake/msvc/modules/` (via /ifcOutput). Consumers get `/reference math=build/HelpMake/msvc/modules/math.ifc` and resolve the import from there.
Modules compile sequentially before any regular source file, so consumers never start before their .ifc exists.
**Not supported**: module partitions (`export module A:P;`) - MSVC needs /internalPartition on the partition file and per-partition /reference on consumers.

## Build order

HelpMake partitions files into modules first, then regular sources:

```text
src/math.cppm      ← compiled first
src/graphics.ixx   ← compiled first
src/main.cpp       ← compiled after (can import math, graphics)
src/util.cpp       ← compiled after
```

Modules that import other modules still get compiled after their dependencies - HelpMake doesn't currently do topological sort, so if you have `module A; import B;` you need to list `B` before `A` in the file (or accept the resulting compile error and reorder).

## Cleaning module caches

GCC's `gcm.cache/` lives in your project root (not under `build/`) by GCC's design. Clean it explicitly:

```bash
hm --clean-gcm     # just gcm.cache/
hm --clean-all     # gcm.cache/ AND build/HelpMake/
```
The normal `hm --clean` does not delete `gcm.cache/` because GCC's cache is outside the `build/HelpMake/` tree.

## Limitations

| Limitation | Reason | Workaround |
|---|---|---|
| Basic MSVC modules work; partitions don't |
| No Zig toolchain modules | Zig's C++ mode doesn't expose module flags | Use `-C=clang++` |
| No topological sort | Simple file-order compile | List dependencies before dependents |
| No module partitions | `export module foo:part;` not yet parsed | Use flat modules |
| No module import in the same TU | `import` must be at file top | Standard C++ rule |

## Debugging modules

Enable verbose mode:
```bash
hm -b --verbose
```

You'll see the module compile command with the full flag set:
```text
[HelpMake] Object Command: clang++ -std=c++20 -fmodules-ts -xc++-module -c src/math.cppm -o build/HelpMake/clang/obj/math.o -MMD -fmodule-file=math=build/HelpMake/clang/modules/math.pcm
```

And the link line consuming the object:
```text
[HelpMake] Link Command: clang++ -std=c++20 build/HelpMake/clang/obj/math.o build/HelpMake/clang/obj/main.o -o build/app.exe
```

If a consumer fails with "module 'math' not found":

- Check `Modules { }` includes `math.cppm`
- Check that `math.cppm` compiled successfully (look for `math.pcm` on disk)
- Try `hm --clean-gcm` and `hm --clean` and rebuild from scratch

## Migration note

C++20 modules are not yet supported by all standard libraries. As of GCC 14 and Clang 17:

- `<iostream>`, `<vector>`, `<string>` are importable as `std.iostream`, `std.vector`, `std.string` only if your standard library ships module units (libc++ 18+ does; libstdc++ does not yet)
- Most projects still use `#include` for standard headers and modules only for their own code

For now, treat modules as a feature for your own project's modularization, not for dropping `#include <...>` entirely.
