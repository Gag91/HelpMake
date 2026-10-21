#include "raylib.h"

int main(void) {
    InitWindow(800, 450, "HelpMake + raylib");
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Hello from HelpMake!", 250, 200, 30, DARKGRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}