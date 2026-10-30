#include "hm/process/process.hpp"
#include "hm/parser/parser.hpp"

#include <array>
#include <cstdio>
#include <string>

#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#include <cstdlib>
#else
#define POPEN popen
#define PCLOSE pclose
#include <sys/wait.h>
#endif

namespace hm {

    std::string Process::join(const std::vector<std::string> &args) {
        std::string result;
        result.reserve(64);

        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i)
                result += ' ';
            const auto &a = args[i];

            bool needsQuote = a.empty() || a.find_first_of(" \t\"") != std::string::npos;

            if (needsQuote) {
                result += '"';
                for (char c : a) {
                    if (c == '"')
                        result += '\\';
                    result += c;
                }
                result += '"';
            } else {
                result += a;
            }
        }
        return result;
    }

    ProcessResult Process::run(const std::string &commandLine) {
        ProcessResult r;

        std::string cmd = commandLine + " 2>&1";

        FILE *pipe = POPEN(cmd.c_str(), "r");
        if (!pipe) {
            r.exitCode = -1;
            return r;
        }

        std::array<char, 4096> buffer;
        while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
            r.output += buffer.data();

        int rc = PCLOSE(pipe);

#ifdef _WIN32
        r.exitCode = rc;
#else
        r.exitCode = WIFEXITED(rc) ? WEXITSTATUS(rc) : rc;
#endif

        return r;
    }

    std::string Process::writeResponseFile(const std::vector<std::string> &args, const std::filesystem::path &rspPath) {
        std::error_code ec;
        std::filesystem::create_directories(rspPath.parent_path(), ec);

        std::ofstream out(rspPath, std::ios::binary);
        if (!out)
            return "";

        for (const auto &a : args) {
            bool needsQuote = a.empty() || a.find_first_of(" \t\"") != std::string::npos;
            if (needsQuote) {
                out << '"';
                for (char c : a) {
                    if (c == '"')
                        out << '\\';
                    out << c;
                }
                out << '"';
            } else {
                out << a;
            }
            out << '\n';
        }

        if (!out)
            return "";

        return "@" + rspPath.generic_string();
    }

} // namespace hm

bool Parser::isGitCheckout(const std::string &path, const std::string &url) const {
    auto remote = hm::Process::run(std::format("git -C \"{}\" config --get remote.origin.url", path));
    auto root = hm::Process::run(std::format("git -C \"{}\" rev-parse --show-toplevel", path));
    if (remote.exitCode != 0 || root.exitCode != 0)
        return false;

    auto trimOutput = [](std::string value) {
        std::size_t first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            return std::string{};
        std::size_t last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    };

    std::error_code ec;
    std::filesystem::path expectedRoot = std::filesystem::weakly_canonical(path, ec);
    if (ec)
        return false;
    std::filesystem::path actualRoot = std::filesystem::weakly_canonical(trimOutput(std::move(root.output)), ec);
    if (ec)
        return false;

    return expectedRoot == actualRoot && trimOutput(std::move(remote.output)) == url;
}

bool Parser::removeGitCheckout(const std::string &path, std::string &error) const {
    std::error_code ec;
    for (std::filesystem::recursive_directory_iterator it(path, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            error = ec.message();
            return false;
        }
        std::filesystem::permissions(it->path(), std::filesystem::perms::owner_all, std::filesystem::perm_options::replace, ec);
        if (ec) {
            error = ec.message();
            return false;
        }
    }
    if (ec) {
        error = ec.message();
        return false;
    }

    std::filesystem::permissions(path, std::filesystem::perms::owner_all, std::filesystem::perm_options::replace, ec);
    if (ec) {
        error = ec.message();
        return false;
    }
    std::filesystem::remove_all(path, ec);
    if (ec) {
        error = ec.message();
        return false;
    }
    return true;
}

