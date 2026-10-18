#include "hm/parser/parser.hpp"

bool Parser::PchneedsRebuild(const std::string &src, const std::string &obj, bool rebuild) {
    if (rebuild)
        return true;

    if (!std::filesystem::exists(obj))
        return true;

    if (!std::filesystem::exists(src)) {
        hp::printlnCl(
            std::format("[HelpMake] Error: PCH header '{}' not found", src),
            hp::RED);
        exit(EXIT_FAILURE);
    }

    auto srcTime = std::filesystem::last_write_time(src);
    auto objTime = std::filesystem::last_write_time(obj);

    if (srcTime > objTime) {
        if (cfg.debug)
            hp::printlnCl(
                std::format("[Debug] PCH header '{}' is newer than '{}'. Rebuilding.",
                            src, obj),
                hp::YELLOW);
        return true;
    }

    for (const auto &dep : v_DepFiles) {
        if (!std::filesystem::exists(dep))
            continue;

        auto depTime = std::filesystem::last_write_time(dep);
        if (depTime > objTime) {
            if (cfg.debug)
                hp::printlnCl(
                    std::format("[Debug] Header '{}' is newer than PCH '{}'. Rebuilding.",
                                dep, obj),
                    hp::YELLOW);
            return true;
        }
    }

    return false;
}