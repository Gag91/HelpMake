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

} // namespace hm

std::string Parser::processGithubEntry(const std::string &value) {
    std::size_t arrow = value.find("->");
    if (arrow == std::string::npos)
        return "";

    std::string url = trim(value.substr(0, arrow));
    std::string folder = trim(value.substr(arrow + 2));

    if (!gitAvailable()) {
        hp::printlnCl("Error: Git system not found. Please install it and add it to PATH.", hp::RED);
        exit(EXIT_FAILURE);
    }

    hp::Folder f;
    if (!f.exists("build/HelpMake/dep"))
        f.create("build/HelpMake/dep");

    if (cfg.verbose)
        hp::printlnCl(std::format("\n[Help_Make] Downloading dependency '{}' ...\n", url), hp::CYAN);

    std::size_t slash = url.find_last_of("/");
    std::string include = url.substr(slash + 1);
    if (include.size() > 4 && include.substr(include.size() - 4) == ".git")
        include.erase(include.size() - 4);

    hp::command(std::format("git clone {} build/HelpMake/dep/{}", url, include));

    std::string fullPath = std::format("build/HelpMake/dep/{}", include);
    if (!folder.empty() && folder != ".")
        fullPath += "/" + folder;

    if (cfg.debug) {
        hp::printlnCl(std::format("[Debug] Fetch Include Folder: '{}'", folder), hp::YELLOW);
        hp::printlnCl(std::format("[Debug] Fetch Clone Folder: '{}'", include), hp::YELLOW);
    }

    return fullPath;
}