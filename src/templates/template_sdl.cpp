#include "hm/template/template.hpp"

namespace hm {

    class SDLTemplate : public Template {
      public:
        using Template::Template;
        std::string name() const override {
            return "sdl";
        }

      protected:
        std::map<std::filesystem::path, std::string> get_src_files() const override {
            return {
                {"src/main.cpp",
                 "#include \"game.hpp\"\n"
                 "#include <SDL2/SDL.h>\n\n"
                 "int main(int argc, char* argv[]) {\n"
                 "    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;\n"
                 "    SDL_Window* window = SDL_CreateWindow(\"" +
                     project_name_ + "\", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);\n"
                                     "    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);\n"
                                     "    bool running = true;\n"
                                     "    SDL_Event event;\n"
                                     "    while (running) {\n"
                                     "        while (SDL_PollEvent(&event)) {\n"
                                     "            if (event.type == SDL_QUIT) running = false;\n"
                                     "        }\n"
                                     "        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);\n"
                                     "        SDL_RenderClear(renderer);\n"
                                     "        SDL_RenderPresent(renderer);\n"
                                     "    }\n"
                                     "    SDL_DestroyRenderer(renderer);\n"
                                     "    SDL_DestroyWindow(window);\n"
                                     "    SDL_Quit();\n"
                                     "    return 0;\n"
                                     "}\n"},
                {"include/game.hpp", "#pragma once\n\nnamespace " + project_identifier() + " {\n    void run();\n}\n"}};
        }

        std::map<std::filesystem::path, std::string> get_txt_files() const override {
            return {
                {"README.md",
                 "# " + project_name_ + "\n\n"
                                        "A C++ project using SDL2.\n\n"
                                        "## Structure\n"
                                        "- `src/` - Source files\n"
                                        "- `include/` - Header files\n"
                                        "- `build/` - Compiled binaries\n"},
            };
        }

        std::map<std::filesystem::path, std::string> get_hm_files() const override {
            return {
                {"HelpMake.txt",
                 "Dependency: SDL2 {\n"
                 "    Git: https://github.com/libsdl-org/SDL\n"
                 "    Tag: release-2.30.11\n"
                 "    win: Build: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=build/install -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF\n"
                 "    !win: Build: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=build/install -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST=OFF\n"
                 "    Build: cmake --build build --config Release\n"
                 "    Build: cmake --install build --config Release\n"
                 "    Include: build/install/include\n"
                 "    LinkDir: build/install/lib\n"
                 "    win: Link: build/install/lib/libSDL2main.a\n"
                 "    win: Link: build/install/lib/libSDL2.dll.a\n"
                 "    win: LinkFlags: -lmingw32 -mwindows\n"
                 "    linux: Link: build/install/lib/libSDL2.a\n"
                 "    linux: LinkFlags: -pthread -ldl\n"
                 "    mac: Link: build/install/lib/libSDL2.a\n"
                 "    mac: LinkFlags: -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio\n"
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
                                     "    -Wall\n"
                                     "}\n\n"
                                     "Copy {\n"
                                     "    win: build/HelpMake/dep/SDL2/build/install/bin/SDL2.dll -> build\n"
                                     "}\n\n"
                                     "Depends: SDL2\n"}};
        }

        std::vector<std::filesystem::path> get_folders() const override {
            return {"src", "include", "build"};
        }
    };

    std::unique_ptr<Template> makeSDLTemplate(std::string_view project_name) {
        return std::make_unique<SDLTemplate>(std::string(project_name));
    }
} // namespace hm