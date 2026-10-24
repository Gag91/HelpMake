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

    std::size_t slash = url.find_last_of("/");
    std::string name = (slash == std::string::npos) ? url : url.substr(slash + 1);
    if (name.size() > 4 && name.substr(name.size() - 4) == ".git")
        name.erase(name.size() - 4);

    if (name.empty()) {
        cfg.v_Errors.push_back(std::format("[HelpMake] Error: Cannot derive repo name from URL: '{}'", url));
        return "";
    }

    std::string depRoot = std::format("build/HelpMake/dep/{}", name);

    if (!std::filesystem::exists(depRoot)) {
        if (cfg.verbose)
            hp::printlnCl(std::format("[HelpMake] Fetch '{}'...", url), hp::CYAN);

        std::string cmd = std::format("git clone --depth=1 {} \"{}\"", url, depRoot);
        auto r = hm::Process::run(cmd);

        if (r.exitCode != 0) {
            hp::printlnCl(std::format("[HelpMake] Error: git clone failed for '{}'", url), hp::RED);
            if (!r.output.empty())
                hp::printlnCl(r.output, hp::RED);

            std::filesystem::remove_all(depRoot, ec);
            return "";
        }
    } else if (cfg.debug) {
        hp::printlnCl(std::format("[Debug] Fetch cache hit: '{}'", depRoot), hp::YELLOW);
    }

    std::string fullPath = depRoot;
    if (!folder.empty() && folder != ".")
        fullPath += "/" + folder;

    if (!std::filesystem::exists(fullPath)) {
        cfg.v_Errors.push_back(std::format(
            "[HelpMake] Error: Fetch include path does not exist: '{}'", fullPath));
        return "";
    }

    if (cfg.debug) {
        hp::printlnCl(std::format("[Debug] Fetch Include Folder: '{}'", folder), hp::YELLOW);
        hp::printlnCl(std::format("[Debug] Fetch Clone Folder: '{}'", name), hp::YELLOW);
        hp::printlnCl(std::format("[Debug] Fetch Resolved Path: '{}'", fullPath), hp::YELLOW);
    }

    return fullPath;
}