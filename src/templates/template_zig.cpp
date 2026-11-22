#include "hm/template/template.hpp"

namespace hm {

    class ZigTemplate : public Template {
      public:
        using Template::Template;
        std::string name() const override {
            return "zig";
        }

      protected:
        std::map<std::filesystem::path, std::string> get_src_files() const override {
            return {
                {"src/main.zig",
                 "const std = @import(\"std\");\n\n"
                 "pub fn main() !void {\n"
                 "    std.debug.print(\"Hello from " +
                     project_name_ + "!\\n\", .{});\n"
                                     "}\n"}};
        }

        std::map<std::filesystem::path, std::string> get_txt_files() const override {
            return {{"README.md", "# " + project_name_ + "\n\nA basic Zig project.\n"}};
        }

        std::map<std::filesystem::path, std::string> get_hm_files() const override {
            return {
                {"HelpMake.txt",
                 "Compiler: zig\n"
                 "Output: build/" +
                     project_name_ + "\n\n"
                                     "InputFiles {\n"
                                     "    src/*.zig\n"
                                     "}\n"}};
        }

        std::vector<std::filesystem::path> get_folders() const override {
            return {"src", "build"};
        }
    };

    std::unique_ptr<Template> makeZigTemplate(std::string_view project_name) {
        return std::make_unique<ZigTemplate>(std::string(project_name));
    }
} // namespace hm