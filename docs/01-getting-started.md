# Getting started

This guide walks you through building your first project with HelpMake in under five minutes.

## 1. Install HelpMake

Follow the [installation guide](../README.md#installation) in the main README. Once `hm` is on your PATH, verify it:

```bash
hm --version
```
You should see:

```text
Help_Make (hm) - Build System
Version: 11.0.0
...
```

## 2. Create HelpMake.txt
In your project root:

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
- Use g++ with -std=c++20
- Compile every .cpp file directly under the src folder (use `**` to include subfolders)
- Add include/ to the include path
- Write the final executable file to build/app.exe
- Pass -Wall and -O2 to every compile and link step

## 3. Create a source file

```cpp
// src/main.cpp
#include <iostream>

int main() {
    std::cout << "Hello, HelpMake!\n";
    return 0;
}
```

## 4. Build

```bash
hm -b
```

The first build compiles every .cpp file and links the result. You should see:

```text
Compilation successful. Output file: build/app.exe
[HelpMake] Build Time: 2.14s
```

## 5. Rebuild

Run `hm -b` again:

```text
[HelpMake] Skip (up to date): src/main.cpp
Compilation successful. Output file: build/app.exe
[HelpMake] Build Time: 0.31s
```

Nothing recompiled. If you edit src/main.cpp and run `hm -b` again, only main.cpp recompiles before the link step runs.

## 6. What just happened

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

- **obj/main.o:** The compiled object file.
- **obj/main.d:** A dependency file that lists every header main.cpp includes. When one of those headers changes, HelpMake recompiles main.cpp.
- **logs/app.txt:** The compiler output for this target without ANSI colors.

The directory is separate for each compiler (gcc/, clang/, msvc/, zig-c++/, etc.). Switching the compiler in your config file will never mix files between different tools.

## Next steps
- Add more files - see [glob rules](02-helpmake-reference.md/#inputfiles)
- Compile a second target - see [Config blocks](02-helpmake-reference.md/#Blocks)
- Speed up with precompiled headers - [see PCH guide](06-pch.md)
- Try a different toolchain - [see Toolchains](04-toolchains.md)
- Full flag list - [see CLI reference](03-cli-reference.md)

