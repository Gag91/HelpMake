#include "hm/cli/cli.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <system_error>
#include <vector>

#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"

namespace hm::cli {

    int clean(bool gcm, bool all, bool verbose) {

        std::error_code ec;
        auto delete_path = [&](std::filesystem::path path) -> bool {
            if (!std::filesystem::exists(path, ec)) {
                return true;
            }

            int rc = 0;
            std::vector<std::filesystem::path> paths_to_delete;

            auto options = std::filesystem::directory_options::skip_permission_denied;
            for (auto it = std::filesystem::recursive_directory_iterator(path, options, ec); it != std::filesystem::recursive_directory_iterator{}; it.increment(ec)) {
                if (ec) {
                    hp::printlnCl(std::format("[HelpMake] Error reading tree: {}", ec.message()), hp::RED);
                    ec.clear();
                    break;
                }

                auto perm = it->status(ec).permissions();
                if (ec) {
                    ec.clear();
                }

                if ((perm & std::filesystem::perms::owner_write) == std::filesystem::perms::none) {
                    std::filesystem::permissions(it->path(), std::filesystem::perms::owner_all, std::filesystem::perm_options::replace, ec);
                    if (ec) {
                        ec.clear();
                    }
                }

                paths_to_delete.push_back(it->path());
            }

            std::sort(paths_to_delete.begin(), paths_to_delete.end(), [](const auto &a, const auto &b) {
                return a.string().length() > b.string().length();
            });

            for (const auto &current_path : paths_to_delete) {
                std::filesystem::remove(current_path, ec);

                if (ec) {
                    hp::printlnCl(std::format("[HelpMake] Failed to delete: {} -> {}", current_path.string(), ec.message()), hp::RED);
                    ++rc;
                    ec.clear();
                } else {
                    if (verbose) {
                        hp::printlnCl(std::format("[HelpMake] Successfully deleted: {}", current_path.string()), hp::GREEN);
                    }
                }
            }

            std::filesystem::remove(path, ec);
            std::filesystem::remove(path, ec);
            if (ec) {
                hp::printlnCl(std::format("[HelpMake] Failed to delete: {} -> {}", path.string(), ec.message()), hp::RED);
                ++rc;
                ec.clear();
            }

            return (rc == 0);
        };

        if (all) {
            if (delete_path("build/HelpMake") && delete_path("gcm.cache")) {
                hp::printlnCl("[HelpMake] Cleaned build/ and gcm.cache", hp::GREEN);
                return 0;
            }
            return 1;
        }

        if (gcm) {
            if (delete_path("gcm.cache")) {
                hp::printlnCl("[HelpMake] Cleaned gcm.cache", hp::GREEN);
                return 0;
            }
            return 1;
        }

        if (delete_path("build/HelpMake")) {
            hp::printlnCl("[HelpMake] Cleaned build/HelpMake", hp::GREEN);
            return 0;
        }
        return 1;
    }

} // namespace hm::cli
