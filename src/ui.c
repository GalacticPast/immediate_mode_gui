#include "ui.h"

static ui_state *state;

void     __ui_calculate_element_sizes();
vector3d __ui_calculate_position(s32 index);
b8       __ui_check_mouse_hover(vector3d position, rectangle dimensions);
ui_elem *__ui_get_prev_sibling(s32 parent_index);
ui_elem *__ui_create_box(const char         *label,
                         ui_elem_type        type,
                         ui_elem_size_type   size_type,
                         ui_elem_action_type action_type,
                         ui_axis_type        axis_type,
                         ui_axis_type        axis_child_type,
                         vector2d            position,
                         rectangle           dimensions);

db_return_code ui_update_mouse(vector2d mouse_pos, ui_mouse_button_state mouse_button_state)
{
    state->prev_mouse_pos       = state->mouse_pos;
    state->prev_mouse_btn_state = state->mouse_btn_state;

    state->mouse_btn_state = mouse_button_state;
    state->mouse_pos       = mouse_pos;

    return DB_SUCCESS;
}

db_return_code ui_init(rectangle (*measure_text_size)(const char *text, u32 font_size))
{
    db_arena arena           = db_arena_init();
    state                    = (ui_state *)db_arena_alloc(&arena, sizeof(ui_state));
    state->arena             = arena;
    state->font_size         = 12; // for now
    state->measure_text_size = measure_text_size;
    state->scale             = 1.2;
    state->window_counter    = 0;

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
    ui_elem_action_type action_type      = TYPE_ACTION_NONE;
    ui_elem_size_type   size_type        = TYPE_SIZE_BASED_ON_CHILD;
    vector2d            position         = {0};
    rectangle           dimensions       = {0};
    color               background_color = (color){105, 80, 76, 255};

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
        background_color = desc->background_color;
    }

    char str[16];
    snprintf(str, 16, "window##%d", state->window_counter);
    ui_elem *elem = __ui_create_box(str,
                                    TYPE_WINDOW,
                                    size_type,
                                    action_type,
                                    TYPE_AXIS_NONE,
                                    TYPE_AXIS_ROW,
                                    position,
                                    dimensions);
    state->window_counter++;

    ui_elem *title_box = __ui_create_box(
        title,
        TYPE_TREE,
        TYPE_SIZE_BASED_ON_PARENT,
        TYPE_ACTION_ANCHORED_TO_PARENT | TYPE_ACTION_DRAGGABLE,
        TYPE_AXIS_BASED_ON_PARENT,
        TYPE_AXIS_COLUMN,
        (vector2d){0},
        (rectangle){0});

    elem->background_color = background_color;
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
    ui_elem *elem = __ui_create_box(label,
                                    TYPE_BUTTON,
                                    TYPE_SIZE_BASED_ON_TEXT,
                                    TYPE_ACTION_PRESSABLE,
                                    TYPE_AXIS_BASED_ON_PARENT,
                                    TYPE_AXIS_NONE,
                                    (vector2d){0},
                                    (rectangle){0});

    return elem->is_active;
}
b8 ui_label(const char *label)
{
    ui_elem *elem = __ui_create_box(label,
                                    TYPE_TEXT,
                                    TYPE_SIZE_BASED_ON_TEXT,
                                    TYPE_ACTION_PRESSABLE,
                                    TYPE_AXIS_BASED_ON_PARENT,
                                    TYPE_AXIS_NONE,
                                    (vector2d){0},
                                    (rectangle){0});
    return false; // label cannot be active or hot
}

void ui_checkbox(const char *label, b8 *boolean)
{
    static rectangle min_checkbox_dimen = (rectangle){20, 20};
    ui_column()
    {
        ui_elem *checkbox = __ui_create_box(label,
                                            TYPE_CHECKBOX,
                                            TYPE_SIZE_FIXED,
                                            TYPE_ACTION_PRESSABLE,
                                            TYPE_AXIS_BASED_ON_PARENT,
                                            TYPE_AXIS_NONE,
                                            (vector2d){0},
                                            min_checkbox_dimen);

        ui_elem *checkbox_label = __ui_create_box(label,
                                                  TYPE_TEXT,
                                                  TYPE_SIZE_BASED_ON_TEXT,
                                                  TYPE_ACTION_NONE,
                                                  TYPE_AXIS_BASED_ON_PARENT,
                                                  TYPE_AXIS_NONE,
                                                  (vector2d){0},
                                                  (rectangle){0});

        checkbox->checkbox_val = boolean;

        if (checkbox->is_active)
        {
            *boolean = !(*boolean);
        }
    }
}

