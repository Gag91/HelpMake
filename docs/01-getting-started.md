# Getting started

This guide walks you through building your first project with HelpMake in under five minutes. You can start with **no config file at all** - HelpMake will detect everything.

## 1. Install HelpMake

Follow the [installation guide](../README.md#installation) in the main README. Once `hm` is on your PATH, verify it:

```bash
hm --version
```

You should see:

```text
Help_Make (hm) - Build System
Version: 12.4.0
...
```

## 2. Try it with zero config

`cd` into any existing C/C++ project (or create an empty directory with a `main.cpp`) and run:

```bash
hm -b
```

HelpMake detects:

- **Compiler** - probes `gcc`, `clang`, `g++`, `clang++`, `cl`, etc. based on your source extensions
- **Version** - runs `<compiler> -dumpversion` and picks the newest standard it supports
- **Input files** - walks `src/` (or `.`), excluding `.git/`, `build/`, `node_modules/`, `test/`, etc.
- **Includes** - finds `include/`, `inc/`, `headers/`, `api/`, `public/`
- **Link flags** - reads `#include <thread>` → `-pthread`, `<cmath>` → `-lm`, etc.
- **Kind** - scans for `main(` and picks `executable` or `static`
- **Output** - reads `Cargo.toml`, `go.mod`, `package.json`, `CMakeLists.txt`, `main.*`, or the directory name

Every detected value appears under `Info:` so you know exactly what was inferred. If you don't like what it picked, add a `HelpMake.txt` and pin the field.

## 3. Optional: write a minimal `HelpMake.txt`

If you want control, one file describes your whole build:

```txt
Compiler: g++
Version: std=c++20
Output: build/app.exe

InputFiles {
    src/*.cpp
}

Includes {
    include
}

Flags {
    -Wall
    -O2
}
```

This tells HelpMake to:

- Use g++ with `-std=c++20`
- Compile every `.cpp` directly under `src/` (use `**` for subfolders)
- Add `include/` to the include path
- Write the final executable to `build/app.exe`
- Pass `-Wall` and `-O2` to every compile and link step

Anything you set is used exactly as written - auto-detection is skipped for that field.

## 4. Create a source file

```cpp
#include <iostream>

int main() {
    std::cout << "Hello, HelpMake!\n";
    return 0;
}
```

## 5. Build

```bash
hm -b
```

The first build compiles every `.cpp` file and links the result. You should see:

```text
Compilation successful. Output file: build/app.exe
```

## 6. Rebuild

Run `hm -b` again:

```text
[HelpMake] Skip (up to date): src/main.cpp
Compilation successful. Output file: build/app.exe
```

Nothing recompiled. Edit `src/main.cpp` and run `hm -b` again - only `main.cpp` recompiles before the link step runs.

## 7. Watch mode

Instead of running `hm -b` after every edit:

```bash
hm -b --watch
```

HelpMake builds once, then watches every source, every directory listed in `Includes { }` (recursively), and `HelpMake.txt` itself. On save, it rebuilds within ~300ms and returns to watching. Ctrl+C exits.

For the full dev loop:

```bash
hm -b --watch --run
```

Edit → save → rebuild → run. No manual steps in between.

## 8. What just happened

HelpMake created a build cache folder:

```text
build/HelpMake/
├── gcc/
│   ├── obj/
│   │   ├── main.o
│   │   └── main.d
│   └── logs/
│       └── app.txt
```

- **`obj/main.o`** - the compiled object file
- **`obj/main.d`** - a depfile listing every header `main.cpp` includes. When one changes, `main.cpp` recompiles
- **`logs/app.txt`** - the compiler output for this target, with ANSI colors stripped

The directory is per-compiler (`gcc/`, `clang/`, `msvc/`, `zig-c++/`, etc.). Switching the compiler never mixes objects between toolchains.

## Next steps

- Full auto-detection rules - [see Auto-detection](11-auto-detection.md)
- Add more files - [see glob rules](02-helpmake-reference.md#inputfiles)
- Compile a second target - [see Config blocks](02-helpmake-reference.md#config-blocks)
- Speed up with precompiled headers - [see PCH guide](06-pch.md)
- Try a different toolchain - [see Toolchains](04-toolchains.md)
- Full flag list - [see CLI reference](03-cli-reference.md)
- Pull in a third-party library - [see Dependency blocks](02-helpmake-reference.md#dependency-blocks)
