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
    ui_elem box    = {};
    ui_elem parent = {.id = 0};
    if (db_stack_get_count(state->curr_parent))
    {
        parent = db_stack_peek(state->curr_parent);
    }

    box.id              = db_hash_string(label);
    ui_elem *prev_state = db_array_find(state->prev_elem_state, box, array_elem_compare);

    if (prev_state != NULL)
    {
        memcpy(&box, prev_state, sizeof(ui_elem));
    }
    else
    {
        u64 parent_index = 0;
        if (type != TYPE_WINDOW)
        {
            parent_index = parent.index;
        }
        box.parent_index = parent_index;
        box.type         = type;
        box.action_type  = action_type;
        box.size_type    = size_type;
        box.label        = label;
    }

    box.index = db_array_get_count(state->elements);

    if (type == TYPE_WINDOW)
    {
#if 1
        box.dimensions = (rectangle){50, 20};
        box.pos        = (vector3d){50, 50, 0};
#endif
        if (db_stack_get_count(state->curr_parent))
        {
            db_stack_clear(state->curr_parent);
        }
        db_stack_push(state->curr_parent, box);
    }
    // do this at the last dummy
    db_array_append(state->elements, box);
    return box.id;
}

db_return_code ui_init(vector2d (*measure_text_width)(const char *text, u32 font_size))
{
    db_arena arena            = db_arena_init();
    state                     = (ui_state *)db_arena_alloc(&arena, sizeof(ui_state));
    state->arena              = arena;
    state->measure_text_width = measure_text_width;
    db_stack_init(state->curr_parent);
    db_stack_init(state->layouts);
    db_array_init(state->elements);
    db_array_init(state->prev_elem_state);
    return DB_SUCCESS;
}

db_return_code ui_update_mouse_pos(vector2d mouse_pos)
{
    ASSERT_WITH_MSG(state != NULL, "ERROR: ui state is null.");

    state->mouse_pos = mouse_pos;
    return DB_SUCCESS;
}

b8 ui_window(const char *title)
{
    u64 id = ui_create_box(title, TYPE_WINDOW, TYPE_BASED_ON_CHILD, TYPE_ACTION_DRAGGABLE | TYPE_ACTION_RESIZABLE);

    return true;
}

b8 ui_button(const char *label)
{
    u64 id = ui_create_box(label, TYPE_BUTTON, TYPE_BASED_ON_TEXT_SIZE, TYPE_ACTION_PRESSABLE);

    return false;
}

// this is also the last call for the ui sys in the frame
db_array(ui_elem) ui_get_render_commands()
{
    state->mouse_pos = (vector2d){0, 0};
    db_stack_clear(state->layouts);
    db_stack_clear(state->curr_parent);
    // this will also clear the prev_elem_state
    db_array_copy(state->prev_elem_state, state->elements);
    db_array_clear(state->elements);
    return state->prev_elem_state;
}
