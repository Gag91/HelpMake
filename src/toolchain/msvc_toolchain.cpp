#include "hm/toolchain/compiler.hpp"
#include <filesystem>

namespace hm {

    class MsvcCompiler : public Compiler {
      public:
        std::string name() const override {
            return "msvc";
        }
        std::string binary() const override {
            return "cl.exe";
        }
        std::string archiveBinary() const override {
            return "lib.exe";
        }
        std::string objectExt() const override {
            return ".obj";
        }

        std::string pchExtension() const override {
            return ".pch";
        }

        std::string includeFlag(const std::string &p) const override {
            return "/I" + p;
        }

        std::string pchSourceFileContent(const std::string &content) const override {
            std::string name = std::filesystem::path(content).filename().string();
            return "#include \"" + name + "\"\n";
        }

        std::vector<std::string> pchCreateArgs(const std::string &headerPath, const std::string &outputPath) const override {
            std::string bareName = std::filesystem::path(headerPath).filename().string();
            return {"/c", "/Yc" + bareName, "/Fp" + outputPath};
        }

        std::vector<std::string> pchUseArgs(const std::string &headerPath, const std::string &binaryPath) const override {
            std::string bareName = std::filesystem::path(headerPath).filename().string();
            return {"/Yu" + bareName, "/Fp" + binaryPath};
        }
        std::vector<std::string> compileArgs(const std::string &src) const override {
            return {"/nologo", "/c", src, "/showIncludes"};
        }

        std::vector<std::string> objectOutputArgs(const std::string &obj) const override {
            return {"/Fo:" + obj};
        }

        std::vector<std::string> executableOutputArgs(const std::string &out) const override {
            return {"/Fe:" + out};
        }

        std::vector<std::string> directBuildArgs(const std::vector<std::string> &inputs, const std::string &output) const override {
            std::vector<std::string> args = inputs;
            args.push_back("/Fe:" + output);
            return args;
        }

        std::vector<std::string> depArgs() const override {
            return {};
        }

        bool pchNeedsSourceFile() const override {
            return true;
        }

        bool writesDepFile() const override {
            return false;
        }

        bool isMSVC() const override {
            return true;
        }
    };

    std::unique_ptr<Compiler> makeMsvcCompiler() {
        return std::make_unique<MsvcCompiler>();
    }

} // namespace hm