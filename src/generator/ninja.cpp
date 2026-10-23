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

    class Ninja : public Generator {
      private:
        static std::string join(const std::vector<std::string> &vec, const std::string &sep = " ") {
            std::string s;
            for (size_t i = 0; i < vec.size(); ++i) {
                if (i)
                    s += sep;
                s += vec[i];
            }
            return s;
        }

      public:
        std::string_view name() const override {
            return "ninja";
        }
        std::string_view Filename() const override {
            return "build.ninja";
        }
        std::string_view description() const override {
            return "Ninja build system: builds from build.ninja";
        }

        std::string escapeValue(const std::string &s) {
            std::string out;
            for (char c : s) {
                if (c == '$')
                    out += "$$";
                else
                    out += c;
            }
            return out;
        }

        std::string escapePath(const std::string &s) {
            std::string out;
            for (char c : s) {
                switch (c) {
                    case '$':
                        out += "$$";
                        break;
                    case ' ':
                        out += "$ ";
                        break;
                    case ':':
                        out += "$:";
                        break;
                    case '\\':
                        out += '/';
                        break;
                    default:
                        out += c;
                        break;
                }
            }
            return out;
        }

        std::string escapePathList(const std::vector<std::string> &v) {
            std::string out;
            for (size_t i = 0; i < v.size(); ++i) {
                if (i)
                    out += ' ';
                out += escapePath(v[i]);
            }
            return out;
        }

        bool generate(Config &cfg, const std::filesystem::path &projectRoot) override {

            std::filesystem::create_directories("build/HelpMake/Ninja");
            auto comp = hm::makeCompiler(cfg.compiler);
            if (!comp) {
                hp::printlnCl("[Ninja] Unknown compiler '" + cfg.compiler + "'", hp::RED);
                return false;
            }

            if (comp->name() == "msvc") {
                hp::printlnCl("[Ninja] MSVC is not supported (different flag syntax).\nUse `hm -b` or wait for the CMake generator.", hp::YELLOW);
                return false;
            }

            if (!comp->supportsSeparate()) {
                hp::printlnCl(std::format("[Ninja] '{}' is one-shot only (no separate compilation).\nNinja needs per-file objects. Use `hm -b` for this toolchain.", cfg.compiler), hp::YELLOW);
                return false;
            }

            if (!cfg.pch.empty())
                cfg.v_Info["[Ninja] Warning: PCH is not emitted. Use `hm -b` if you need it."] = hp::YELLOW;

            if (!cfg.Precmd.empty() || !cfg.v_Precmd.empty())
                cfg.v_Info["[Ninja] Warning: PreBuild is not emitted."] = hp::YELLOW;

            if (!cfg.Postcmd.empty() || !cfg.v_Postcmd.empty())
                cfg.v_Info["[Ninja] Warning: PostBuild is not emitted."] = hp::YELLOW;

            if (!cfg.v_Copy.empty())
                cfg.v_Info["[Ninja] Warning: Copy is not emitted."] = hp::YELLOW;

            if (!cfg.v_Github.empty())
                cfg.v_Info["[Ninja] Warning: Fetch is not emitted."] = hp::YELLOW;

            if (!cfg.v_Modules.empty())
                cfg.v_Info["[Ninja] Warning: Modules have no special handling; "
                           "they'll be compiled like normal sources (may fail on Clang)."] = hp::YELLOW;

            std::vector<std::string> files = expandInputs(cfg);
            if (files.empty()) {
                hp::printlnCl("[Ninja] No files matched InputFiles { }.\nCheck your glob patterns.", hp::RED);
                return false;
            }

            std::string version = cfg.version;
            if (!version.empty() && version.front() != '-')
                version.insert(version.begin(), '-');

            std::string combined = version;
            if (!cfg.flags.empty())
                combined += (combined.empty() ? "" : " ") + cfg.flags;

            std::string compileFlags, linkFlags;
            {
                std::istringstream iss(combined);
                std::string tok;
                while (iss >> tok) {
                    if (tok.rfind("-l", 0) == 0)
                        linkFlags += (linkFlags.empty() ? "" : " ") + tok;
                    else
                        compileFlags += (compileFlags.empty() ? "" : " ") + tok;
                }
            }

            std::string cxx = comp->binary();
            for (auto &s : comp->subcommand())
                cxx += " " + s;

            bool isStatic = to_lower(cfg.kind) == "static";

            std::string objDir = "build/HelpMake/" + comp->name() + "/obj/";

            std::vector<std::string> objects;
            objects.reserve(files.size());

            std::ostringstream n;
            n << "# Generated by HelpMake. Do not edit.\n";
            n << "# Regenerate with: hm --generate ninja\n\n";
            n << "ninja_required_version = 1.10\n\n";

            n << "builddir = build/HelpMake/Ninja\n\n";

            n << "cxx     = " << escapeValue(cxx) << "\n";
            n << "cflags  = " << escapeValue(compileFlags) << "\n";
            n << "ldflags = " << escapeValue(linkFlags) << "\n\n";

            n << "rule cxx_compile\n";
            n << "  command = $cxx $cflags -MMD -MF $out.d -c $in -o $out\n";
            n << "  depfile = $out.d\n";
            n << "  deps = gcc\n";
            n << "  description = CXX $out\n\n";

            if (isStatic) {
                n << "rule archive\n";
                n << "  command = " << comp->archiveBinary() << " rcs $out $in\n";
                n << "  description = AR $out\n\n";
            } else {
                n << "rule link\n";
                n << "  command = $cxx $in $ldflags -o $out\n";
                n << "  description = LINK $out\n\n";
            }

            for (const auto &src : files) {
                std::string rel = std::filesystem::path(src).generic_string();
                std::string obj = objDir + rel + comp->objectExt();
                objects.push_back(obj);
                n << "build " << escapePath(obj) << ": cxx_compile " << escapePath(src) << "\n";
            }
            n << "\n";

            n << "build " << escapePath(cfg.output) << ": "
              << (isStatic ? "archive" : "link") << " "
              << escapePathList(objects) << "\n";
            n << "default " << escapePath(cfg.output) << "\n";

            std::filesystem::path outPath = projectRoot / std::string(Filename());
            return writeIfChanged(outPath, n.str());
        }
    };

    std::unique_ptr<Generator> makeNinja() {
        return std::make_unique<Ninja>();
    }

} // namespace hm::gen