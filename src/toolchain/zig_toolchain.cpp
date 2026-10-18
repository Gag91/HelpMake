#include "hm/toolchain/compiler.hpp"

namespace hm {

    class ZigCompiler : public Compiler {
      public:
        std::string name() const override {
            return "zig";
        }
        std::string binary() const override {
            return "zig";
        }
        std::string archiveBinary() const override {
            return "ar";
        }
        std::string objectExt() const override {
            return "";
        }
        std::string pchExtension() const override {
            return "";
        }

        std::string includeFlag(const std::string &p) const override {
            return "";
        }

        std::vector<std::string> pchCreateArgs(const std::string &, const std::string &) const override {
            return {};
        }
        std::vector<std::string> pchUseArgs(const std::string &, const std::string &) const override {
            return {};
        }

        std::vector<std::string> compileArgs(const std::string &src) const override {
            return {};
        }

        std::vector<std::string> objectOutputArgs(const std::string &) const override {
            return {};
        }

        std::vector<std::string> executableOutputArgs(const std::string &out) const override {
            return {"-femit-bin=", out};
        }

        std::vector<std::string> depArgs() const override {
            return {};
        }

        bool writesDepFile() const override {
            return false;
        }

        bool supportsSeparate() const override {
            return false;
        }

        bool isZig() const override {
            return true;
        }

        std::vector<std::string> directBuildArgs(const std::vector<std::string> &inputs, const std::string &output) const override {
            std::vector<std::string> args = inputs;
            args.push_back("-femit-bin=" + output);
            return args;
        }
    };

    std::unique_ptr<Compiler> makeZigCompiler() {
        return std::make_unique<ZigCompiler>();
    }

} // namespace hm