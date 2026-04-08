#define DB_IMPLEMENTATION
#include "db.h"

#include "raylib.h"
#include "ui.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

rectangle ui_measure_text(const char *text, u32 font_size)
{
    s32 text_width = MeasureText(text, font_size);
    return (rectangle){text_width, font_size}; // change this to be dynamic
}

int main()
{

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Immediate mode gui");
    SetTargetFPS(60); // Set target FPS (maximum)

    ui_init(&ui_measure_text);
    Vector2               mouse_pos   = {};
    ui_mouse_button_state mouse_state = 0;

    while (!WindowShouldClose())
    {
        mouse_pos   = GetMousePosition();
        mouse_state = 0;

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            mouse_state |= TYPE_MOUSE_LEFT_BUTTON_PRESSED;
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            mouse_state |= TYPE_MOUSE_LEFT_BUTTON_RELEASED;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            mouse_state |= TYPE_MOUSE_RIGHT_BUTTON_PRESSED;
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
        {
            mouse_state |= TYPE_MOUSE_RIGHT_BUTTON_RELEASED;
        }

        BeginDrawing();

        ClearBackground(BLACK);

        ui_update_mouse((vector2d){mouse_pos.x, mouse_pos.y}, mouse_state);

        ui_window_ext("Hello world", .position = {100, 100})
        {
            static b8 check = false;
            ui_checkbox("Checkbox", &check);
            static s32 choice = -1;
            ui_radio_button("radio 0", &choice, 0);
            ui_radio_button("radio 1", &choice, 1);
            ui_radio_button("radio 2", &choice, 2);

            static f32 r = 0;
            static f32 g = 0;
            static f32 b = 0;
            static f32 a = 0;
            ui_slider_ext("color",
                          .min        = 0,
                          .max        = 255,
                          .first_val  = &r,
                          .second_val = &g,
                          .third_val  = &b,
                          .fourth_val = &a);
        }

        db_array(ui_render_element) elems = ui_get_render_commands();

        for (s32 i = 0; i < db_array_get_count(elems); i++)
        {
            Vector2 position   = (Vector2){elems[i].position.x, elems[i].position.y};
            Vector2 dimensions = (Vector2){elems[i].dimensions.width, elems[i].dimensions.height};
            Color   color      = (Color){elems[i].color.r, elems[i].color.g, elems[i].color.b, elems[i].color.a};
            Vector2 center     = (Vector2){elems[i].center.x, elems[i].center.y};
            Vector2 start_pos  = (Vector2){elems[i].start_pos.x, elems[i].start_pos.y};
            Vector2 end_pos    = (Vector2){elems[i].end_pos.x, elems[i].end_pos.y};

            switch (elems[i].type)
            {
                case TYPE_RENDER_RECTANGLE:
                {
                    DrawRectangleV((Vector2)position, dimensions, color);
                }
                break;
                case TYPE_RENDER_TEXT:
                {
                    DrawText(elems[i].label, position.x, position.y, 14, color);
                }
                break;
                case TYPE_RENDER_CIRCLE:
                {
                    DrawCircleV(center, elems[i].radius, color);
                }
                break;
                case TYPE_RENDER_LINE:
                {
                    DrawLineV(start_pos, end_pos, color);
                }
                break;
                case TYPE_RENDER_NONE:
                default:
                    break;
            }
        }
        EndDrawing();
    }
    CloseWindow();
}
