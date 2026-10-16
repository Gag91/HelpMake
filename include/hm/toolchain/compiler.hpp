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

        virtual std::string includeFlag(const std::string &path) const = 0;

        virtual std::vector<std::string> compileArgs(const std::string &source) const = 0;
        virtual std::vector<std::string> objectOutputArgs(const std::string &obj) const = 0;
        virtual std::vector<std::string> executableOutputArgs(const std::string &out) const = 0;
        virtual std::vector<std::string> depArgs() const = 0;

        virtual bool writesDepFile() const = 0;
    };

    std::unique_ptr<Compiler> makeCompiler(const std::string &name);

    std::unique_ptr<Compiler> makeGccCompiler();
    std::unique_ptr<Compiler> makeClangCompiler();
    std::unique_ptr<Compiler> makeMsvcCompiler();

} // namespace hm