#pragma once

#include <memory>
#include <string>
#include <vector>

namespace hm {

    class Compiler {
      public:
        virtual ~Compiler() = default;

        virtual std::string name() const = 0;
        virtual std::string binary() const = 0;
        virtual std::string archiveBinary() const = 0;
        virtual std::string objectExt() const = 0;
        virtual std::string pchExtension() const = 0;
        virtual std::string includeFlag(const std::string &path) const = 0;
        virtual std::string pchSourceFileContent(const std::string &content) const {
            return "";
        }

        virtual std::vector<std::string> pchCreateArgs(const std::string &headerPath, const std::string &outputPath) const = 0;
        virtual std::vector<std::string> pchUseArgs(const std::string &headerPath, const std::string &binaryPath) const = 0;
        virtual std::vector<std::string> compileArgs(const std::string &source) const = 0;
        virtual std::vector<std::string> objectOutputArgs(const std::string &obj) const = 0;
        virtual std::vector<std::string> executableOutputArgs(const std::string &out) const = 0;
        virtual std::vector<std::string> depArgs() const = 0;
        virtual std::vector<std::string> directBuildArgs(const std::vector<std::string> &inputs, const std::string &output) const = 0;
        virtual std::vector<std::string> subcommand() const {
            return {};
        }

        virtual bool pchNeedsSourceFile() const {
            return false;
        }

        virtual bool supportsSeparate() const {
            return true;
        }

        virtual bool isGcc() const {
            return false;
        }

        virtual bool isClang() const {
            return false;
        }

        virtual bool isMSVC() const {
            return false;
        }

        virtual bool isZig() const {
            return false;
        }

        virtual bool isZigC() const {
            return false;
        }

        virtual bool isZigCpp() const {
            return false;
        }

        virtual bool writesDepFile() const = 0;
    };

    std::unique_ptr<Compiler> makeCompiler(const std::string &name);

    std::unique_ptr<Compiler> makeGccCompiler();
    std::unique_ptr<Compiler> makeClangCompiler();
    std::unique_ptr<Compiler> makeMsvcCompiler();
    std::unique_ptr<Compiler> makeZigCompiler();
    std::unique_ptr<Compiler> makeZigCCompiler();
    std::unique_ptr<Compiler> makeZigCppCompiler();

} // namespace hm