void ui_radio_button(const char *label, s32 *choice, s32 id)
{
    ASSERT(choice != NULL);
    ui_column()
    {
        ui_elem *circle = __ui_create_box(label,
                                          TYPE_RADIO_BUTTON,
                                          TYPE_SIZE_FIXED,
                                          TYPE_ACTION_PRESSABLE,
                                          TYPE_AXIS_BASED_ON_PARENT,
                                          TYPE_AXIS_NONE,
                                          (vector2d){0},
                                          (rectangle){20, 20}); // @info: this is actually the radius and the dimensions of the square

        circle->radio_button_id = id;
        circle->choice          = choice;

        if (circle->is_active)
        {
            *circle->choice = *circle->choice == id ? -1 : id; // if it was already set set it to null
        }

        ui_elem *text = __ui_create_box(label,
                                        TYPE_TEXT,
                                        TYPE_SIZE_BASED_ON_TEXT,
                                        TYPE_ACTION_NONE,
                                        TYPE_AXIS_BASED_ON_PARENT,
                                        TYPE_AXIS_NONE,
                                        (vector2d){0},
                                        (rectangle){0});
    }
}

void __ui_slider(const char *label, ui_slider_desc *desc)
{
    ui_column()
    {
        ui_elem *text = __ui_create_box(label,
                                        TYPE_TEXT,
                                        TYPE_SIZE_BASED_ON_TEXT,
                                        TYPE_ACTION_NONE,
                                        TYPE_AXIS_BASED_ON_PARENT,
                                        TYPE_AXIS_NONE,
                                        (vector2d){0},
                                        (rectangle){0});

        if (desc->first_val)
        {
            db_string first_label = db_string_make(label);
            db_string_append(first_label, "##1");
            ui_elem *first_slider     = __ui_create_box(first_label,
                                                        TYPE_SLIDER,
                                                        TYPE_SIZE_FIXED,
                                                        TYPE_ACTION_DRAGGABLE,
                                                        TYPE_AXIS_BASED_ON_PARENT,
                                                        TYPE_AXIS_NONE,
                                                        (vector2d){0},
                                                        (rectangle){50, 25});
            // do the logic
            first_slider->slider_min  = desc->min;
            first_slider->slider_max  = desc->max;
            first_slider->val_ind     = 0;
            first_slider->val_ptrs[0] = desc->first_val;

            db_string_free(first_label);
        }
        if (desc->second_val)
        {
            db_string second_label = db_string_make(label);
            db_string_append(second_label, "##2");
            ui_elem *second_slider     = __ui_create_box(second_label,
                                                         TYPE_SLIDER,
                                                         TYPE_SIZE_FIXED,
                                                         TYPE_ACTION_DRAGGABLE,
                                                         TYPE_AXIS_BASED_ON_PARENT,
                                                         TYPE_AXIS_NONE,
                                                         (vector2d){0},
                                                         (rectangle){50, 25});
            // do the logic
            second_slider->slider_min  = desc->min;
            second_slider->slider_max  = desc->max;
            second_slider->val_ind     = 1;
            second_slider->val_ptrs[1] = desc->second_val;
            db_string_free(second_label);
        }
        if (desc->third_val)
        {
            db_string third_label = db_string_make(label);
            db_string_append(third_label, "##3");
            ui_elem *third_slider     = __ui_create_box(third_label,
                                                        TYPE_SLIDER,
                                                        TYPE_SIZE_FIXED,
                                                        TYPE_ACTION_DRAGGABLE,
                                                        TYPE_AXIS_BASED_ON_PARENT,
                                                        TYPE_AXIS_NONE,
                                                        (vector2d){0},
                                                        (rectangle){50, 25});
            // do the logic
            third_slider->slider_min  = desc->min;
            third_slider->slider_max  = desc->max;
            third_slider->val_ind     = 2;
            third_slider->val_ptrs[2] = desc->third_val;

            db_string_free(third_label);
        }
        if (desc->fourth_val)
        {
            // try
            db_string fourth_label = db_string_make(label);
            db_string_append(fourth_label, "##4");
            ui_elem *fourth_slider     = __ui_create_box(fourth_label,
                                                         TYPE_SLIDER,
                                                         TYPE_SIZE_FIXED,
                                                         TYPE_ACTION_DRAGGABLE,
                                                         TYPE_AXIS_BASED_ON_PARENT,
                                                         TYPE_AXIS_NONE,
                                                         (vector2d){0},
                                                         (rectangle){50, 25});
            // do the logic
            fourth_slider->slider_min  = desc->min;
            fourth_slider->slider_max  = desc->max;
            fourth_slider->val_ind     = 3;
            fourth_slider->val_ptrs[3] = desc->fourth_val;

            db_string_free(fourth_label);
        }
    }
}

