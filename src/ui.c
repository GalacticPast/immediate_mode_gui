#include "ui.h"

static ui_state *state;

b8 array_elem_compare(ui_elem *find, ui_elem *with)
{
    ASSERT_WITH_MSG(find != NULL, "The elem that you want to find is NULL");
    ASSERT_WITH_MSG(with != NULL, "This is a bug, Idk how this happened. Array should never pass a null to the comp ");
    return find->id == with->id;
}

u64 ui_create_box(const char *label, ui_elem_type type, ui_elem_size_type size_type, ui_elem_action_type action_type)
{
    ui_elem box         = {};
    box.id              = dbh_hash_string(label);
    ui_elem *prev_state = dbh_array_find(state->prev_elem_state, box, array_elem_compare);

    if (prev_state != NULL)
    {
        memcpy(&box, prev_state, sizeof(ui_elem));
    }
    else
    {
        u64 parent_index = 0;
        if (type != TYPE_WINDOW)
        {
            ui_elem parent = dbh_stack_peek(state->curr_parent);
            parent_index     = parent.elem_index;
        }
        box.parent_index = parent_index;
        box.type = type;
        box.action_type = action_type; 
        box.size_type = size_type; 
        box.label = label;

        if(type == TYPE_WINDOW)
        {
#if 1
            box.dimensions = (rectangle){50, 20};
            box.pos = (vector3d){50,50,0};
#endif
            dbh_stack_push(state->curr_parent, box);
        }
    }
    dbh_array_append(state->elements, box);

    return box.id;
}

dbh_return_code ui_init()
{
    dbh_arena arena = dbh_arena_init();
    state           = (ui_state *)dbh_arena_alloc(&arena, sizeof(ui_state));
    state->arena    = arena;
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

b8 ui_window(const char *title)
{
    u64 id = ui_create_box(title, TYPE_WINDOW, TYPE_BASED_ON_CHILD, TYPE_ACTION_DRAGGABLE | TYPE_ACTION_RESIZABLE);
    
    return true;
}

b8 ui_button(const char *label);

dbh_array(ui_elem) ui_get_render_commands()
{
    return state->elements;
}

