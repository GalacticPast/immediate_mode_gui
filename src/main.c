#define DB_IMPLEMENTATION
#include "db.h"

#include "raylib.h"
#include "ui.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

rectangle ui_measure_text(const char *text, u32 font_size)
{
    s32 text_width = MeasureText(text, font_size);
    return (rectangle){text_width, 12}; // change this to be dynamic
}

int main()
{

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Immediate mode gui");
    SetTargetFPS(60); // Set target FPS (maximum)

    ui_init(&ui_measure_text);

    // test
    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(BLACK);

        ui_window_ext("Hello world", .position = {100, 100})
        {
            ui_row()
            {
                ui_button("Button1");
                ui_button("Button2");
                ui_button("Button3");
                ui_column()
                {
                    ui_button("Button8");
                    ui_button("Button11");
                    ui_button("Button9");
                }
            }
            ui_row()
            {
                ui_button("Button4");
                ui_button("Button6");
                ui_button("Button5");
            }
        }

        db_array(ui_elem) elems = ui_get_render_commands();

        for (s32 i = 0; i < db_array_get_count(elems); i++)
        {
#if 1
            // ui_elem *elem = &elems[i];
            DrawRectangleLines(elems[i].position.x, elems[i].position.y, elems[i].dimensions.width,
                               elems[i].dimensions.height, WHITE);
#else
            DrawRectangle(elems[i].position.x, elems[i].position.y, elems[i].dimensions.width,
                          elems[i].dimensions.height, WHITE);
#endif
        }

        EndDrawing();
    }
    CloseWindow();
}
