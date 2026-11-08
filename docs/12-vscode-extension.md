# VS Code Extension

HelpMake has an official Visual Studio Code extension that adds syntax highlighting for `HelpMake.txt` and `.helpmake` files, making it easier to read and write your build scripts.

**Install from the VS Code Marketplace:** [HelpMake on VS Code Marketplace](https://marketplace.visualstudio.com/items?itemName=Xavi99.helpmake)

## Features

- **Syntax highlighting** for `HelpMake.txt` and `.helpmake` files
- **Comments** (`#`)
- **Block keywords** (`InputFiles`, `Includes`, `Flags`, `Fetch`, `Modules`, `PreBuild`, `PostBuild`, `Copy`, `Set`, `Arguments`, `Config`, `Dependency`)
- **Config and dependency names** highlighted distinctly (e.g., `Config: debug {`, `Dependency: raylib {`)
- **All scalar fields** (`Compiler`, `Version`, `Output`, `Kind`, `Run`, `Jobs`, `PCH`, `Depends`)
- **All dependency fields** (`Git`, `Url`, `Path`, `Tag`, `Commit`, `Hash`, `Build`, `Include`, `Link`, `LinkDir`, `LinkFlags`)
- **Variable expansion** (`[[name]]`)

## Usage

Just open a `HelpMake.txt` or any `.helpmake` file in VS Code. Highlighting works automatically. No configuration required.

## Source Code

The extension is open-source and available on GitHub: [HelpMake-Vscode-Extension](https://github.com/Gag91/HelpMake-Vscode-Extension)