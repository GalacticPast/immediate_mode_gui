#define DB_IMPLEMENTATION
#include "db.h"

#include "raylib.h"
#include "ui.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

static Font *ui_font;

rectangle ui_measure_text(const char *text, u32 font_size)
{
    ASSERT(font_size == 14);
    Vector2 text_width = MeasureTextEx(*ui_font, text, font_size, 2);
    return (rectangle){text_width.x, text_width.y}; // change this to be dynamic
}

int main()
{

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Immediate mode gui");
    SetTargetFPS(60); // Set target FPS (maximum)

    db_arena main_arena  = db_arena_init();
    db_arena frame_arena = db_arena_init();

    u32         size      = 0;
    const char *file_name = "src/Archivo-Regular.ttf";
    Font        font      = LoadFont(file_name);
    ui_font               = &font;

    ui_init(&size, 0, &ui_measure_text);
    void *ui_mem = db_arena_alloc(&main_arena, size);
    ui_init(&size, ui_mem, &ui_measure_text);

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

        BeginDrawing();

        ClearBackground(BLACK);

        db_array_render_elements elems = ui_get_render_commands();

        for (s32 i = 0; i < elems.length; i++)
        {
            Vector2 position   = (Vector2){elems.data[i].position.x, elems.data[i].position.y};
            Vector2 dimensions = (Vector2){elems.data[i].dimensions.width, elems.data[i].dimensions.height};
            Color   color      = (Color){elems.data[i].color.r, elems.data[i].color.g, elems.data[i].color.b, elems.data[i].color.a};
            Vector2 center     = (Vector2){elems.data[i].center.x, elems.data[i].center.y};
            Vector2 start_pos  = (Vector2){elems.data[i].start_pos.x, elems.data[i].start_pos.y};
            Vector2 end_pos    = (Vector2){elems.data[i].end_pos.x, elems.data[i].end_pos.y};

            Vector2 clip_position   = (Vector2){elems.data[i].clip_position.x, elems.data[i].clip_position.y};
            Vector2 clip_dimensions = (Vector2){elems.data[i].clip_dimensions.width, elems.data[i].clip_dimensions.height};

            BeginScissorMode(clip_position.x, clip_position.y, clip_dimensions.x, clip_dimensions.y);
            switch (elems.data[i].type)
            {
                case TYPE_RENDER_RECTANGLE:
                {
                    DrawRectangleV((Vector2)position, dimensions, color);
                }
                break;
                case TYPE_RENDER_TEXT:
                {
                    DrawTextEx(*ui_font, db_string_get_cstr(&frame_arena, &elems.data[i].label), position, 14, 2, color);
                }
                break;
                case TYPE_RENDER_CIRCLE:
                {
                    DrawCircleV(center, elems.data[i].radius, color);
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
            EndScissorMode();
        }
        EndDrawing();
        db_arena_reset(&frame_arena);
    }
    CloseWindow();
}
