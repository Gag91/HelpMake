#include "hm/toolchain/compiler.hpp"

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

        std::string includeFlag(const std::string &p) const override {
            return "/I" + p;
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

        std::vector<std::string> depArgs() const override {
            return {};
        }

        bool writesDepFile() const override {
            return false;
        }
    };

    std::unique_ptr<Compiler> makeMsvcCompiler() {
        return std::make_unique<MsvcCompiler>();
    }

} // namespace hm