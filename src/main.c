#define DB_IMPLEMENTATION
#include "db.h"

#include "raylib.h"
#include "ui.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

vector2d ui_measure_text(const char *text, u32 font_size)
{
    s32 text_width = MeasureText(text, font_size);
    return (vector2d){text_width, 0};
}

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Immediate mode gui");
    ui_init(&ui_measure_text);

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(BLACK);

        if (ui_window("Hello world"))
        {
        }

        db_array(ui_elem) elems = ui_get_render_commands();

        for (s32 i = 0; i < db_array_get_count(elems); i++)
        {
            DrawRectangle(elems[i].pos.x, elems[i].pos.y, elems[i].dimensions.width, elems[i].dimensions.height, WHITE);
        }

        EndDrawing();
    }
    CloseWindow();
}
