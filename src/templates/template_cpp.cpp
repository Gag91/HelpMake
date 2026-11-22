#include "hm/template/template.hpp"

namespace hm {

    class CppTemplate : public Template {
      public:
        using Template::Template;
        std::string name() const override {
            return "cpp";
        }

      protected:
        std::map<std::filesystem::path, std::string> get_src_files() const override {
            return {
                {"src/main.cpp",
                 "#include <iostream>\n"
                 "#include \"hello.hpp\"\n\n"
                 "int main() {\n"
                 "    std::cout << \"Hello from " +
                     project_name_ + "!\\n\";\n"
                                     "    return 0;\n"
                                     "}\n"},
                {"include/hello.hpp", "#pragma once\n\nnamespace " + project_identifier() + " {\n    void greet();\n}\n"}};
        }

        std::map<std::filesystem::path, std::string> get_txt_files() const override {
            return {{"README.md", "# " + project_name_ + "\n\nA basic C++ project.\n"}};
        }

        std::map<std::filesystem::path, std::string> get_hm_files() const override {
            return {
                {"HelpMake.txt",
                 "Compiler: g++\n"
                 "Version: std=c++20\n"
                 "Output: build/" +
                     project_name_ + "\n\n"
                                     "InputFiles {\n"
                                     "    src/*.cpp\n"
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

    std::unique_ptr<Template> makeCppTemplate(std::string_view project_name) {
        return std::make_unique<CppTemplate>(std::string(project_name));
    }
} // namespace hm