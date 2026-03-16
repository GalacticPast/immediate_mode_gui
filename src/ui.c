#include "ui.h"

static ui_state* state;

void ui_create_box(const char* label, ui_elem_type type, ui_elem_size_type size_type,ui_elem_action_type action_type)
{
    u64 id = dbh_hash_string(label);
    u32 parent_ind = 0;

    if(type != TYPE_WINDOW)
    {
        ui_elem parent = dbh_stack_peek(state->curr_parent);
        parent_ind = parent.id;
    }

    ui_elem elem = 
    {
        .parent_id = parent_ind,
    }; 
}

dbh_return_code ui_init(f32 screen_width , f32 screen_height)
{
    dbh_arena arena = dbh_arena_init();
    state = (ui_state *)dbh_arena_alloc(&arena,sizeof(ui_state));     
    state->arena = arena;
    dbh_stack_init(state->curr_parent);
    dbh_array_init(state->elements);
    dbh_array_init(state->prev_elem_state);
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
    ui_create_box(title, TYPE_WINDOW, TYPE_BASED_ON_CHILD, TYPE_ACTION_DRAGGABLE | TYPE_ACTION_RESIZABLE);
    
}

bool ui_button(const char *label);

dbh_array(ui_elem) ui_get_render_commands();
