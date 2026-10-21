#include "hm/dependecy.hpp"
#include "hm/parser/parser.hpp"
#include "hm/process/process.hpp"

int Parser::buildDependency(const hm::Dependency &dep) {
    std::string tag = "[HelpMake]";
    std::string depRoot = "build/HelpMake/dep/" + dep.name;

    std::filesystem::create_directories("build/HelpMake/dep");

    if (!std::filesystem::exists(depRoot)) {
        std::string cmd;
        bool hasTmpFile = false;
        std::string tmpPath;

        if (!dep.git.empty()) {
            cmd = "git clone";
            if (!dep.tag.empty() && dep.commit.empty())
                cmd += " --depth=1 -b " + dep.tag;
            cmd += " " + dep.git + " " + depRoot;
            if (!dep.commit.empty())
                cmd += " && git -C " + depRoot + " checkout " + dep.commit;
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