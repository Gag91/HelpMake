#include "hm/toolchain/compiler.hpp"

namespace hm {

    class GoCompiler : public Compiler {
      public:
        std::string name() const override {
            return "go";
        }

        std::string binary() const override {
            return "go";
        }

        std::vector<std::string> subcommand() const override {
            return {"build"};
        }

        std::string archiveBinary() const override {
            return "";
        }

        std::string objectExt() const override {
            return "";
        }

        std::string pchExtension() const override {
            return "";
        }

        std::string includeFlag(const std::string &) const override {
            return "";
        }

        std::vector<std::string> pchCreateArgs(const std::string &, const std::string &) const override {
            return {};
        }

        std::vector<std::string> pchUseArgs(const std::string &, const std::string &) const override {
            return {};
        }

        std::vector<std::string> compileArgs(const std::string &) const override {
            return {};
        }

        std::vector<std::string> objectOutputArgs(const std::string &) const override {
            return {};
        }

        std::vector<std::string> executableOutputArgs(const std::string &out) const override {
            return {"-o", out};
        }

        std::vector<std::string> depArgs() const override {
            return {};
        }

        std::vector<std::string> directBuildArgs(const std::vector<std::string> &inputs,
                                                 const std::string &output) const override {
            std::vector<std::string> args = inputs;
            args.push_back("-o");
            args.push_back(output);
            return args;
        }

        bool writesDepFile() const override {
            return false;
        }

        bool supportsSeparate() const override {
            return false;
        }

        bool isGo() const override {
            return true;
        }
    };

    std::unique_ptr<Compiler> makeGoCompiler() {
        return std::make_unique<GoCompiler>();
    }

} // namespace hm