b8 __ui_row_begin(void)
{
    __ui_create_box("",
                    TYPE_LAYOUT_NODE,
                    TYPE_SIZE_BASED_ON_CHILD,
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
    __ui_create_box("",
                    TYPE_LAYOUT_NODE,
                    TYPE_SIZE_BASED_ON_CHILD,
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

void ui_print_elements()
{
    ui_elem *elem = NULL;
    s32      i    = 0;
    db_array_for_each_ptr(state->elements, i, elem)
    {
        printf("{\n id : %lu \n label : %s \n }\n", elem->id, elem->label);
    }
}

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

    db_array_clear(state->prev_elem_state);
    db_stack_clear(state->curr_parent);
    // this will also clear the prev_elem_state
    elem = NULL;
    i    = 0;
    db_array_for_each_ptr(state->elements, i, elem)
    {
        if ((elem->type & TYPE_TEXT))
        {
            elem->position = (vector3d){elem->position.x + 3, elem->position.y + 3, elem->position.z};
        }
        if (elem->type & TYPE_CHECKBOX)
        {
            ASSERT(elem->checkbox_val != NULL);
            if (*elem->checkbox_val)
            {
                f32 two_thirds_y = elem->position.y + (elem->dimensions.height * 0.666666);
                f32 half_x       = elem->position.x + (elem->dimensions.width * 0.5);
                f32 scale        = 6.5;

                elem->check_common_start = (vector2d){half_x, two_thirds_y};

                elem->check_second_half_end = (vector2d){1.35, -1.28};   // -------->   thanks  desmos
                elem->check_first_half_end  = (vector2d){-1.023, -0.71}; // -- /

                elem->check_first_half_end.x  *= scale;
                elem->check_first_half_end.y  *= scale;
                elem->check_second_half_end.x *= scale;
                elem->check_second_half_end.y *= scale;

                elem->check_first_half_end.x += half_x;
                elem->check_first_half_end.y += two_thirds_y;

                elem->check_second_half_end.x += half_x;
                elem->check_second_half_end.y += two_thirds_y;
            }
        }
        if (elem->type & TYPE_RADIO_BUTTON)
        {
            f32 half_x = elem->position.x + (elem->dimensions.width * 0.5);
            f32 half_y = elem->position.y + (elem->dimensions.height * 0.5);

            elem->radio_button_center.x = half_x;
            elem->radio_button_center.y = half_y;
        }
        db_array_append(state->prev_elem_state, *elem);

        //  @refactor: this because every other elem will have a text that we need to render --> ?? with the helll
        if (!(elem->type & TYPE_LAYOUT_NODE)
            && !(elem->type & TYPE_TEXT)
            && !(elem->type & TYPE_CHECKBOX)
            && !(elem->type & TYPE_RADIO_BUTTON)
            && !(elem->type & TYPE_SLIDER)
            && !(elem->type & TYPE_WINDOW))
        {
            ui_elem text    = {};
            text.label      = elem->label; // just copy the parent pointer
            // @todo: fix the text centering
            text.position   = (vector3d){elem->position.x + 3, elem->position.y + 3, elem->position.z};
            text.type       = TYPE_TEXT;
            text.text_color = elem->text_color;
            db_array_append(state->prev_elem_state, text);
        }
        if (elem->type & TYPE_RADIO_BUTTON && elem->radio_button_id == *elem->choice)
        {
            ui_elem shadow_button = {};
            f32     half_x        = elem->position.x + (elem->dimensions.width * 0.5);
            f32     half_y        = elem->position.y + (elem->dimensions.height * 0.5);

            shadow_button.type                = elem->type;
            shadow_button.radio_button_center = elem->radio_button_center;

            shadow_button.dimensions.width  = elem->dimensions.width - 3;
            shadow_button.dimensions.height = elem->dimensions.width - 3;
            shadow_button.background_color  = (color){255, 0, 0, 255}; // red

            db_array_append(state->prev_elem_state, shadow_button);
        }
        if (elem->type & TYPE_SLIDER)
        {
            //@warn: should we be doing this here?
            vector3d s_pos = elem->position;
            if (elem->is_active)
            {
                f32 point_in_box = state->mouse_pos.x - elem->position.x;
                point_in_box     = db_clamp_integer(0, point_in_box, elem->dimensions.width);
                f32 rel_val      = point_in_box * elem->slider_max * (1 / elem->dimensions.width);
                ASSERT(rel_val <= elem->slider_max);
                *elem->val_ptrs[elem->val_ind] = rel_val;
            }

            s_pos.x  = *elem->val_ptrs[elem->val_ind] * (1 / elem->slider_max) * elem->dimensions.width;
            s_pos.x += elem->position.x - 8;
            s_pos.x  = db_clamp_integer(elem->position.x, s_pos.x, elem->position.x + elem->dimensions.width);

            s_pos.y = elem->position.y;

            char buffer[16];
            snprintf(buffer, 16, "%2.f", *elem->val_ptrs[elem->val_ind]);

            ui_elem   slider_val  = {};
            db_string txt         = db_string_make(buffer); // we are actually leaking mem here
            slider_val.label      = txt;                    // just copy the parent pointer
            slider_val.position   = (vector3d){elem->position.x + 3, elem->position.y + 3, elem->position.z};
            slider_val.type       = TYPE_TEXT;
            slider_val.text_color = elem->text_color;
            db_array_append(state->prev_elem_state, slider_val);

            ui_elem slider_box = {};

            slider_box.position   = s_pos;
            slider_box.dimensions = (rectangle){8, elem->dimensions.height};

            slider_box.background_color = (color){52, 66, 56, 255}; // green
            db_array_append(state->prev_elem_state, slider_box);
        }
    }

    // ui_print_elements();
    db_array_clear(state->elements);

    db_array_append(state->elements, (ui_elem){0});

    state->window_counter = 0; // reset the counter

    return state->prev_elem_state;
}

// private implementation

vector3d __ui_calculate_position(s32 index)
{
    if (index == 0)
    {
        return (vector3d){0};
    }

    static u32 padding_x = 2;
    static u32 padding_y = 2;

    ui_elem *first_child = db_array_get_index_ptr(state->elements, index);

    ui_elem *parent = db_array_get_index_ptr(state->elements, first_child->parent_index);

    vector3d cursor = {parent->position.x + padding_x, parent->position.y + padding_y, 0};

    // if (parent->type & TYPE_WINDOW)
    // {
    //     rectangle text_dimen = state->measure_text_size(parent->label, state->font_size * state->scale);
    //     cursor               = (vector3d){parent->position.x, parent->position.y + text_dimen.height + padding_y, 0};
    // }
    //
    ui_elem *node = first_child;

    for (; node->index != 0;
         node = db_array_get_index_ptr(state->elements, node->next_sibling_index))
    {
        if (node->action_type & TYPE_ACTION_ANCHORED_TO_PARENT)
        {
            if (node->is_active)
            {
                f32 delta_x         = state->mouse_pos.x - state->prev_mouse_pos.x;
                f32 delta_y         = state->mouse_pos.y - state->prev_mouse_pos.y;
                // @warn: clamp needed ??
                parent->position.x += delta_x;
                parent->position.y += delta_y;
            }
        }
        if (node->axis_type & TYPE_AXIS_COLUMN)
        {
            node->position  = cursor;
            cursor.x       += node->dimensions.width + padding_x;
        }
        else if (node->axis_type & TYPE_AXIS_ROW)
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
    static s32 padding_x = 2;
    static s32 padding_y = 2;

    u64 length = db_array_get_count(state->elements);

    ui_elem *elem         = &state->elements[0];
    ui_elem *parent       = &state->elements[0];
    db_array(ui_elem) arr = state->elements;

    // postorder traversal, traverse from the child first and then work up
    for (s64 i = length - 1; i > 0; i--)
    {
        elem   = &arr[i];
        parent = &arr[elem->parent_index];

        rectangle *e_dim = &elem->dimensions;
        rectangle *p_dim = &parent->dimensions;

        parent->child_count++;

        switch (elem->type)
        {
        case TYPE_WINDOW: {
            rectangle text_size  = state->measure_text_size(elem->label, state->font_size * state->scale);
            e_dim->width         = db_max(text_size.width + 2 * padding_x, e_dim->width);
            e_dim->height       += text_size.height + 2 * padding_y;

            // initial padding
            e_dim->width  += padding_x;
            e_dim->height += padding_y;
        }
        break;
        case TYPE_TREE: // @todo: we have to do another pass to account for any size changes after this point
        {
            rectangle text_size = state->measure_text_size(elem->label, state->font_size * state->scale);
            e_dim->width        = p_dim->width;
            e_dim->height       = text_size.height + 2 * padding_y;
        }
        break;
        case TYPE_TEXT:
        case TYPE_BUTTON: {
            rectangle text_size = state->measure_text_size(elem->label, state->font_size * state->scale);
            e_dim->width        = text_size.width + 2 * padding_x;
            e_dim->height       = text_size.height + 2 * padding_y;

            e_dim->width  *= state->scale;
            e_dim->height *= state->scale;
        }
        break;
        case TYPE_LAYOUT_NODE: {
            // initial padding
            e_dim->width  += padding_x;
            e_dim->height += padding_y;
        }
        break;
        case TYPE_CHECKBOX:
        case TYPE_RADIO_BUTTON: {
            e_dim->width  *= state->scale;
            e_dim->height *= state->scale;
        }
        break;
        case TYPE_SLIDER:
        case TYPE_TEXT_FIELD:
        case TYPE_SCROLL_BAR:
        case TYPE_NONE:
            break;
        }
        if (parent->index != 0)
        {
            if (parent->axis_child_type & TYPE_AXIS_COLUMN)
            {
                p_dim->width  += e_dim->width + padding_x;
                p_dim->height  = db_max(p_dim->height, e_dim->height + padding_y);
            }
            else if (parent->axis_child_type & TYPE_AXIS_ROW)
            {
                p_dim->width   = db_max(p_dim->width, e_dim->width + padding_x);
                p_dim->height += e_dim->height + padding_y;
            }
        }
    }
}

b8 __ui_check_mouse_hover(vector3d elem_position, rectangle elem_dimensions)
{
    vector2d *mouse_pos = &state->mouse_pos;

    b8 res = ((elem_position.y < mouse_pos->y)
              && (mouse_pos->y < elem_position.y + elem_dimensions.height)
              && (elem_position.x < mouse_pos->x)
              && (mouse_pos->x < elem_position.x + elem_dimensions.width));

    return res;
}

// this is also the last call for the ui sys in the frame

ui_elem *__ui_get_prev_sibling(s32 parent_index)
{
    ui_elem *parent = db_array_get_index_ptr(state->elements, parent_index);
    ASSERT(parent != NULL);

    ui_elem *child = db_array_get_index_ptr(state->elements, parent->first_child_index);
    for (; child->next_sibling_index != 0; child = db_array_get_index_ptr(state->elements, child->next_sibling_index))
    {
    }
    return child;
}

b8 array_elem_compare(ui_elem *find, ui_elem *with)
{
    ASSERT_WITH_MSG(find != NULL, "The elem that you want to find is NULL");
    ASSERT_WITH_MSG(with != NULL, "This is a bug, Idk how this happened. Array should never pass a null to the comp ");
    return find->id == with->id;
}

ui_elem *__ui_create_box(const char         *label,
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
    ui_elem *parent = &state->elements[0];

    if (db_stack_get_count(state->curr_parent))
    {
        parent = db_array_get_index_ptr(state->elements, db_stack_peek(state->curr_parent));
    }

    // set some state for the box

    // this is kinda shit
    box.label     = db_string_make(label);                                                                                 // shit
    u64 id        = db_murmur64A_seed(box.label, db_string_length(box.label), parent == NULL ? DB_HASH_SEED : parent->id); // shit
    box.id        = id;
    // this still deosnt take care of the fact if in the same layout row / column
    // we have two different ui elems with the same label
    // this will still generate the same id
    ui_elem *prev = db_array_find(state->prev_elem_state, box, array_elem_compare); // shit

    if (prev)
    {
        db_string_free(box.label); // mega shit
        box.label = prev->label;   // shit
    }

    ASSERT(db_string_length(box.label) == (s64)strlen(label));
    // it doesnt matter if the parent.id is 0 because 0 is the sentinel node
    box.type        = type;
    box.action_type = action_type;
    box.size_type   = size_type;
    box.index       = db_array_get_count(state->elements);
    box.position    = (vector3d){position.x, position.y, 0};
    if (prev)
    {
        box.position = prev->position;
    }
    box.dimensions        = dimensions;
    box.axis_type         = axis_type;
    box.axis_child_type   = axis_child_type;
    box.child_count       = 0;
    box.parent_index      = parent->index;
    ui_elem *prev_sibling = __ui_get_prev_sibling(parent->index);

    if (prev_sibling->parent_index == box.parent_index) // checking cause the prev sibling might be the sentinel node -> which means it didnt have any prev siblings
    {
        prev_sibling->next_sibling_index = box.index;
        box.prev_sibling_index           = prev_sibling->index;
    }

    if (parent->first_child_index == 0) // set the first parent child index
    {
        parent->first_child_index = box.index;
    }
    if (axis_type & TYPE_AXIS_BASED_ON_PARENT)
    {
        box.axis_type = parent->axis_child_type;
        ASSERT(!(parent->axis_child_type & TYPE_AXIS_NONE));
    }

    if (!(type & TYPE_LAYOUT_NODE) && !(type & TYPE_WINDOW))
    {
        box.text_color       = (color){255, 255, 255, 255};
        box.background_color = (color){90, 128, 133, 255};
    }

    b8 is_hot    = false;
    b8 is_active = false;
    if (prev && !(action_type == TYPE_ACTION_NONE))
    {
        is_hot = __ui_check_mouse_hover(prev->position, prev->dimensions);
        if (is_hot)
        {
            state->hot_elem_id = box.id;
        }
        else
        {
            state->hot_elem_id = 0;
        }

        if (is_hot && state->mouse_btn_state & TYPE_MOUSE_LEFT_BUTTON_PRESSED)
        {
            if (state->active_elem_id == 0)
            {
                is_active             = true;
                state->active_elem_id = box.id;
            }
        }
        else if (state->mouse_btn_state & TYPE_MOUSE_LEFT_BUTTON_RELEASED)
        {
            is_active             = false;
            state->active_elem_id = 0;
        }

        // if the user is still pressing the button then
        if (action_type & TYPE_ACTION_DRAGGABLE
            && state->mouse_btn_state & TYPE_MOUSE_LEFT_BUTTON_PRESSED
            && state->active_elem_id == box.id)
        {
            is_active             = true;
            state->active_elem_id = box.id;
        }

        if (is_hot & !(type & TYPE_LAYOUT_NODE))
        {
            box.background_color = (color){188, 188, 188, 255};
        }
    }

    box.is_hot    = is_hot;
    box.is_active = is_active;

    // do this at the last. Dummy
    if (type & TYPE_WINDOW || type & TYPE_LAYOUT_NODE)
    {
        db_stack_push(state->curr_parent, box.index);
    }

    db_array_append(state->elements, box);
    return db_array_get_last_ptr(state->elements);
}
