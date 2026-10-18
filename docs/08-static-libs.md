# Static libraries

`Kind: static` produces a static library (`.a` on Unix-likes, `.lib` on MSVC) instead of an executable.

## Producing a static library

```txt
Compiler: g++
Version: std=c++20
Kind: static
Output: build/libmath.a

InputFiles {
    src/math/*.cpp
}

Includes {
    include
}

Flags {
    -Wall
    -O2
}
```

Build:
```bash
hm -b
```

HelpMake runs:

1. Compile each source into an object under `build/HelpMake/gcc/obj/`
2. Run `ar rcs build/libmath.a <all objects>` to create the archive

On MSVC, step 2 becomes `lib.exe /OUT:build/libmath.lib <objects>`.

## Consuming a static library

In a downstream project (or a config in the same `HelpMake.txt`):

```txt
Config: app {
    Compiler: g++
    Output: build/app.exe

    InputFiles {
        src/main.cpp
    }

    Flags {
        -Lbuild
        -lmath
    }
}
```

HelpMake places all `-l` flags after the object files on the link command line, so the standard Unix link order rule is respected automatically:

```text
g++ … build/HelpMake/gcc/obj/main.o -Lbuild -lmath -o build/app.exe
```
No manual ordering required.

## MSVC differences

| Aspect | GCC / Clang | MSVC |
|---|---|---|
| Archive tool | `ar rcs` | `lib.exe` |
| Extension | `.a` | `.lib` |
| Library path flag | `-L<path>` | `/LIBPATH:<path>` |
| Library flag | `-l<name>` | `<name>.lib` (passed literally) |
| Link library section | end of command | end of command |

For MSVC, use:
```txt
Flags {
    /LIBPATH:build
    math.lib
}
```
HelpMake doesn't try to translate `-l` into `<name>.lib` automatically - spell out the full library name for MSVC.

## Full example: lib + app in one file

```txt
Compiler: g++
Version: std=c++20

# Library
Config: math-lib {
    Kind: static
    Output: build/libmath.a
    InputFiles {
        lib/math/*.cpp
    }
    Includes {
        lib/math/include
    }
    Flags {
        -Wall
        -O2
    }
}

# Consumer
Config: app {
    Output: build/app.exe
    InputFiles {
        app/*.cpp
    }
    Includes {
        lib/math/include
    }
    Flags {
        -Wall
        -O2
        -Lbuild
        -lmath
    }
}
```

Build both, in order:
```bash
hm -b --config math-lib
hm -b --config app
```

`app` requires `math-lib` to be built first - HelpMake doesn't (yet) have inter-config dependencies, so the CI script handles ordering:
```bash
hm -b --config math-lib && hm -b --config app
```

## Gotcha: --no-sep incompatible with Kind: static

`--no-sep` asks HelpMake to pass all sources to the compiler in one shot. There's no object file to archive in that mode, so `Kind: static` + `--no-sep` is rejected with:

```text
Error: 'Kind: static' requires separate compilation. Remove --no-sep.
```
Static library builds always use separate compilation.

## Gotcha: PCH interaction

PCH works with `Kind: static` the same as it works with executables - the PCH is built once, cached per compiler, and injected into every source compile. No special handling needed.

## When to use static libraries

- **Reusable code across projects:** package common utilities as `.a`
- **Faster incremental links:** relinking against a prebuilt `.a` is faster than recompiling the source
- **Distribution:** ship a `.a` + headers instead of source

Not ideal for:
- Single-binary distribution - use normal `Kind: executable` with all sources compiled in
- Cross-platform binaries - `.a` is ABI-specific; ship source or use CMake

## Inspecting an archive

```bash
ar t build/libmath.a          # list members
ar x build/libmath.a          # extract all
nm build/libmath.a | grep add # check a specific symbol
```

MSVC equivalent:
```cmd
lib /LIST build/libmath.lib
dumpbin /symbols build/libmath.lib
```
