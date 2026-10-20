#include "hm/toolchain/compiler.hpp"
#include <string_view>

namespace hm {

    class GccCompiler : public Compiler {
        bool isClang;
        bool isC;

      public:
        explicit GccCompiler(bool clang, bool isC = false) : isClang(clang), isC(isC) {}

        std::string name() const override {
            return isClang ? "clang" : "gcc";
        }
        std::string binary() const override {
            if (isC && isClang)
                return "clang";
            if (isC)
                return "gcc";
            return isClang ? "clang++" : "g++";
        }
        std::string archiveBinary() const override {
            return "ar";
        }
        std::string objectExt() const override {
            return ".o";
        }
        std::string pchExtension() const override {
            return isClang ? ".pch" : ".gch";
        }

        std::string includeFlag(const std::string &p) const override {
            return "-I" + p;
        }

        std::vector<std::string> pchCreateArgs(const std::string &headerPath, const std::string &outputPath) const override {
            if (!isC)
                return {"-x", "c++-header", headerPath, "-o", outputPath};
            return {"-x", "c-header", headerPath, "-o", outputPath};
        }

        std::vector<std::string> pchUseArgs(const std::string &headerPath, const std::string &binaryPath) const override {
            if (isClang)
                return {"-include-pch", binaryPath};

            return {"-include", headerPath};
        }

        std::vector<std::string> compileArgs(const std::string &src) const override {
            if (src.ends_with(".c"))
                return {"-fdiagnostics-color=always", "-xc", "-c", src};
            if (isClang)
                return {"-fdiagnostics-color=always", "-c", src};
            return {"-fdiagnostics-color=always", "-xc++", "-c", src};
        }

        std::vector<std::string> objectOutputArgs(const std::string &obj) const override {
            return {"-o", obj};
        }

        std::vector<std::string> executableOutputArgs(const std::string &out) const override {
            return {"-o", out};
        }

        std::vector<std::string> depArgs() const override {
            return {"-MMD"};
        }

        std::vector<std::string> directBuildArgs(const std::vector<std::string> &inputs, const std::string &output) const override {
            std::vector<std::string> args = inputs;
            args.push_back("-o");
            args.push_back(output);
            return args;
        }

        bool writesDepFile() const override {
            return true;
        }

        bool pchNeedsSourceFile() const override {
            return false;
        }
    };

    std::unique_ptr<Compiler> makeGccCompiler(std::string_view name) {
        if (name == "gcc")
            return std::make_unique<GccCompiler>(false, true);
        return std::make_unique<GccCompiler>(false);
    }

    std::unique_ptr<Compiler> makeClangCompiler(std::string_view name) {
        if (name == "clang")
            return std::make_unique<GccCompiler>(true, true);
        return std::make_unique<GccCompiler>(true);
    }

} // namespace hm