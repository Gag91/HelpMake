#include "hm/dependecy.hpp"
#include "hm/parser/parser.hpp"
#include "hm/process/process.hpp"

#include <algorithm>
#include <cctype>

int Parser::buildDependency(const hm::Dependency &dep) {
    std::string tag = "[HelpMake]";
    std::string depRoot = "build/HelpMake/dep/" + dep.name;

    if (!dep.commit.empty() && ((dep.commit.size() != 40 && dep.commit.size() != 64) ||
                                !std::ranges::all_of(dep.commit, [](unsigned char c) -> bool { return std::isxdigit(c) != 0; }))) {
        hp::printlnCl(std::format("{} Dependency '{}': invalid Git commit '{}'.", tag, dep.name, dep.commit), hp::RED);
        return 1;
    }

    std::filesystem::create_directories("build/HelpMake/dep");

    bool clone = !std::filesystem::exists(depRoot);
    if (!clone && !dep.git.empty() && !isGitCheckout(depRoot, dep.git)) {
        hp::printlnCl(std::format("{} Replacing stale dependency cache for '{}'.", tag, dep.name), hp::YELLOW);
        std::string error;
        if (!removeGitCheckout(depRoot, error)) {
            hp::printlnCl(std::format("{} Dependency '{}': cannot remove stale cache '{}': {}", tag, dep.name, depRoot, error), hp::RED);
            return 1;
        }
        clone = true;
    }

    if (clone) {
        std::string cmd;
        bool hasTmpFile = false;
        std::string tmpPath;

        if (!dep.git.empty()) {
            cmd = "git clone";
            if (dep.commit.empty() && !dep.tag.empty())
                cmd += " --depth=1 -b " + dep.tag;
            else
                cmd += " --depth=1";
            cmd += " \"" + dep.git + "\" \"" + depRoot + "\"";
        } else if (!dep.url.empty()) {
            tmpPath = depRoot + ".download";
            hasTmpFile = true;

            std::string dl = "curl -L -o " + tmpPath + " " + dep.url;
            auto r = hm::Process::run(dl);
            if (r.exitCode != 0) {
                hp::printlnCl(std::format("{} Dependency '{}': download failed", tag, dep.name), hp::RED);
                if (!r.output.empty())
                    hp::printlnCl(r.output, hp::RED);
                std::error_code ec;
                std::filesystem::remove(tmpPath, ec);
                return 1;
            }

            std::filesystem::create_directories(depRoot);

            if (dep.url.ends_with(".tar.gz") || dep.url.ends_with(".tgz"))
                cmd = "tar -xzf " + tmpPath + " -C " + depRoot + " --strip-components=1";
            else if (dep.url.ends_with(".zip"))
                cmd = "unzip -q " + tmpPath + " -d " + depRoot;
            else {
                hp::printlnCl(std::format("{} Dependency '{}': unsupported archive in '{}'", tag, dep.name, dep.url), hp::RED);
                std::error_code ec;
                std::filesystem::remove(tmpPath, ec);
                return 1;
            }
        } else if (!dep.path.empty()) {
            if (!std::filesystem::exists(dep.path)) {
                hp::printlnCl(std::format("{} Dependency '{}': path '{}' not found", tag, dep.name, dep.path), hp::RED);
                return 1;
            }
            std::error_code ec;
            std::filesystem::copy(dep.path, depRoot, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                hp::printlnCl(std::format("{} Dependency '{}': copy failed: {}", tag, dep.name, ec.message()), hp::RED);
                return 1;
            }
        } else {
            hp::printlnCl(std::format("{} Dependency '{}': no source (Git/Url/Path)", tag, dep.name), hp::RED);
            return 1;
        }

        if (!cmd.empty()) {
            auto r = hm::Process::run(cmd);

            if (hasTmpFile) {
                std::error_code ec;
                std::filesystem::remove(tmpPath, ec);
            }

            if (r.exitCode != 0) {
                hp::printlnCl(std::format("{} Dependency '{}': fetch failed", tag, dep.name), hp::RED);
                if (!r.output.empty())
                    hp::printlnCl(r.output, hp::RED);
                return 1;
            }
        }
    }

    if (!dep.git.empty() && !dep.commit.empty()) {
        auto current = hm::Process::run(std::format("git -C \"{}\" rev-parse HEAD", depRoot));
        std::string currentCommit = current.output;
        currentCommit.erase(0, currentCommit.find_first_not_of(" \t\r\n"));
        currentCommit.erase(currentCommit.find_last_not_of(" \t\r\n") + 1);
        if (current.exitCode != 0 || currentCommit != dep.commit) {
            std::string checkout = std::format("git -C \"{}\" fetch --depth=1 origin {} && git -C \"{}\" checkout --detach {}", depRoot, dep.commit, depRoot, dep.commit);
            auto result = hm::Process::run(checkout);
            if (result.exitCode != 0) {
                hp::printlnCl(std::format("{} Dependency '{}': could not check out commit '{}'.", tag, dep.name, dep.commit), hp::RED);
                if (!result.output.empty())
                    hp::printlnCl(result.output, hp::RED);
                return 1;
            }
        }
    }

    std::string buildIdPath = depRoot + "/.buildid";
    std::string currentId;
    for (const auto &b : dep.builds)
        currentId += b + "\n";

    std::string previousId;
    if (std::ifstream in(buildIdPath); in) {
        std::string line;
        while (std::getline(in, line))
            previousId += line + "\n";
    }

    if (!dep.builds.empty() && previousId != currentId) {
        for (const auto &b : dep.builds) {
            auto r = hm::Process::run(std::format("cd \"{}\" && {}", depRoot, b));
            if (r.exitCode != 0) {
                hp::printlnCl(std::format("{} Dependency '{}': build failed: {}", tag, dep.name, b), hp::RED);
                hp::printlnCl(r.output, hp::RED);
                return 1;
            }
        }
        std::ofstream(buildIdPath) << currentId;
    }

    return 0;
}