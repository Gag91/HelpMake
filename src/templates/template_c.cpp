#include "hm/template/template.hpp"

namespace hm {

    class CTemplate : public Template {
      public:
        using Template::Template;
        std::string name() const override {
            return "c";
        }

      protected:
        std::map<std::filesystem::path, std::string> get_src_files() const override {
            return {
                {"src/main.c",
                 "#include <stdio.h>\n"
                 "#include \"hello.h\"\n\n"
                 "int main() {\n"
                 "    printf(\"Hello from " +
                     project_name_ + "!\\n\");\n"
                                     "    return 0;\n"
                                     "}\n"},
                {"include/hello.h", "#pragma once\n\nvoid greet(void);\n"}};
        }

        std::map<std::filesystem::path, std::string> get_txt_files() const override {
            return {{"README.md", "# " + project_name_ + "\n\nA basic C project.\n"}};
        }

        std::map<std::filesystem::path, std::string> get_hm_files() const override {
            return {
                {"HelpMake.txt",
                 "Compiler: gcc\n"
                 "Version: std=c11\n"
                 "Output: build/" +
                     project_name_ + "\n\n"
                                     "InputFiles {\n"
                                     "    src/*.c\n"
                                     "}\n\n"
                                     "Includes {\n"
                                     "    include\n"
                                     "}\n\n"
                                     "Flags {\n"
                                     "    -Wall\n"
                                     "    -O2\n"
                                     "}\n"}};
        }

        std::vector<std::filesystem::path> get_folders() const override {
            return {"src", "include", "build"};
        }
    };

    std::unique_ptr<Template> makeCTemplate(std::string_view project_name) {
        return std::make_unique<CTemplate>(std::string(project_name));
    }
} // namespace hm