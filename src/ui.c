#include "ui.h"

static ui_state *state;

void     __ui_calculate_element_sizes();
vector3d __ui_calculate_position(s32 index);

b8 array_elem_compare(ui_elem *find, ui_elem *with)
{
    ASSERT_WITH_MSG(find != NULL, "The elem that you want to find is NULL");
    ASSERT_WITH_MSG(with != NULL, "This is a bug, Idk how this happened. Array should never pass a null to the comp ");
    return find->id == with->id;
}

ui_elem *ui_create_box(const char         *label,
                       ui_elem_type        type,
                       ui_elem_size_type   size_type,
                       ui_elem_action_type action_type,
                       ui_axis_type        axis_type,
                       ui_axis_type        axis_child_type,
                       vector2d            position,
                       rectangle           dimensions)
{
    ui_elem box     = {};
    // see if there is a parent for this
    ui_elem *parent = NULL;

    if (db_stack_get_count(state->curr_parent))
    {
        parent = db_array_get_index_ptr(state->elements, db_stack_peek(state->curr_parent));
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
    box.id              = db_hash_string(box.label);
    // it doesnt matter if the parent.id is 0 because 0 is the sentinel node
    box.type            = type;
    box.action_type     = action_type;
    box.size_type       = size_type;
    box.index           = db_array_get_count(state->elements);
    box.position        = (vector3d){position.x, position.y, 0};
    box.dimensions      = dimensions;
    box.axis_type       = axis_type;
    box.axis_child_type = axis_child_type;
    box.child_count     = 0;

    if (parent != NULL)
    {
        box.parent_index = parent->index;
        if (parent->first_child_index == 0) // set the first parent child index
        {
            parent->first_child_index = box.index;
        }
        if (axis_type & TYPE_AXIS_BASED_ON_PARENT)
        {
            box.axis_type = parent->axis_child_type;
            ASSERT(!(parent->axis_child_type & TYPE_AXIS_NONE));
        }
    }

    // calculate min size
    if (!(type & TYPE_LAYOUT_NODE))
    {
        rectangle *dimen = &box.dimensions;
        dimen->width     = state->measure_text_width(box.label, state->font_size).width;
        dimen->height    = state->font_size;

        // FIXME: make padding dynamic
        dimen->width  += 10; // padding
        dimen->height += 10; // padding
    }

    ui_elem *prev_sibling = db_array_get_last_ptr(state->elements);

    if (type & TYPE_LAYOUT_NODE)
    {
        // if the layout node has the same parent as its prev siblings then just do the normal logic
        // if not
        if (prev_sibling->parent_index != box.parent_index)
        {
            // this is a terrible hack
            // what we are doing is talking advantage of the db_stack implementation
            // when we pop from the stack it doesnt zero the recent bucket, it just
            // decrements the array count by 1. So wehen we push another elem it will
            // ovveride that bucket. so before we ovveride we access that bucket and get
            // the most recent popped element. example:  ui tree is like this
            //
            /*             window               curr_parent stack
                           /    \                    row_0
                           row_0                     window
             *
             *
             * lets say we exit the scope of row_0 it will pop the row_o elem from the
             curr_parent stack

             curr_parent stack
             -> we popped it but the data is still there
             window
             *  now we want to push row_1 which is the next row for our window
             * now row_1 needs to know what's its previous sibling is.
             * now before we push it to the stack we get the popped elem's shadow data
             and we know whats the row_1's previous sibling.
             */
            s64 count    = db_stack_get_count(state->curr_parent);
            prev_sibling = &state->elements[state->curr_parent[count]];

            if (prev_sibling->parent_index)
            {
                ASSERT_WITH_MSG((prev_sibling->parent_index == box.parent_index),
                                "Well I was not clever as I was thinking I was :)");
            }
        }
    }

    //
    // IGNORE THE FOLLOWING COMMENT: ITS ALREADY FIXED // JUST PUTTINIG IT HERE FOR THE FUTURE
    // if the prev_sibling is the parent then this means this is the first child
    // of the parent and vice versa shit this doenst take into account if the
    // user puts a row or column layout in the middle like for example ui_row()
    // {
    //      button;
    //      button;
    //      butoon_n; --
    //      ui_col()   \
    //      {           \
    //      ...          -> the n+1 node's prev sibling should be button_n but I
    //      think we dont do that }           / button_n+1; ---
    // }
    // INFO:  wait why should the n+1 node be the button, it can be the ui row/column node

    if (prev_sibling->parent_index == box.parent_index)
    {
        prev_sibling->next_sibling_index = box.index;
        box.prev_sibling_index           = prev_sibling->index;
    }

    // do this at the last. Dummy
    if (type & TYPE_WINDOW || type & TYPE_LAYOUT_NODE)
    {
        db_stack_push(state->curr_parent, box.index);
    }
    db_array_append(state->elements, box);
    return db_array_get_last_ptr(state->elements);
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
    return DB_SUCCESS;
}

db_return_code ui_update_mouse_pos(vector2d mouse_pos)
{
    ASSERT_WITH_MSG(state != NULL, "ERROR: ui state is null.");

    state->mouse_pos = mouse_pos;
    return DB_SUCCESS;
}

b8 __ui_window_begin(const char *title, ui_window_desc *desc)
{
    ui_elem_action_type action_type = TYPE_ACTION_DRAGGABLE | TYPE_ACTION_RESIZABLE;
    ui_elem_size_type   size_type   = TYPE_BASED_ON_CHILD;
    vector2d            position    = {0};
    rectangle           dimensions  = {0};

    if (desc)
    {
        if (desc->position.x != 0 || desc->position.y != 0)
        {
            position = desc->position;
        }
        if (desc->dimensions.width != 0 || desc->dimensions.height != 0)
        {
            dimensions = desc->dimensions;
        }
        if (desc->action_type & TYPE_ACTION_NONE) // if its not none assign it
        {
            action_type = desc->action_type;
        }
        if (desc->size_type & TYPE_SIZE_NONE) // if its not none assign it
        {
            size_type = desc->size_type;
        }
    }

    ui_elem *elem = ui_create_box(title,
                                  TYPE_WINDOW,
                                  size_type,
                                  action_type,
                                  TYPE_AXIS_NONE,
                                  TYPE_AXIS_COLUMN,
                                  position,
                                  dimensions);

    elem->background_color = (color){255, 0, 0, 255};
    elem->text_color       = (color){255, 255, 255, 255};
    return true;
}

b8 __ui_window_end(void)
{
    db_stack_pop(state->curr_parent);
    return true;
}

b8 ui_button(const char *label)
{
    ui_elem *elem = ui_create_box(label,
                                  TYPE_BUTTON,
                                  TYPE_BASED_ON_TEXT_SIZE,
                                  TYPE_ACTION_PRESSABLE,
                                  TYPE_AXIS_BASED_ON_PARENT,
                                  TYPE_AXIS_NONE,
                                  (vector2d){0},
                                  (rectangle){0});

    elem->background_color = (color){0, 255, 0, 255};
    elem->text_color       = (color){255, 255, 255, 255};

    return true;
}

// private implementation

b8 __ui_row_begin(void)
{
    ui_create_box("",
                  TYPE_LAYOUT_NODE,
                  TYPE_BASED_ON_CHILD,
                  TYPE_ACTION_NONE,
                  TYPE_AXIS_BASED_ON_PARENT,
                  TYPE_AXIS_ROW,
                  (vector2d){0}, (rectangle){0});

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
    ui_create_box("",
                  TYPE_LAYOUT_NODE,
                  TYPE_BASED_ON_CHILD,
                  TYPE_ACTION_NONE,
                  TYPE_AXIS_BASED_ON_PARENT,
                  TYPE_AXIS_COLUMN,
                  (vector2d){0},
                  (rectangle){0});

    db_stack_push(state->curr_axis, TYPE_AXIS_COLUMN);
    return true; // macro hack
}

b8 __ui_column_end(void)
{
    db_stack_pop(state->curr_axis); // this will crash if the stack is empty
    db_stack_pop(state->curr_parent);
    return true; // macro hack
}

vector3d __ui_calculate_position(s32 index)
{
    if (index == 0)
    {
        return (vector3d){0};
    }

    static u32 padding_x = 5;
    static u32 padding_y = 5;

    ui_elem *first_child = db_array_get_index_ptr(state->elements, index);

    ui_elem *parent = db_array_get_index_ptr(state->elements, first_child->parent_index);

    vector3d cursor = {parent->position.x + padding_x, parent->position.y + padding_y, 0};

    if (parent->type & TYPE_WINDOW)
    {
        rectangle text_dimen = state->measure_text_width(parent->label, state->font_size);
        cursor               = (vector3d){parent->position.x, parent->position.y + text_dimen.height + padding_y, 0};
    }

    ui_elem *node = first_child;

    for (; node->index != 0;
         node = db_array_get_index_ptr(state->elements, node->next_sibling_index))
    {
        if (node->axis_type & TYPE_AXIS_ROW)
        {
            node->position  = cursor;
            cursor.x       += node->dimensions.width + padding_x;
        }
        else if (node->axis_type & TYPE_AXIS_COLUMN)
        {
            node->position  = cursor;
            cursor.y       += node->dimensions.height + padding_y;
        }

        __ui_calculate_position(node->first_child_index);
    }
    return cursor;
}

void __ui_calculate_element_sizes()
{
    // well what if the pane was part of a window and then the user dragged it
    // and then it became a window?
    static s32 padding_x = 5;
    static s32 padding_y = 5;

    u64 length = db_array_get_count(state->elements);

    ui_elem *elem         = &state->elements[0];
    ui_elem *parent       = &state->elements[0];
    db_array(ui_elem) arr = state->elements;

    // postorder traversal, traverse from the child first and then work up
    for (s64 i = length - 1; i > 0; i--)
    {
        elem   = &arr[i];
        parent = &arr[elem->parent_index];
        parent->child_count++;
        // wait am I doing the logic correctly ?
        if (!(elem->type & TYPE_WINDOW))
        {
            if (elem->type & TYPE_LAYOUT_NODE)
            {
                parent->dimensions.width   = db_max(elem->dimensions.width, parent->dimensions.width);
                parent->dimensions.height += elem->dimensions.height + padding_y;
            }
            else // then its a ui primitive
            {
                if (parent->first_child_index == elem->index)
                {
                    parent->dimensions.width  += padding_x; // kind of a hack, terrible for performance :)
                    parent->dimensions.height += padding_y; // kind of a hack, terrible for performance :)
                    // im doing this because we are not talking into account of the first initial padding
                    // for example
                    /*
                        if we dont add that additional padding at the start for the first child our layout would be like this
                        our first button would look like its squised to the right
                       +--------+-----------------------------+
                       |button |pad|button |pad| button |pad|
                       +--------+-----------------------------+
                        if we add it at first
                        +--------+---------------------------------+
                       ||pad| button |pad| button |pad| button |pad|
                       +--------+----------------------------------+
                     *
                     *
                     *
                     *
                     *  wait I think it's the opposite????
                    */
                }
                if (elem->axis_type & TYPE_AXIS_ROW)
                {
                    parent->dimensions.width  += elem->dimensions.width + padding_x;
                    parent->dimensions.height  = db_max(elem->dimensions.height + 2 * padding_y, parent->dimensions.height);
                }
                else
                {
                    parent->dimensions.width   = db_max(elem->dimensions.width + 2 * padding_x, parent->dimensions.width);
                    parent->dimensions.height += elem->dimensions.height + padding_y;
                }
            }
        }
    }
}

// this is also the last call for the ui sys in the frame
db_array(ui_elem) ui_get_render_commands()
{

    __ui_calculate_element_sizes(); // calculate the sizes

    // calculate relative positions
    ui_elem *elem = NULL;
    s32      i    = 0;
    db_array_for_each_ptr(state->elements, i, elem)
    {
        if (elem->type & TYPE_WINDOW)
        {
            __ui_calculate_position(i);
        }
    }

    state->mouse_pos = (vector2d){0, 0};
    db_stack_clear(state->curr_parent);
    // this will also clear the prev_elem_state
    elem = NULL;
    i    = 0;
    db_array_for_each_ptr(state->elements, i, elem)
    {
        db_array_append(state->prev_elem_state, *elem);
        if (!(elem->type & TYPE_LAYOUT_NODE)) // because every other elem will have a text that we need to render
        {
            ui_elem text    = {};
            text.label      = elem->label; // just copy the parent pointer
            text.position   = (vector3d){elem->position.x + 3, elem->position.y + 3, elem->position.z};
            text.type       = TYPE_TEXT;
            text.text_color = elem->text_color;
            db_array_append(state->prev_elem_state, text);
        }
    }

    db_array_clear(state->elements);

    db_array_append(state->elements, (ui_elem){0});

    return state->prev_elem_state;
}
