#define DBH_IMPLEMENTATION
#include "dbh.h"

#include "raylib.h"
#include "ui.h"


#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Immediate mode gui");
    ui_init();

    while (!WindowShouldClose())
    {
        BeginDrawing();
        
        ClearBackground(BLACK);
        
        if(ui_window("Hello world"))
        {
        
        }
        
        dbh_array(ui_elem) elems = ui_get_render_commands();
        
        for(s32 i = 0 ; i < dbh_array_get_count(elems) ; i++)
        {
            DrawRectangle(elems[i].pos.x,elems[i].pos.y, elems[i].dimensions.width, elems[i].dimensions.height, WHITE);
        }

        EndDrawing();
    }
    CloseWindow();
}
