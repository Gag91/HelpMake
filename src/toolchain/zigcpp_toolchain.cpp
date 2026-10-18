#include "hm/toolchain/compiler.hpp"

namespace hm {

    class ZigCppCompiler : public Compiler {
      public:
        std::string name() const override {
            return "zig-c++";
        }

        std::string binary() const override {
            return "zig";
        }

        std::vector<std::string> subcommand() const override {
            return {"c++"};
        }

        std::string archiveBinary() const override {
            return "ar";
        }

        std::string objectExt() const override {
            return ".o";
        }

        std::string pchExtension() const override {
            return ".pch";
        }

        std::string includeFlag(const std::string &p) const override {
            return "-I" + p;
        }

        std::vector<std::string> pchCreateArgs(const std::string &headerPath,
                                               const std::string &outputPath) const override {
            return {"-x", "c++-header", headerPath, "-o", outputPath};
        }

        std::vector<std::string> pchUseArgs(const std::string &,
                                            const std::string &binaryPath) const override {
            return {"-include-pch", binaryPath};
        }

        std::vector<std::string> compileArgs(const std::string &src) const override {
            return {"-fdiagnostics-color=always", "-c", src};
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

        std::vector<std::string> directBuildArgs(const std::vector<std::string> &inputs,
                                                 const std::string &output) const override {
            std::vector<std::string> args = inputs;
            args.push_back("-o");
            args.push_back(output);
            return args;
        }

        bool writesDepFile() const override {
            return true;
        }
        bool isZigCpp() const override {
            return true;
        }
    };

    std::unique_ptr<Compiler> makeZigCppCompiler() {
        return std::make_unique<ZigCppCompiler>();
    }

} // namespace hm