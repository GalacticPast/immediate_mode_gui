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
    // test
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
            ui_row()
            {
                ui_label("Press me: ");
                ui_button("Press");
            }
            ui_row()
            {
                b8 a = false;
                ui_checkbox("Checkbox", &a);
            }
        }

        db_array(ui_elem) elems = ui_get_render_commands();

        for (s32 i = 0; i < db_array_get_count(elems); i++)
        {
            Color background_color = *(struct Color *)((void *)&elems[i].background_color);
            Color text_color       = *(struct Color *)((void *)&elems[i].text_color);

            switch (elems[i].type)
            {
            case TYPE_NONE:
            case TYPE_WINDOW:
            case TYPE_BUTTON:
            case TYPE_RADIO_BUTTONS:
            case TYPE_CHECKBOX:
            case TYPE_SLIDER:
            case TYPE_TEXT_FIELD:
            case TYPE_SCROLL_BAR:
            case TYPE_LAYOUT_NODE: {
#if 0

                DrawRectangleLines(elems[i].position.x, elems[i].position.y, elems[i].dimensions.width,
                                   elems[i].dimensions.height, background_color);
#else
                DrawRectangle(elems[i].position.x, elems[i].position.y, elems[i].dimensions.width,
                              elems[i].dimensions.height, background_color);
#endif
            }
            break;
            case TYPE_TEXT: {
                DrawText(elems[i].label, elems[i].position.x, elems[i].position.y, 12, text_color);
            }
            break;
            case TYPE_LINE: {
                DrawLine(elems[i].position.x, elems[i].position.y, 12, text_color);
            }
            break;
            }

            EndDrawing();
        }
        CloseWindow();
    }
}
