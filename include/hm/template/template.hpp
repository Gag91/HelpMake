#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hm {

    class Template {
      protected:
        std::string project_name_;
        std::string project_identifier() const {
            std::string identifier = project_name_;
            if (!identifier.empty() && !((identifier.front() >= 'A' && identifier.front() <= 'Z') ||
                                         (identifier.front() >= 'a' && identifier.front() <= 'z') ||
                                         identifier.front() == '_')) {
                identifier.front() = '_';
            }
            for (char &character : identifier) {
                if (!((character >= 'A' && character <= 'Z') ||
                      (character >= 'a' && character <= 'z') ||
                      (character >= '0' && character <= '9') ||
                      character == '_')) {
                    character = '_';
                }
            }
            return identifier;
        }

        virtual std::map<std::filesystem::path, std::string> get_src_files() const = 0;
        virtual std::map<std::filesystem::path, std::string> get_txt_files() const = 0;
        virtual std::map<std::filesystem::path, std::string> get_hm_files() const = 0;
        virtual std::vector<std::filesystem::path> get_folders() const = 0;

      public:
        explicit Template(std::string name) : project_name_(std::move(name)) {}
        virtual ~Template() = default;

        virtual std::string name() const = 0;

        bool create_tree() const;
        bool fill_files() const;
    };

    std::unique_ptr<Template> makeTemplate(std::string_view name, std::string_view project_name);
    std::unique_ptr<Template> makeSDLTemplate(std::string_view project_name);
    std::unique_ptr<Template> makeSFMLTemplate(std::string_view project_name);
    std::unique_ptr<Template> makeRaylibCppTemplate(std::string_view project_name);
    std::unique_ptr<Template> makeCppTemplate(std::string_view project_name);
    std::unique_ptr<Template> makeCTemplate(std::string_view project_name);
    std::unique_ptr<Template> makeZigTemplate(std::string_view project_name);
    std::unique_ptr<Template> makeGoTemplate(std::string_view project_name);

} // namespace hm