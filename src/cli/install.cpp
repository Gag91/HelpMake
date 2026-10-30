#include "hm/parser/parser.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

static bool registerFile(const std::filesystem::path &filename) {
    std::filesystem::path FolderPath = filename.parent_path();
    FolderPath = std::filesystem::weakly_canonical(FolderPath);
    std::string folderPathStr = FolderPath.string();

#ifdef _WIN32
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    DWORD bufferSize = 0;
    DWORD type = REG_EXPAND_SZ;

    if (RegQueryValueExA(hKey, "Path", NULL, &type, NULL, &bufferSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }

    std::vector<char> currentPathBuffer(bufferSize);
    if (RegQueryValueExA(hKey, "Path", NULL, &type, (LPBYTE)currentPathBuffer.data(), &bufferSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }

    std::string currentPathStr(currentPathBuffer.data());

    if (currentPathStr.find(folderPathStr) != std::string::npos) {
        RegCloseKey(hKey);
        return true;
    }

    std::string newPath = currentPathStr.empty() ? folderPathStr : currentPathStr + ";" + folderPathStr;

    if (RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ, (const BYTE *)newPath.c_str(), newPath.length() + 1) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }

    RegCloseKey(hKey);

    DWORD_PTR dwReturnValue;
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM) "Environment", SMTO_ABORTIFHUNG, 5000, &dwReturnValue);

    return true;

#else
    const char *homeDir = std::getenv("HOME");
    if (!homeDir)
        return false;

    std::filesystem::path bashrcPath = std::filesystem::path(homeDir) / ".bashrc";

    std::ifstream checkFile(bashrcPath);
    if (checkFile.is_open()) {
        std::string line;
        while (std::getline(checkFile, line)) {
            if (line.find(folderPathStr) != std::string::npos) {
                return true;
            }
        }
        checkFile.close();
    }

    std::ofstream configFile(bashrcPath, std::ios_base::app);
    if (!configFile.is_open())
        return false;

    configFile << "\nexport PATH=\"" << folderPathStr << ":$PATH\"\n";

    configFile.close();
    return true;
#endif
}

void Parser::install() {
    hp::printlnCl("[HelpMake] Installing HelpMake...", hp::CYAN);

#ifdef _WIN32
    constexpr std::string_view installDir = "C:\\HelpMake";
    constexpr std::string_view cloneCmd = "git clone https://github.com/Gag91/Help_Make C:\\HelpMake";
    constexpr std::string_view buildCmd = "cd /d C:\\HelpMake && hm -b --debug --verbose";
    std::filesystem::path binaryPath = std::filesystem::path(installDir) / "hm.exe";
#else
    constexpr std::string_view installDir = "~/.local/share/HelpMake";
    constexpr std::string_view cloneCmd = "git clone https://github.com/Gag91/Help_Make ~/.local/share/HelpMake";
    constexpr std::string_view buildCmd = "cd ~/.local/share/HelpMake && hm -b --debug --verbose";

    std::filesystem::path binaryPath;
    const char *homeDir = std::getenv("HOME");
    if (homeDir) {
        binaryPath = std::filesystem::path(homeDir) / ".local" / "share" / "HelpMake" / "hm";
    } else {
        binaryPath = "hm";
    }
#endif

    hp::printlnCl("[HelpMake] Downloading...", hp::CYAN);
    if (std::system(std::string(cloneCmd).c_str()) != 0) {
        hp::printlnCl("[HelpMake] Error: Downloading failed.", hp::RED);
        exit(EXIT_FAILURE);
    }

    hp::printlnCl("[HelpMake] Compiling...", hp::CYAN);
    if (std::system(std::string(buildCmd).c_str()) != 0) {
        hp::printlnCl("[HelpMake] Error: Compilation failed.", hp::RED);
        exit(EXIT_FAILURE);
    }

    hp::printlnCl("[HelpMake] Adding to PATH...", hp::CYAN);
    if (!registerFile(binaryPath)) {
        hp::printlnCl("[HelpMake] Warning: PATH update failed.", hp::YELLOW);
        hp::printlnCl(std::format("[HelpMake] Add {} to PATH manually.", installDir), hp::YELLOW);
        return;
    }

    hp::printlnCl("[HelpMake] Installed successfully.", hp::GREEN);
    hp::printlnCl("[HelpMake] Restart your terminal to use 'hm'.", hp::CYAN);
    exit(EXIT_SUCCESS);
}