# Variables

Variables let you reuse strings across `HelpMake.txt` without repeating them. They're useful for CI, cross-platform scripts, and multi-config projects.

## Defining variables

```txt
Set {
    CXX: g++
    CXX_STD: c++20
    OUT_DIR: build
    OPT: -O2
}
```
Each line inside `Set { }` is `name: value`. Names are case-sensitive and may not contain spaces.

## Using variables

Expand with `[[name]]`:

```txt
Set {
    CXX: g++
    CXX_STD: c++20
    OUT_DIR: build
}

Compiler: [[CXX]]
Version: std=[[CXX_STD]]
Output: [[OUT_DIR]]/app

InputFiles {
    src/*.cpp
}

Flags {
    -Wall
    -[[CXX_STD]]-optimized
}
```
`[[name]]` is expanded in every field and block entry. Values are substituted verbatim - no whitespace trimming happens on expansion.

## CLI override

Any variable can be overridden from the command line:

```bash
hm -b --var CXX=clang++ --var OPT=-O3
```

Precedence rules:

- **CLI (`--var`) wins:** If a name is set on the command line, the `Set { }` entry of the same name is ignored.
- **`Set { }` values win over nothing:** If a `[[name]]` has no definition anywhere, the parse fails with an undefined-variable error.

You can override a variable that's not defined in the file:

```txt
Output: [[OUT_DIR]]/app
```
```bash
hm -b --var OUT_DIR=bin
```
This is useful for CI where you never edit `HelpMake.txt`.

## Escaping

To emit a literal `[[`, write `[[[[`:

```txt
Flags {
    -DMSG="[[[[not a variable]]]]"
}
```
The parser consumes the first two brackets of `[[[[`, then leaves the remaining `[[` as literal text.

## Errors

**Undefined variable:**
```text
[HelpMake] Error: Undefined variable "[[CXX]]"
```

**Unterminated expansion:**
```text
[HelpMake] Error: Unterminated variable. Missing "]]"
```

Both errors include file context in the full message. `hm --show` will refuse to print configs that have variable errors.

## Full example

```txt
# HelpMake.txt

Set {
    CXX: g++
    CXX_STD: c++20
    OUT_DIR: build
    OPT_LEVEL: -O2
}

Compiler: [[CXX]]
Version: std=[[CXX_STD]]
Output: [[OUT_DIR]]/app

InputFiles {
    src/*.cpp
}

Includes {
    include
}

Flags {
    -Wall
    -Wextra
    [[OPT_LEVEL]]
}

Config: debug {
    Flags: -g -O0 -DDEBUG
    Output: [[OUT_DIR]]/debug.exe
}

Config: release {
    Flags: -O3 -DNDEBUG -flto
    Output: [[OUT_DIR]]/release.exe
}
```

Build with defaults:
```bash
hm -b --config release
```

Build with clang and `-O3` override:
```bash
hm -b --config release --var CXX=clang++ --var OPT_LEVEL=-O3
```

CI pipeline:
```bash
# Linux
hm -b --config release --var CXX=clang++

# Windows
hm -b --config release --var CXX=cl
```

## Variables in configs

Variables expand at parse time, before config inheritance. This means:

- A `Set { }` at the root is visible to every config.
- A `Config: name { }` block can define its own `Set { }` - that shadows for that config's fields only.
- `--var` overrides every `Set { }` definition, regardless of scope.

## Limitations

- **No computed expressions:** `[[A]][[B]]` concatenates `A` and `B`, but you can't do math or string manipulation.
- **No conditional expansion:** Every `[[name]]` must have a defined value.
- **No environment variable passthrough:** Use `--var NAME=$ENV_VAR` in your shell for that.
