#include "hm/parser/parser.hpp"

bool Parser::needsRebuild(const std::string &src, const std::string &obj, bool rebuild) {
    if (!std::filesystem::exists(src)) {
        hp::printlnCl(std::format("[HelpMake] Error: source file not found: {}", src), hp::Color::RED);
        exit(EXIT_FAILURE);
    }

    if (rebuild)
        return true;

    if (!std::filesystem::exists(obj))
        return true;

    auto srcTime = std::filesystem::last_write_time(src);
    auto objTime = std::filesystem::last_write_time(obj);

    if (srcTime > objTime) {
        if (cfg.debug)
            hp::printlnCl(std::format("\n[Debug] Source file '{}' is newer than object '{}'. Rebuilding.", src, obj), hp::Color::YELLOW);
        return true;
    }

    std::ifstream depFile(std::filesystem::path(obj).replace_filename(std::filesystem::path(obj).stem().string() + ".d").generic_string());
    if (!depFile.is_open()) {
        if (!cfg.quiet)
            hp::printlnCl(std::format("\nWarning: Could not open dependency file for '{}'. Rebuilding.\n", obj), hp::Color::YELLOW);
        return true;
    }
    std::string line;
    auto depTime = (std::filesystem::file_time_type::min)();

    while (std::getline(depFile, line)) {
        std::istringstream iss(line);
        std::string depFilePath;
        while (iss >> depFilePath) {
            if (depFilePath.find(":") != std::string::npos || depFilePath == "\\" || (depFilePath.length() <= 2 && depFilePath.back() == '\\'))
                continue;

            if (depFilePath == "CXX_IMPORTS" || depFilePath == "+=")
                continue;

            if (depFilePath.find(".c++-module") != std::string::npos)
                continue;

            if (!std::filesystem::exists(depFilePath)) {
                if (!cfg.quiet)
                    hp::printlnCl(std::format("Warning: Dependency file '{}' does not exist. Rebuilding.", depFilePath), hp::Color::YELLOW);
                return true;
            }

            depTime = std::filesystem::last_write_time(depFilePath);
            if (depTime > objTime) {
                if (cfg.debug)
                    hp::printlnCl(std::format("[Debug] Dependency '{}' is newer than object '{}'. Rebuilding.", depFilePath, obj), hp::Color::YELLOW);
                return true;
            }
        }
    }

    return false;
}