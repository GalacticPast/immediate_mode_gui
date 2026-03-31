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
            switch (elems[i].type)
            {
            case TYPE_NONE:
            case TYPE_WINDOW:
            case TYPE_BUTTON:
            case TYPE_RADIO_BUTTONS:
            case TYPE_CHECKBOXES:
            case TYPE_SLIDER:
            case TYPE_TEXT_FIELD:
            case TYPE_SCROLL_BAR:
            case TYPE_LAYOUT_NODE: {
                DrawRectangle(elems[i].position.x, elems[i].position.y, elems[i].dimensions.width,
                              elems[i].dimensions.height, (Color){elems[i].background_color.r, elems[i].background_color.g, elems[i].background_color.b, elems[i].background_color.a});
            }
            break;
            case TYPE_TEXT: {
                DrawText(elems[i].label, elems[i].position.x, elems[i].position.y, 12, (Color){elems[i].text_color.r, elems[i].text_color.g, elems[i].text_color.b, elems[i].text_color.a});
            }
            break;
            }
#else
            DrawRectangle(elems[i].position.x, elems[i].position.y, elems[i].dimensions.width,
                          elems[i].dimensions.height, WHITE);
#endif
        }

        EndDrawing();
    }
    CloseWindow();
}
