#include "hm/template/template.hpp"

#include <fstream>

namespace hm {

    bool Template::create_tree() const {
        for (const auto &folder : get_folders()) {
            std::filesystem::create_directories(folder);
        }
        return true;
    }

    bool Template::fill_files() const {
        auto write_map = [](const std::map<std::filesystem::path, std::string> &files) -> bool {
            for (const auto &[path, content] : files) {
                if (!path.parent_path().empty()) {
                    std::filesystem::create_directories(path.parent_path());
                }
                std::ofstream out(path);
                if (!out)
                    return false;
                out << content;
            }
            return true;
        };

        if (!write_map(get_src_files()))
            return false;
        if (!write_map(get_txt_files()))
            return false;
        if (!write_map(get_hm_files()))
            return false;
        return true;
    }

    std::unique_ptr<Template> makeTemplate(std::string_view name, std::string_view project_name) {
        if (name == "sdl")
            return makeSDLTemplate(project_name);
        if (name == "sfml")
            return makeSFMLTemplate(project_name);
        if (name == "raylib" || name == "raylib-cpp")
            return makeRaylibCppTemplate(project_name);
        if (name == "cpp")
            return makeCppTemplate(project_name);
        if (name == "c")
            return makeCTemplate(project_name);
        if (name == "zig")
            return makeZigTemplate(project_name);
        if (name == "go" || name == "golang")
            return makeGoTemplate(project_name);
        return nullptr;
    }

} // namespace hm