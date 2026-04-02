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
    Vector2               mouse_pos   = {};
    ui_mouse_button_state mouse_state = 0;

    b8 a = false;
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
            ui_column()
            {
                ui_label("Press me: ");
                if (ui_button("Press"))
                {
                    printf("Hellooo you pressed me. \n");
                }
            }
            ui_button("Hello ?");
            ui_checkbox("Checkbox", &a);
            static s32 choice = -1;
            ui_radio_button("radio 0", &choice, 0);
            ui_radio_button("radio 1", &choice, 1);
            ui_radio_button("radio 2", &choice, 2);
        }

        db_array(ui_elem) elems = ui_get_render_commands();

        for (s32 i = 0; i < db_array_get_count(elems); i++)
        {
            Color   background_color = *(struct Color *)((void *)&elems[i].background_color);
            Color   text_color       = *(struct Color *)((void *)&elems[i].text_color);
            Vector2 position         = *(struct Vector2 *)((void *)&elems[i].position);
            switch (elems[i].type)
            {
            case TYPE_NONE:
            case TYPE_WINDOW:
            case TYPE_BUTTON:
            case TYPE_CHECKBOX:
            case TYPE_SLIDER:
            case TYPE_TEXT_FIELD:
            case TYPE_SCROLL_BAR: {
#if 0

                DrawRectangleLines(elems[i].position.x, elems[i].position.y, elems[i].dimensions.width,
                                   elems[i].dimensions.height, background_color);
#else
                DrawRectangle(elems[i].position.x, elems[i].position.y, elems[i].dimensions.width,
                              elems[i].dimensions.height, background_color);
                b8 res = elems[i].type & TYPE_CHECKBOX;
                if (res) // @refactor: this is kinda shit
                {
                    Vector2 common_point = *(Vector2 *)((void *)&elems[i].check_common_start);
                    Vector2 first_half   = *(Vector2 *)((void *)&elems[i].check_first_half_end);
                    Vector2 second_half  = *(Vector2 *)((void *)&elems[i].check_second_half_end);
                    DrawLineEx(common_point, first_half, 3, WHITE);
                    DrawLineEx(common_point, second_half, 3, WHITE);
                }
#endif
            }
            case TYPE_LAYOUT_NODE: {
                DrawRectangleLines(elems[i].position.x, elems[i].position.y, elems[i].dimensions.width,
                                   elems[i].dimensions.height, WHITE);
            }
            break;
                break;
            case TYPE_TEXT: {
                DrawText(elems[i].label, elems[i].position.x, elems[i].position.y, 12, text_color);
            }
            break;
            case TYPE_RADIO_BUTTON: {
                Vector2 center = *(Vector2 *)((void *)&elems[i].radio_button_center);
                DrawCircleV(center, elems[i].dimensions.width / 2, background_color);
                // DrawRectangle(elems[i].position.x, elems[i].position.y, elems[i].dimensions.width,
                //               elems[i].dimensions.height, background_color);
            }
            break;
            }
        }
        EndDrawing();
    }
    CloseWindow();
}
