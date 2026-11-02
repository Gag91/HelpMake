# Contributing to HelpMake

Thanks for your interest. HelpMake is a small project, currently maintained by one person.

## Before you write code

- **Bugs**: open an issue with the exact command you ran, your OS, compiler, compiler version, and the output. A minimal reproducer is worth more than a paragraph.
- **Feature requests**: open an issue first. HelpMake deliberately says no to a lot of features. The whole design philosophy is "does one thing, does it well." A feature that expands the scope of the tool is likely to be declined even if it's well-implemented.
- **"Should I work on X?"**: ask in an issue *before* opening a PR. A one-paragraph "here's what I want to change and why" costs two minutes; discovering it's out of scope after implementation costs a weekend.

## Code style

- C++20, no external dependencies beyond `hp/` (Help_Library) and the standard library.
- A `.clang-format` is provided, don't modify it.
- Errors should be actionable ("did you mean X?") not just "failed". This is one of the reasons HelpMake exists.
- Commit messages: short subject line. No need for a template.

## Testing

- To build the project: run `hm -b` from the repo root. `HelpMake.txt` is already configured for self-hosting.
- If you don't have `hm` yet (e.g. you're fixing `hm` itself), build using `install.bat` for windows or `install.sh` for linux - then run `hm -b` to confirm self-hosting still works.
- Before opening a PR, run `hm test`. It builds and runs every config marked `Test: true`. If your change breaks a test, that's a bug in your change.
- If you're adding a feature, add a `Test: true` config that exercises it.
- If you're adding a new CLI flag, add it to the `else if` chain in `src/cli/build.cpp`. If you're adding a new config-file field, add it to `parseSection` in `src/parser/parser.cpp`, to `knownKeys` (so typos like `Conpiler:` are caught), and to the `Config` struct. Both need a `Test: true` config that exercises them.

## What gets merged

Good PRs to this project usually look like:

- **Bugfixes** with a clear reproducer and a test that exercises the fix.
- **New toolchain support** - adding a compiler, a module strategy, a generator target.
- **Documentation improvements**, especially when they close a gap someone got stuck on.
- **Performance work** that's measured, not assumed.
- **Small UX improvements** - a better error message, a better default, a clearer `--help` line.

What usually gets declined:

- **New flags** that duplicate an existing one with a different name. `--dry-run` vs `--dryrun` vs `--dry` - one of them is enough.
- **New syntax in `HelpMake.txt`.** The file format is deliberately small. Every field added has to earn its place against "can this be expressed with `Flags { }` and `Set { }`?"
- **Rewrites of working code for style reasons.** If it compiles, passes tests, and its shape isn't actively causing bugs, it stays.
- **New dependencies.** `hp/` (Help_Library) is fetched via `Fetch { }` at build time - sources live at `build/HelpMake/dep/Help_Library/` after the first `hm -b`. Changes made inside `build/` will be overwritten on the next fetch. If you want to modify `hp/`, open a PR at [Help_Library](https://github.com/Gag91/Help_Library).

## Review process

- If a PR isn't merged, you'll get a reason. It's usually "out of scope" or "would change the design in a direction I don't want", not "this is bad code."
- Contributions are accepted under the MIT license of the project. If you don't want your code to be MIT-licensed, don't send a PR.

## Code of Conduct
Be civil. Critique code, not people. Report concerns to `xavi640` on Discord - bad behavior gets a warning, then a ban.