std::string Parser::processGithubEntry(const std::string &value) {
    std::size_t arrow = value.find("->");
    if (arrow == std::string::npos) {
        cfg.v_Errors.push_back(std::format(
            "[HelpMake] Error: Fetch entry missing '->': '{}'", value));
        return "";
    }

    std::string url = trim(value.substr(0, arrow));
    std::string folder = trim(value.substr(arrow + 2));

    if (url.empty()) {
        cfg.v_Errors.push_back(std::format("[HelpMake] Error: Fetch entry has an empty URL: '{}'", value));
        return "";
    }

    std::string lockedCommit;
    if (cfg.frozen) {
        auto it = lockEntries.find("fetch:" + url);
        if (it == lockEntries.end() || it->second.url != url || it->second.commit.empty()) {
            hp::printlnCl(std::format("[HelpMake] Error: No valid frozen lock entry for Fetch '{}'. Run 'hm --update-lock'.", url), hp::RED);
            return "";
        }
        lockedCommit = it->second.commit;
    }

    std::size_t slash = url.find_last_of("/");
    std::string name = (slash == std::string::npos) ? url : url.substr(slash + 1);
    if (name.size() > 4 && name.substr(name.size() - 4) == ".git")
        name.erase(name.size() - 4);

    if (name.empty()) {
        cfg.v_Errors.push_back(std::format("[HelpMake] Error: Cannot derive repo name from URL: '{}'", url));
        return "";
    }

    std::string depRoot = std::format("build/HelpMake/dep/{}", name);
    if (cfg.dryRun) {
        std::string resolvedPath = depRoot;
        if (!folder.empty() && folder != ".")
            resolvedPath += "/" + folder;
        return resolvedPath;
    }

    if (!gitAvailable()) {
        hp::printlnCl("[HelpMake] Error: Git not found on PATH. Install git to use Fetch { }.", hp::RED);
        std::exit(EXIT_FAILURE);
    }

    std::error_code ec;
    std::filesystem::create_directories("build/HelpMake/dep", ec);
    if (ec) {
        cfg.v_Errors.push_back(std::format("[HelpMake] Error: Cannot create build/HelpMake/dep: {}", ec.message()));
        return "";
    }

    bool clone = !std::filesystem::exists(depRoot);
    if (!clone && !isGitCheckout(depRoot, url)) {
        hp::printlnCl(std::format("[HelpMake] Replacing stale Fetch cache for '{}'.", url), hp::YELLOW);
        std::string error;
        if (!removeGitCheckout(depRoot, error)) {
            cfg.v_Errors.push_back(std::format("[HelpMake] Error: Cannot remove stale Fetch cache '{}': {}", depRoot, error));
            return "";
        }
        clone = true;
    }

    if (clone) {
        if (cfg.verbose)
            hp::printlnCl(std::format("[HelpMake] Fetch '{}'...", url), hp::CYAN);

        std::string cmd = std::format("git clone --depth=1 \"{}\" \"{}\"", url, depRoot);
        auto r = hm::Process::run(cmd);

        if (r.exitCode != 0) {
            if (!r.output.empty())
                hp::printlnCl(r.output, hp::RED);

            std::filesystem::remove_all(depRoot, ec);
            cfg.v_Errors.push_back(std::format("[HelpMake] Error: Fetch clone failed for '{}'.", url));
            return "";
        }
    } else if (cfg.debug) {
        hp::printlnCl(std::format("[Debug] Fetch cache hit: '{}'", depRoot), hp::YELLOW);
    }

    if (cfg.frozen) {
        auto current = hm::Process::run(std::format("git -C \"{}\" rev-parse HEAD", depRoot));
        std::string currentCommit = current.output;
        currentCommit.erase(0, currentCommit.find_first_not_of(" \t\r\n"));
        currentCommit.erase(currentCommit.find_last_not_of(" \t\r\n") + 1);
        if (current.exitCode != 0 || currentCommit != lockedCommit) {
            std::string checkout = std::format("git -C \"{}\" fetch --depth=1 origin {} && git -C \"{}\" checkout --detach {}", depRoot, lockedCommit, depRoot, lockedCommit);
            auto result = hm::Process::run(checkout);
            if (result.exitCode != 0) {
                if (!result.output.empty())
                    hp::printlnCl(result.output, hp::RED);
                cfg.v_Errors.push_back(std::format("[HelpMake] Error: Could not check out locked Fetch revision '{}' for '{}'.", lockedCommit, url));
                return "";
            }
        }
    }

    fetchRoots[url] = depRoot;

    std::string fullPath = depRoot;
    if (!folder.empty() && folder != ".")
        fullPath += "/" + folder;

    if (!std::filesystem::exists(fullPath)) {
        cfg.v_Errors.push_back(std::format(
            "[HelpMake] Error: Fetch include path does not exist: '{}'", fullPath));
        return "";
    }

    if (cfg.debug) {
        cfg.v_Info[std::format("[Debug] Fetch Include Folder: '{}'", folder)] = hp::YELLOW;
        cfg.v_Info[std::format("[Debug] Fetch Clone Folder: '{}'", name)] = hp::YELLOW;
        cfg.v_Info[std::format("[Debug] Fetch Resolved Path: '{}'", fullPath)] = hp::YELLOW;
    }

    return fullPath;
}