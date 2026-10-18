#include "hm/toolchain/compiler.hpp"

namespace hm {

    class GenericCompiler : public Compiler {
        std::string name_;

      public:
        explicit GenericCompiler(std::string name) : name_(std::move(name)) {}

        std::string name() const override {
            return name_;
        }
        std::string binary() const override {
            return name_;
        }
        std::string archiveBinary() const override {
            return "ar";
        }
        std::string objectExt() const override {
            return ".o";
        }

        std::string includeFlag(const std::string &p) const override {
            return "-I" + p;
        }

        std::string pchExtension() const override {
            return "";
        }

        std::vector<std::string> compileArgs(const std::string &src) const override {
            return {"-c", src};
        }

        std::vector<std::string> objectOutputArgs(const std::string &obj) const override {
            return {"-o", obj};
        }

        std::vector<std::string> executableOutputArgs(const std::string &out) const override {
            return {"-o", out};
        }

        std::vector<std::string> depArgs() const override {
            return {};
        }

        std::vector<std::string> pchCreateArgs(const std::string &, const std::string &) const override {
            return {};
        }

        std::vector<std::string> pchUseArgs(const std::string &, const std::string &) const override {
            return {};
        }

        bool writesDepFile() const override {
            return false;
        }
    };

    std::unique_ptr<Compiler> makeCompiler(const std::string &name) {
        if (name == "gcc" || name == "g++")
            return makeGccCompiler();
        if (name == "clang" || name == "clang++")
            return makeClangCompiler();
        if (name == "msvc" || name == "cl")
            return makeMsvcCompiler();
        return std::make_unique<GenericCompiler>(name);
    }

} // namespace hm