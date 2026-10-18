#include "hm/toolchain/compiler.hpp"

namespace hm {

    class GccCompiler : public Compiler {
        bool isClang;

      public:
        explicit GccCompiler(bool clang) : isClang(clang) {}

        std::string name() const override {
            return isClang ? "clang" : "gcc";
        }
        std::string binary() const override {
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
            return {"-x", "c++-header", headerPath, "-o", outputPath};
        }

        std::vector<std::string> pchUseArgs(const std::string &headerPath, const std::string &binaryPath) const override {
            if (isClang)
                return {"-include-pch", binaryPath};

            return {"-include", headerPath};
        }

        std::vector<std::string> compileArgs(const std::string &src) const override {
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

    std::unique_ptr<Compiler> makeGccCompiler() {
        return std::make_unique<GccCompiler>(false);
    }

    std::unique_ptr<Compiler> makeClangCompiler() {
        return std::make_unique<GccCompiler>(true);
    }

} // namespace hm