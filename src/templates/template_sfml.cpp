#include "hm/template/template.hpp"

namespace hm {

    class SFMLTemplate : public Template {
      public:
        using Template::Template;
        std::string name() const override {
            return "sfml";
        }

      protected:
        std::map<std::filesystem::path, std::string> get_src_files() const override {
            return {
                {"src/main.cpp",
                 "#include \"game.hpp\"\n"
                 "#include <SFML/Graphics.hpp>\n\n"
                 "int main() {\n"
                 "    sf::RenderWindow window(sf::VideoMode({800, 600}), \"" +
                     project_name_ + "\");\n"
                                     "    sf::CircleShape shape(100.f);\n"
                                     "    shape.setFillColor(sf::Color::Green);\n\n"
                                     "    while (window.isOpen()) {\n"
                                     "        while (const std::optional event = window.pollEvent()) {\n"
                                     "            if (event->is<sf::Event::Closed>()) {\n"
                                     "                window.close();\n"
                                     "            }\n"
                                     "        }\n\n"
                                     "        window.clear(sf::Color(30, 30, 30));\n"
                                     "        window.draw(shape);\n"
                                     "        window.display();\n"
                                     "    }\n"
                                     "    return 0;\n"
                                     "}\n"},
                {"include/game.hpp", "#pragma once\n\nnamespace " + project_identifier() + " {\n    void run();\n}\n"}};
        }

        std::map<std::filesystem::path, std::string> get_txt_files() const override {
            return {{"README.md", "# " + project_name_ + "\n\nA C++ project using SFML.\n"}};
        }

        std::map<std::filesystem::path, std::string> get_hm_files() const override {
            return {
                {"HelpMake.txt",
                 "Dependency: SFML {\n"
                 "    Git: https://github.com/SFML/SFML\n"
                 "    Tag: 3.0.2\n"
                 "    Build: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=build/install -DBUILD_SHARED_LIBS=OFF -DSFML_BUILD_EXAMPLES=OFF -DSFML_BUILD_TEST_SUITE=OFF -DSFML_BUILD_AUDIO=OFF -DSFML_BUILD_NETWORK=OFF\n"
                 "    Build: cmake --build build --config Release\n"
                 "    Build: cmake --install build --config Release\n"
                 "    Include: build/install/include\n"
                 "    LinkDir: build/install/lib\n"
                 "    Link: build/install/lib/libsfml-graphics-s.a\n"
                 "    Link: build/install/lib/libsfml-window-s.a\n"
                 "    Link: build/install/lib/libsfml-system-s.a\n"
                 "    win: LinkFlags: -lopengl32 -lwinmm -lgdi32\n"
                 "    linux: LinkFlags: -lX11 -lXrandr -lXcursor -lXi -lXinerama -lXxf86vm -lGL -lfreetype -pthread\n"
                 "    mac: LinkFlags: -framework AppKit -framework IOKit -framework CoreVideo -framework OpenGL -framework Carbon -framework Cocoa\n"
                 "}\n\n"
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
                                     "    -DSFML_STATIC\n"
                                     "}\n\n"
                                     "Depends: SFML\n"}};
        }

        std::vector<std::filesystem::path> get_folders() const override {
            return {"src", "include", "build"};
        }
    };

    std::unique_ptr<Template> makeSFMLTemplate(std::string_view project_name) {
        return std::make_unique<SFMLTemplate>(std::string(project_name));
    }
} // namespace hm