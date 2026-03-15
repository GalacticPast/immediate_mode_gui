#include "ui.h"

static ui_state* state;

void ui_create_box(const char* label, ui_elem_type type, ui_elem_size_type size);

dbh_return_code ui_init(f32 screen_width , f32 screen_height)
{
    dbh_arena arena = dbh_arena_init();
    state = (ui_state *)dbh_arena_alloc(&arena,sizeof(ui_state));     
    state->arena = arena;

    return DBH_SUCCESS;
}

dbh_return_code ui_update_mouse_pos(vector2d mouse_pos)
{
    ASSERT_WITH_MSG(state != NULL, "ERROR: ui state is null.");
    
    state->mouse_pos = mouse_pos;
    return DBH_SUCCESS;
}

bool ui_window(const char *title)
{
    ui_create_box(title, TYPE_WINDOW, TYPE_GROWABLE_BASED_ON_CHILD);

}

bool ui_button(const char *label);

dbh_array(ui_elem) ui_get_render_commands();
