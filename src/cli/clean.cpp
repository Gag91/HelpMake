#include "hm/cli/cli.hpp"

#include <filesystem>

#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"

namespace hm::cli {

    void clean(bool gcm, bool all) {
        if (all) {
            std::filesystem::remove_all("build/HelpMake");
            std::filesystem::remove_all("gcm.cache");
            hp::printlnCl("[HelpMake] Cleaned build/HelpMake and gcm.cache", hp::GREEN);
            return;
        }

        if (gcm) {
            std::filesystem::remove_all("gcm.cache");
            hp::printlnCl("[HelpMake] Cleaned gcm.cache", hp::GREEN);
            return;
        }

        std::filesystem::remove_all("build/HelpMake");
        hp::printlnCl("[HelpMake] Cleaned build/HelpMake", hp::GREEN);
    }

} // namespace hm::cli