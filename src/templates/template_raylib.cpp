#include "hm/template/template.hpp"

namespace hm {

    class RaylibCppTemplate : public Template {
      public:
        using Template::Template;
        std::string name() const override {
            return "raylib-cpp";
        }

      protected:
        std::map<std::filesystem::path, std::string> get_src_files() const override {
            return {
                {"src/main.cpp",
                 "#include \"raylib.h\"\n\n"
                 "int main() {\n"
                 "    InitWindow(800, 600, \"" +
                     project_name_ + "\");\n"
                                     "    SetTargetFPS(60);\n\n"
                                     "    while (!WindowShouldClose()) {\n"
                                     "        BeginDrawing();\n"
                                     "        ClearBackground(RAYWHITE);\n"
                                     "        DrawText(\"Hello, Raylib!\", 190, 200, 20, LIGHTGRAY);\n"
                                     "        EndDrawing();\n"
                                     "    }\n\n"
                                     "    CloseWindow();\n"
                                     "    return 0;\n"
                                     "}\n"},
                {"include/game.hpp",
                 "#pragma once\n\n"
                 "namespace " +
                     project_identifier() + " {\n"
                                     "    void run();\n"
                                     "}\n"}};
        }

        std::map<std::filesystem::path, std::string> get_txt_files() const override {
            return {{"README.md", "# " + project_name_ + "\n\nA C++ project using Raylib.\n"}};
        }

        std::map<std::filesystem::path, std::string> get_hm_files() const override {
            return {
                {"HelpMake.txt",
                 "Compiler: g++\n"
                 "Version: std=c++23\n"
                 "Output: build/" +
                     project_name_ + "\n\n"
                                     "Includes {\n"
                                     "    include\n"
                                     "}\n\n"
                                     "InputFiles {\n"
                                     "    src/*.cpp\n"
                                     "}\n\n"
                                     "Dependency: raylib {\n"
                                     "    Git: https://github.com/raysan5/raylib\n"
                                     "    Tag: 5.5\n"
                                     "    Build: cmake -B build -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=OFF\n"
                                     "    Build: cmake --build build --config Release\n"
                                     "    Include: src\n"
                                     "    Link: build/raylib/libraylib.a\n"
                                     "    LinkFlags: -lm -lpthread -lwinmm -lgdi32 -lopengl32 -lshell32 -lole32\n"
                                     "}\n\n"
                                     "Depends: raylib\n"},
            };
        }

        std::vector<std::filesystem::path> get_folders() const override {
            return {"src", "include", "build"};
        }
    };

    std::unique_ptr<Template> makeRaylibCppTemplate(std::string_view project_name) {
        return std::make_unique<RaylibCppTemplate>(std::string(project_name));
    }
} // namespace hm
