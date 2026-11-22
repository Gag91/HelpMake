#include "hm/template/template.hpp"

namespace hm {

    class GoTemplate : public Template {
      public:
        using Template::Template;
        std::string name() const override {
            return "go";
        }

      protected:
        std::map<std::filesystem::path, std::string> get_src_files() const override {
            return {{"main.go",
                     "package main\n\n"
                     "import \"fmt\"\n\n"
                     "func main() {\n"
                     "    fmt.Println(\"Hello from " +
                         project_name_ + "!\\n\")\n"
                                         "}\n"}};
        }

        std::map<std::filesystem::path, std::string> get_txt_files() const override {
            return {{"README.md", "# " + project_name_ + "\n\nA basic Go project.\n"}};
        }

        std::map<std::filesystem::path, std::string> get_hm_files() const override {
            return {{"HelpMake.txt",
                     "Compiler: go\n"
                     "Output: build/" +
                         project_name_ + "\n\n"
                                         "InputFiles {\n"
                                         "    main.go\n"
                                         "}\n"}};
        }

        std::vector<std::filesystem::path> get_folders() const override {
            return {"build"};
        }
    };

    std::unique_ptr<Template> makeGoTemplate(std::string_view project_name) {
        return std::make_unique<GoTemplate>(std::string(project_name));
    }

} // namespace hm
