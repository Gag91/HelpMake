#include "hm/parser/parser.hpp"

void Parser::install() {
    hp::printlnCl("[HelpMake] Installing HelpMake...", hp::Color::CYAN);

#ifdef _WIN32
    constexpr std::string_view installDir = "C:\\HelpMake";
    constexpr std::string_view cloneCmd = "git clone https://github.com/Gag91/Help_Make C:\\HelpMake";
    constexpr std::string_view buildCmd = "cd /d C:\\HelpMake && g++ -std=c++20 src/*.cpp -o hm.exe -Iinclude";
    constexpr std::string_view pathCmd = "setx PATH \"%PATH%;C:\\HelpMake\"";
#else
    constexpr std::string_view installDir = "~/.local/share/HelpMake";
    constexpr std::string_view cloneCmd = "git clone https://github.com/Gag91/Help_Make ~/.local/share/HelpMake";
    constexpr std::string_view buildCmd = "cd ~/.local/share/HelpMake && g++ -std=c++20 src/*.cpp -o hm -Iinclude";
    constexpr std::string_view pathCmd = "mkdir -p ~/.local/bin && ln -sf ~/.local/share/HelpMake/hm ~/.local/bin/hm";
#endif

    hp::printlnCl("[HelpMake] Downloading...", hp::Color::CYAN);
    if (std::system(std::string(cloneCmd).c_str()) != 0) {
        hp::printlnCl("[HelpMake] Error: Downloading failed.", hp::Color::RED);
        exit(EXIT_FAILURE);
    }

    hp::printlnCl("[HelpMake] Compiling...", hp::Color::CYAN);
    if (std::system(std::string(buildCmd).c_str()) != 0) {
        hp::printlnCl("[HelpMake] Error: Compilation failed.", hp::Color::RED);
        exit(EXIT_FAILURE);
    }

    hp::printlnCl("[HelpMake] Adding to PATH...", hp::Color::CYAN);
    if (std::system(std::string(pathCmd).c_str()) != 0) {
        hp::printlnCl("[HelpMake] Warning: PATH update failed.", hp::Color::YELLOW);
        hp::printlnCl(std::format("[HelpMake] Add {} to PATH manually.", installDir), hp::Color::YELLOW);
        return;
    }

    hp::printlnCl("[HelpMake] Installed successfully.", hp::Color::GREEN);
    hp::printlnCl("[HelpMake] Restart your terminal to use 'hm'.", hp::Color::CYAN);
    exit(EXIT_SUCCESS);
}