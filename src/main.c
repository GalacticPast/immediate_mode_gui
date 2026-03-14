#include "ui.h"
#include "raylib.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Immediate mode gui");

    while (!WindowShouldClose())
    {
        BeginDrawing();
        
        ClearBackground(BLACK);

        EndDrawing();
    }
    CloseWindow();
}
