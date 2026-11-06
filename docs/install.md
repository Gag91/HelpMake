## HelpMake Installation

**Prerequisites:** `git` and a C++20 compiler (`g++ 11+`, `clang++ 11+`, or `MSVC 19.30+`).

### Linux / macOS / WSL:
```bash
curl -fsSL https://raw.githubusercontent.com/Gag91/Help_Make/main/install.sh | bash
```

### Windows (cmd or PowerShell):
```cmd
curl -fsSL -o install.bat https://raw.githubusercontent.com/Gag91/Help_Make/main/install.bat && install.bat
```

Both scripts clone to `~/.local/share/HelpMake` (`%USERPROFILE%\HelpMake` on Windows), build `hm` with your system compiler, and add it to your user `PATH`. Restart your terminal after they finish.

### Manual build (if you'd rather do it yourself):
```bash
git clone https://github.com/Gag91/Help_Make
cd Help_Make
g++ -std=c++20 src/cli/*.cpp src/executor/*.cpp src/fs/*.cpp \
src/generator/*.cpp src/getters/*.cpp src/init/*.cpp \
src/parser/*.cpp src/process/*.cpp src/toolchain/*.cpp \
src/main.cpp -o hm -Iinclude
```
Then put `hm` on your `PATH` manually.

### Custom install location:
**Linux / macOS / WSL:**
```bash
HELPMAKE_DIR=/opt/HelpMake bash install.sh
```

**Windows:**
```cmd
set HELPMAKE_DIR=D:\Dev\HelpMake
install.bat
```

### Uninstall:
Delete `~/.local/share/HelpMake` (or `%USERPROFILE%\HelpMake`), remove `hm` from `~/.local/bin` (or `%USERPROFILE%\.local\bin`), and remove the corresponding line from your shell profile or user `PATH`.
