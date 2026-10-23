#include "hm/generator/generator.hpp"

#include "hm/generator/util.hpp"
#include "hm/toolchain/compiler.hpp"
#include "hm/util/containers.hpp"
#include "hm/util/strings.hpp"
#include "hp/colors/color.hpp"
#include "hp/other/other.hpp"

#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace hm::gen {

    class CMake : public Generator {
      private:
        static std::string escapeValue(const std::string &s) {
            std::string out;
            for (char c : s) {
                if (c == '"')
                    out += "\\\"";
                else if (c == '\\')
                    out += "\\\\";
                else
                    out += c;
            }
            return out;
        }

        static std::string extractStandard(const std::string &version) {
            std::string v = version;
            if (!v.empty() && (v.front() == '-' || v.front() == '/'))
                v.erase(v.begin());
            if (v.rfind("std=c++", 0) == 0 || v.rfind("std:c++", 0) == 0)
                return v.substr(7);
            if (v.rfind("std=c", 0) == 0 || v.rfind("std:c", 0) == 0)
                return v.substr(5);
            return "";
        }

        static void splitFlags(const std::string &flags, std::vector<std::string> &compile, std::vector<std::string> &link, std::vector<std::string> &archives) {
            std::istringstream iss(flags);
            std::string tok;
            while (iss >> tok) {
                if (tok.rfind("-l", 0) == 0)
                    link.push_back(tok);
                else if (tok.ends_with(".a") || tok.ends_with(".lib") || tok.ends_with(".so"))
                    archives.push_back(tok);
                else
                    compile.push_back(tok);
            }
        }

      public:
        std::string_view name() const override {
            return "cmake";
        }
        std::string_view Filename() const override {
            return "CMakeLists.txt";
        }
        std::string_view description() const override {
            return "CMake build system: generates CMakeLists.txt";
        }

        bool generate(const Config &cfg, const std::filesystem::path &projectRoot) override {
            auto comp = hm::makeCompiler(cfg.compiler);
            if (!comp) {
                hp::printlnCl("[CMake] Unknown compiler '" + cfg.compiler + "'", hp::RED);
                return false;
            }
            if (!comp->supportsSeparate()) {
                hp::printlnCl(std::format("[CMake] '{}' is one-shot only. Use `hm -b`.", cfg.compiler), hp::YELLOW);
                return false;
            }

            if (!cfg.Precmd.empty() || !cfg.v_Precmd.empty())
                hp::printlnCl("[CMake] Warning: PreBuild not emitted.", hp::YELLOW);
            if (!cfg.Postcmd.empty() || !cfg.v_Postcmd.empty())
                hp::printlnCl("[CMake] Warning: PostBuild not emitted.", hp::YELLOW);
            if (!cfg.v_Copy.empty())
                hp::printlnCl("[CMake] Warning: Copy not emitted.", hp::YELLOW);
            if (!cfg.v_Github.empty())
                hp::printlnCl("[CMake] Warning: Fetch not emitted (use FetchContent).", hp::YELLOW);
            if (!cfg.v_Depends.empty())
                hp::printlnCl("[CMake] Warning: Depends not emitted (use FetchContent).", hp::YELLOW);
            if (!cfg.v_Modules.empty())
                hp::printlnCl("[CMake] Warning: Modules not emitted (needs CMake 3.28+ FILE_SET CXX_MODULES).", hp::YELLOW);

            std::vector<std::string> files = expandInputs(cfg);
            if (files.empty()) {
                hp::printlnCl("[CMake] No files matched InputFiles { }.", hp::RED);
                return false;
            }

            bool hasC = false;
            bool hasCxx = false;
            for (const auto &f : files) {
                if (f.ends_with(".c"))
                    hasC = true;
                else
                    hasCxx = true;
            }
            std::string languages = (hasC && hasCxx) ? "C CXX" : (hasC ? "C" : "CXX");

            std::filesystem::path outPath(cfg.output);
            std::string targetName = outPath.stem().string();
            if (targetName.empty())
                targetName = "app";
            for (auto &c : targetName)
                if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
                    c = '_';

            std::string projectName = cfg.name.empty() ? targetName : cfg.name;
            for (auto &c : projectName)
                if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
                    c = '_';

            std::string version = cfg.version;
            if (!version.empty() && version.front() != '-')
                version.insert(version.begin(), '-');
            std::string combined = version;
            if (!cfg.flags.empty())
                combined += (combined.empty() ? "" : " ") + cfg.flags;

            std::vector<std::string> compileFlags, linkFlags, archives;
            splitFlags(combined, compileFlags, linkFlags, archives);

            std::string std = extractStandard(cfg.version);
            bool isStatic = to_lower(cfg.kind) == "static";

            std::ostringstream n;
            n << "# Generated by HelpMake. Do not edit.\n";
            n << "# Regenerate with: hm --gen cmake" << (cfg.name.empty() ? "" : " --config " + cfg.name) << "\n\n";
            n << "cmake_minimum_required(VERSION 4.0)\n";
            n << "project(" << projectName << " LANGUAGES " << languages << ")\n\n";

            if (!comp->isMSVC()) {
                if (hasCxx)
                    n << "set(CMAKE_CXX_COMPILER \"" << escapeValue(comp->binary()) << "\")\n";
                if (hasC)
                    n << "set(CMAKE_C_COMPILER \"" << escapeValue(comp->binary()) << "\")\n";
                n << "\n";
            }

            if (!std.empty() && hasCxx) {
                n << "set(CMAKE_CXX_STANDARD " << std << ")\n";
                n << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";
            }

            if (isStatic)
                n << "add_library(" << targetName << " STATIC\n";
            else
                n << "add_executable(" << targetName << "\n";
            for (const auto &f : files)
                n << "    " << f << "\n";
            n << ")\n\n";

            if (!cfg.includeFiles.empty()) {
                n << "target_include_directories(" << targetName << " PRIVATE\n";
                for (const auto &inc : cfg.includeFiles)
                    n << "    " << inc << "\n";
                n << ")\n\n";
            }

            if (!compileFlags.empty()) {
                n << "target_compile_options(" << targetName << " PRIVATE\n";
                for (const auto &f : compileFlags)
                    n << "    " << f << "\n";
                n << ")\n\n";
            }

            if (!linkFlags.empty()) {
                n << "target_link_libraries(" << targetName << " PRIVATE\n";
                for (const auto &f : linkFlags)
                    n << "    " << f.substr(2) << "\n";
                n << ")\n\n";
            }

            if (!archives.empty()) {
                n << "target_link_options(" << targetName << " PRIVATE\n";
                for (const auto &a : archives)
                    n << "    \"" << escapeValue(a) << "\"\n";
                n << ")\n\n";
            }

            if (!cfg.pch.empty()) {
                n << "target_precompile_headers(" << targetName << " PRIVATE\n";
                n << "    " << cfg.pch << "\n";
                n << ")\n\n";
            }

            std::string outDir = outPath.parent_path().string();
            if (!outDir.empty()) {
                std::filesystem::path outAbs = std::filesystem::absolute(outDir);
                n << "set_target_properties(" << targetName << " PROPERTIES\n";
                n << "    RUNTIME_OUTPUT_DIRECTORY \"" << escapeValue(outAbs.generic_string()) << "\"\n";
                n << "    ARCHIVE_OUTPUT_DIRECTORY \"" << escapeValue(outAbs.generic_string()) << "\"\n";
                n << ")\n\n";
            }

            std::filesystem::path OutPath = projectRoot / std::string(Filename());
            return writeIfChanged(OutPath, n.str());
        }
    };

    std::unique_ptr<Generator> makeCMake() {
        return std::make_unique<CMake>();
    }

} // namespace hm::gen