#include "ui.h"

static ui_state *state;

void __ui_calculate_element_sizes();

b8 array_elem_compare(ui_elem *find, ui_elem *with)
{
    ASSERT_WITH_MSG(find != NULL, "The elem that you want to find is NULL");
    ASSERT_WITH_MSG(with != NULL, "This is a bug, Idk how this happened. Array should never pass a null to the comp ");
    return find->id == with->id;
}

u64 ui_create_box(const char *label, ui_elem_type type, ui_elem_size_type size_type, ui_elem_action_type action_type)
{
    ui_elem box    = {};
    // see if there is a parent for this
    ui_elem parent = {.index = 0};

    if (db_stack_get_count(state->curr_parent))
    {
        parent = db_stack_peek(state->curr_parent);
    }

    // set some state for the box

    // this is kinda shit
    box.label     = db_string_make(label);                                                          // shit
    u64      id   = db_hash_string(box.label);                                                      // shit
    ui_elem *prev = db_array_find(state->prev_elem_state, (ui_elem){.id = id}, array_elem_compare); // shit
    if (prev)
    {
        db_string_free(box.label); // mega shit
        box.label = prev->label;   // shit
    }

    ASSERT(db_string_length(box.label) == (s64)strlen(label));
    box.id           = db_hash_string(box.label);
    // it doesnt matter if the parent.id is 0 because 0 is the sentinel node
    box.axis         = db_stack_peek(state->curr_axis);
    box.parent_index = parent.index;
    box.type         = type;
    box.action_type  = action_type;
    box.size_type    = size_type;
    box.index        = db_array_get_count(state->elements);

    // Explain this
    box.prev_elem_index = state->prev_elem_index;
    box.next_elem_index = 0;

    if (type == TYPE_WINDOW)
    {
        // if there is already another window then clear the stack for that window
        // because it means theat we are dont processing that window for that frame
        if (db_stack_get_count(state->curr_parent))
        {
            db_stack_clear(state->curr_parent);
        }
        db_stack_push(state->curr_parent, box);
    }
    // calculate min size
    rectangle *dimen = &box.dimensions;
    dimen->width     = state->measure_text_width(box.label, state->font_size).width;
    dimen->height    = state->font_size;

    // FIXME: make padding dynamic
    dimen->width  += 10; // padding
    dimen->height += 10; // padding

    //
    // do this at the last. Dummy
    db_array_append(state->elements, box);
    return box.id;
}

db_return_code ui_init(rectangle (*measure_text_width)(const char *text, u32 font_size))
{
    db_arena arena            = db_arena_init();
    state                     = (ui_state *)db_arena_alloc(&arena, sizeof(ui_state));
    state->prev_elem_index    = 0;
    state->arena              = arena;
    state->font_size          = 12; // for now
    state->measure_text_width = measure_text_width;

    db_stack_init(state->curr_parent);

    db_stack_init(state->curr_axis);
    db_stack_push(state->curr_axis, TYPE_AXIS_COLUMN); // default will be column

    db_array_init(state->elements);
    db_array_init(state->prev_elem_state);

    // sentinel node
    db_array_append(state->elements, (ui_elem){0});
    db_array_append(state->prev_elem_state, (ui_elem){0});
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

    return true;
}

// this is also the last call for the ui sys in the frame
db_array(ui_elem) ui_get_render_commands()
{
    __ui_calculate_element_sizes(); // calculate the sizes

    state->mouse_pos = (vector2d){0, 0};
    db_stack_clear(state->curr_parent);
    // this will also clear the prev_elem_state
    db_array_copy(state->prev_elem_state, state->elements);
    db_array_clear(state->elements);

    db_array_append(state->elements, (ui_elem){});

    return state->prev_elem_state;
}

// private implementation
void __append_dummy_layout_elem()
{
    // row dummy elem
    ui_elem dummy_elem       = {};
    u64     dummy_elem_index = db_array_get_count(state->elements);
    dummy_elem.index         = dummy_elem_index;
    dummy_elem.parent_index  = (db_stack_peek_ptr(state->curr_parent))->index;

    db_array_append(state->elements, dummy_elem);
    db_stack_push(state->curr_parent, dummy_elem);
}

b8 __ui_row_begin(void)
{
    __append_dummy_layout_elem();
    db_stack_push(state->curr_axis, TYPE_AXIS_ROW);
    return true; // macro hack
}

b8 __ui_row_end(void)
{
    db_stack_pop(state->curr_axis); // this will crash if the stack is empty
    db_stack_pop(state->curr_parent);
    return true; // macro hack
}

b8 __ui_column_begin(void)
{
    __append_dummy_layout_elem();
    db_stack_push(state->curr_axis, TYPE_AXIS_COLUMN);
    return true; // macro hack
}

b8 __ui_column_end(void)
{
    db_stack_pop(state->curr_axis); // this will crash if the stack is empty
    db_stack_pop(state->curr_parent);
    return true; // macro hack
}

rectangle __ui_elem_size_bfs(s32 index)
{
    ui_elem  *elem       = db_array_get_index_ptr(state->elements, index);
    rectangle dimensions = {0, 0};
    ui_elem  *next       = db_array_get_index_ptr(state->elements, elem->child_index);

    while (next->index != 0)
    {
        if (next->size_type == TYPE_BASED_ON_CHILD)
        {
            rectangle dime     = __ui_elem_size_bfs(next->index);
            dimensions.width  += dime.width;
            dimensions.height += dime.height;
        }
        else
        {
            dimensions.width  += next->dimensions.width;
            dimensions.height += next->dimensions.height;
        }
        next = db_array_get_index_ptr(state->elements, next->next_elem_index);
    }

    return dimensions;
}

void __ui_calculate_element_sizes()
{
    // well what if the pane was part of a window and then the user dragged it and then it became a window?
    u64       length     = db_array_get_count(state->elements);
    rectangle dimensions = {};
    // for (u64 i = 0; i < length; i++)
    //{
    //     ui_elem *elem = &state->elements[i];
    //     if (elem->type == TYPE_WINDOW)
    //     {
    //         dimensions               = __ui_elem_size_bfs(i);
    //         elem->dimensions.width  += dimensions.width;
    //         elem->dimensions.height += dimensions.height;
    //     }
    // }
    //

    ui_elem *elem         = &state->elements[0];
    ui_elem *parent       = &state->elements[0];
    db_array(ui_elem) arr = state->elements;

    for (s64 i = length - 1; i >= 0; i--)
    {
        elem   = &arr[i];
        parent = &arr[elem->parent_index];

        // wait am I doing the logic correctly ?
        if (!(elem->type & TYPE_WINDOW))
        {
            if (elem->axis == TYPE_AXIS_ROW)
            {
                parent->dimensions.width  += elem->dimensions.width;
                parent->dimensions.height  = max(elem->dimensions.height, parent->dimensions.height);
            }
            else
            {
                parent->dimensions.width   = max(elem->dimensions.width, parent->dimensions.width);
                parent->dimensions.height += elem->dimensions.height;
            }
        }
    }
}
