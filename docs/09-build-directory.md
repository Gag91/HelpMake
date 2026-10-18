# Build directory layout

HelpMake writes everything under `build/HelpMake/`. Nothing in your project root is touched except the final output file.

## The tree

```text
build/HelpMake/
├── gcc/                ← objects and logs for the GCC toolchain
│   ├── obj/
│   │   ├── main.o
│   │   ├── main.d
│   │   ├── util.o
│   │   └── util.d
│   └── logs/
│       ├── app.txt     ← clean log for the "app" output
│       └── app.raw     ← raw ANSI log
├── clang/
│   ├── obj/
│   ├── modules/        ← C++20 module PCMs
│   └── logs/
├── msvc/
│   ├── obj/            ← .obj files
│   └── logs/
├── zig-c++/
│   ├── obj/
│   └── logs/
├── zig-cc/
│   ├── obj/
│   └── logs/
├── headers/            ← PCH caches, one per toolchain
│   ├── gcc/
│   │   └── pch.hpp.gch
│   ├── clang/
│   │   └── pch.hpp.pch
│   ├── msvc/
│   │   ├── pch.hpp.pch
│   │   └── pch.cpp     ← synthetic source for MSVC
│   ├── zig-c++/
│   └── zig-cc/
└── dep/                ← git-fetched dependencies
    └── <repo-name>/
```

## Per-compiler isolation

Every toolchain gets its own subtree: `build/HelpMake/gcc/`, `build/HelpMake/clang/`, `build/HelpMake/msvc/`, `build/HelpMake/zig-c++/`, `build/HelpMake/zig-cc/`.

**Why this matters:** object files from different compilers are not link-compatible. A `.o` from `g++` links with `libstdc++`; a `.o` from `zig c++` links with `libc++`. Linking them together fails with hundreds of "undefined reference to `std::__1::basic_string`" errors - or worse, links but crashes at runtime.

By isolating object directories per compiler:

- Switching `Compiler: g++` → `Compiler: zig c++` in `HelpMake.txt` finds an empty `build/HelpMake/zig-c++/obj/` and recompiles everything
- No manual `--rebuild` or `hm --clean` needed
- Two configs using different compilers can build in the same project without stepping on each other

## Log locations

Per-target logs:
```text
build/HelpMake/<compiler>/logs/<output-base>.txt
```

Where `<output-base>` is the output path's basename without extension. For `Output: build/app.exe`, the log is `build/HelpMake/gcc/logs/app.txt`.

Two files per build:

- `.txt` - ANSI color codes stripped; safe for `grep`, `less`, CI artifact upload
- `.raw` - the raw compiler output including escapes

When a build fails, HelpMake prints the log path in the error message:
```text
Compilation Failed
See Logs: build/HelpMake/gcc/logs/app.txt
```

## Fetched dependencies

`Fetch { }` clones repos into:
```text
build/HelpMake/dep/<repo-name>/
```

Where `<repo-name>` is derived from the URL's last path segment (with `.git` stripped). For `https://github.com/glfw/glfw.git`, the directory is `build/HelpMake/dep/glfw/`.

Repos are cloned once. If the directory already exists, HelpMake **does not** re-clone - delete it manually if you need to update:

```bash
rm -rf build/HelpMake/dep/glfw
hm -b
```

## Recommended .gitignore

Add to your project's `.gitignore`:

```gitignore
# HelpMake build artifacts
build/
gcm.cache/
compile_commands.json

# C++20 module caches
*.pcm
*.gcm
```
The `build/` entry covers everything in this doc - no need to enumerate `build/HelpMake/gcc/obj/` etc.

## Cleaning commands

| Command | Deletes |
|---|---|
| `hm --clean` | `build/HelpMake/` (all compilers, all configs, all PCHs, all deps) |
| `hm --clean-gcm` | `gcm.cache/` (GCC module cache - lives outside `build/`) |
| `hm --clean-all` | Both of the above |

`hm --clean` is safe to run at any time. It does not touch your source files, your `HelpMake.txt`, or your final output binary (which lives wherever `Output:` points - typically `build/app.exe`, outside `build/HelpMake/`).

To delete the final binary too:

```bash
hm --clean
rm -f build/app.exe
```

## Migration from pre-11.0

Before version 11.0, objects lived in `build/HelpMake/obj/` (flat, no per-compiler isolation). If you upgraded from 10.x, you'll find stale objects there. Run:

```bash
hm --clean
hm -b
```
The old `build/HelpMake/obj/` directory is safely removed by `--clean`. New builds write to `build/HelpMake/<compiler>/obj/` and never look at the old path.

No manual migration is needed. The per-compiler layout is created on demand - the first build after upgrade will simply see no objects and recompile everything.
