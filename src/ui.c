#include "ui.h"

static ui_state *state;

void     __ui_calculate_element_sizes();
void     __ui_calculate_position(s32 index);
void     __ui_resize_n_reposition_elements(s32 index);
b8       __ui_check_mouse_hover(vector3d position, rectangle dimensions);
ui_elem *__ui_get_prev_sibling(s32 parent_index);
ui_elem *__ui_create_box(const char                    *label,
                         ui_elem_type                   type,
                         ui_elem_size_type              size_type,
                         ui_elem_children_position_type children_pos_type,
                         ui_elem_action_type            action_type,
                         ui_axis_type                   axis_type,
                         ui_axis_type                   axis_child_type,
                         ui_render_type                 render_command_type,
                         vector2d                       position,
                         rectangle                      dimensions);

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
    db_array_init(state->windows);
    db_array_init(state->prev_elem_state);
    db_array_init(state->render_elements);

    // sentinel node
    db_array_append(state->elements, (ui_elem){0});

    state->curr_top_parent = &state->elements[0];

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
    }

    char str[16];
    snprintf(str, 16, "window##%d", state->window_counter);
    ui_elem *elem = __ui_create_box(str,
                                    TYPE_WINDOW,
                                    size_type,
                                    TYPE_POS_NONE, //@note: for now
                                    action_type,
                                    TYPE_AXIS_NONE,
                                    TYPE_AXIS_COLUMN,
                                    TYPE_RENDER_RECTANGLE,
                                    position,
                                    dimensions);
    state->window_counter++;

    ui_elem *title_box = __ui_create_box(
        title,
        TYPE_TREE,
        TYPE_SIZE_BASED_ON_PARENT,
        TYPE_POS_PLACE_CHILDREN_AT_CENTER, // for the text
        TYPE_ACTION_ANCHORED_TO_PARENT | TYPE_ACTION_DRAGGABLE,
        TYPE_AXIS_BASED_ON_PARENT,
        TYPE_AXIS_COLUMN,
        TYPE_RENDER_RECTANGLE | TYPE_RENDER_TEXT,
        (vector2d){0},
        (rectangle){0});

    elem->background_color = background_color;
    elem->text_color       = (color){255, 255, 255, 255};
    return true;
}

b8 __ui_window_end(void)
{
    ui_row(0)
    {
        ui_elem *box = __ui_create_box("",
                                       TYPE_BUTTON,
                                       TYPE_SIZE_FIXED,
                                       TYPE_POS_NONE,
                                       TYPE_ACTION_PRESSABLE | TYPE_ACTION_DRAGGABLE,
                                       TYPE_AXIS_BASED_ON_PARENT,
                                       TYPE_AXIS_NONE,
                                       TYPE_RENDER_RECTANGLE,
                                       (vector2d){0},
                                       (rectangle){25, 25});
    }
    db_stack_pop(state->curr_parent);
    return true;
}

b8 ui_button(const char *label)
{
    ui_elem *elem = __ui_create_box(label,
                                    TYPE_BUTTON,
                                    TYPE_SIZE_BASED_ON_TEXT | TYPE_SIZE_FLEX_GROW | TYPE_SIZE_FLEX_SHRINK,
                                    TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                    TYPE_ACTION_PRESSABLE,
                                    TYPE_AXIS_BASED_ON_PARENT,
                                    TYPE_AXIS_NONE,
                                    TYPE_RENDER_RECTANGLE | TYPE_RENDER_TEXT,
                                    (vector2d){0},
                                    (rectangle){0});

    return elem->is_active;
}
b8 ui_label(const char *label)
{
    ui_elem *elem = __ui_create_box(label,
                                    TYPE_LABEL,
                                    TYPE_SIZE_BASED_ON_TEXT,
                                    TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                    TYPE_ACTION_PRESSABLE,
                                    TYPE_AXIS_BASED_ON_PARENT,
                                    TYPE_AXIS_NONE,
                                    TYPE_RENDER_TEXT,
                                    (vector2d){0},
                                    (rectangle){0});
    return false; // label cannot be active or hot
}

void ui_checkbox(const char *label, b8 *boolean)
{
    static rectangle min_checkbox_dimen = (rectangle){20, 20};
    ui_row(0)
    {
        ui_elem *checkbox = __ui_create_box(label,
                                            TYPE_CHECKBOX,
                                            TYPE_SIZE_FIXED,
                                            TYPE_POS_NONE,
                                            TYPE_ACTION_PRESSABLE,
                                            TYPE_AXIS_BASED_ON_PARENT,
                                            TYPE_AXIS_NONE,
                                            TYPE_RENDER_RECTANGLE,
                                            (vector2d){0},
                                            min_checkbox_dimen);

        ui_elem *checkbox_label = __ui_create_box(label,
                                                  TYPE_LABEL,
                                                  TYPE_SIZE_BASED_ON_TEXT,
                                                  TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                                  TYPE_ACTION_NONE,
                                                  TYPE_AXIS_BASED_ON_PARENT,
                                                  TYPE_AXIS_NONE,
                                                  TYPE_RENDER_TEXT,
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
    ui_row(0)
    {
        ui_elem *circle = __ui_create_box(label,
                                          TYPE_RADIO_BUTTON,
                                          TYPE_SIZE_FIXED,
                                          TYPE_POS_NONE,
                                          TYPE_ACTION_PRESSABLE,
                                          TYPE_AXIS_BASED_ON_PARENT,
                                          TYPE_AXIS_NONE,
                                          TYPE_RENDER_CIRCLE,
                                          (vector2d){0},
                                          (rectangle){20, 20}); // @info: this is actually the radius and the dimensions of the square

        circle->radio_button_id = id;
        circle->choice          = choice;

        if (circle->is_active)
        {
            *circle->choice = *circle->choice == id ? -1 : id; // if it was already set set it to null
        }

        ui_elem *text = __ui_create_box(label,
                                        TYPE_LABEL,
                                        TYPE_SIZE_BASED_ON_TEXT,
                                        TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                        TYPE_ACTION_NONE,
                                        TYPE_AXIS_BASED_ON_PARENT,
                                        TYPE_AXIS_NONE,
                                        TYPE_RENDER_TEXT,
                                        (vector2d){0},
                                        (rectangle){0});
    }
}

void __ui_slider(const char *label, ui_slider_desc *desc)
{
    ui_row(0)
    {
        ui_elem *text = __ui_create_box(label,
                                        TYPE_LABEL,
                                        TYPE_SIZE_BASED_ON_TEXT,
                                        TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                        TYPE_ACTION_NONE,
                                        TYPE_AXIS_BASED_ON_PARENT,
                                        TYPE_AXIS_NONE,
                                        TYPE_RENDER_TEXT,
                                        (vector2d){0},
                                        (rectangle){0});

        if (desc->first_val)
        {
            db_string first_label = db_string_make(label);
            db_string_append(first_label, "##1");
            ui_elem *first_slider     = __ui_create_box(first_label,
                                                        TYPE_SLIDER,
                                                        TYPE_SIZE_MIN_DIMENSIONS | TYPE_SIZE_FLEX_GROW | TYPE_SIZE_FLEX_SHRINK,
                                                        TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                                        TYPE_ACTION_DRAGGABLE,
                                                        TYPE_AXIS_BASED_ON_PARENT,
                                                        TYPE_AXIS_NONE,
                                                        TYPE_RENDER_RECTANGLE,
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
                                                         TYPE_SIZE_MIN_DIMENSIONS | TYPE_SIZE_FLEX_GROW | TYPE_SIZE_FLEX_SHRINK,
                                                         TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                                         TYPE_ACTION_DRAGGABLE,
                                                         TYPE_AXIS_BASED_ON_PARENT,
                                                         TYPE_AXIS_NONE,
                                                         TYPE_RENDER_RECTANGLE,
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
                                                        TYPE_SIZE_MIN_DIMENSIONS | TYPE_SIZE_FLEX_GROW | TYPE_SIZE_FLEX_SHRINK,
                                                        TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                                        TYPE_ACTION_DRAGGABLE,
                                                        TYPE_AXIS_BASED_ON_PARENT,
                                                        TYPE_AXIS_NONE,
                                                        TYPE_RENDER_RECTANGLE,
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
                                                         TYPE_SIZE_MIN_DIMENSIONS | TYPE_SIZE_FLEX_GROW | TYPE_SIZE_FLEX_SHRINK,
                                                         TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                                         TYPE_ACTION_DRAGGABLE,
                                                         TYPE_AXIS_BASED_ON_PARENT,
                                                         TYPE_AXIS_NONE,
                                                         TYPE_RENDER_RECTANGLE,
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

b8 __ui_row_begin(ui_layout_desc *desc)
{
    ui_layout_desc l_desc = {.children_position_type = TYPE_POS_NONE,
                             .padding                = {5, 5}};
    if (desc)
    {
        l_desc.children_position_type = desc->children_position_type;
        l_desc.padding                = desc->padding;
    }
    ui_elem *row_box = __ui_create_box("",
                                       TYPE_LAYOUT_NODE,
                                       TYPE_SIZE_BASED_ON_CHILD,
                                       l_desc.children_position_type,
                                       TYPE_ACTION_NONE,
                                       TYPE_AXIS_BASED_ON_PARENT,
                                       TYPE_AXIS_ROW,
                                       TYPE_RENDER_NONE,
                                       (vector2d){0}, (rectangle){0});

    row_box->padding = l_desc.padding;

    db_stack_push(state->curr_axis, TYPE_AXIS_ROW);
    return true; // macro hack
}

b8 __ui_row_end(void)
{
    db_stack_pop(state->curr_axis); // this will crash if the stack is empty
    db_stack_pop(state->curr_parent);
    return true; // macro hack
}

b8 __ui_column_begin(ui_layout_desc *desc)
{
    ui_layout_desc l_desc = {.children_position_type = TYPE_POS_NONE,
                             .padding                = {5, 5}};
    if (desc)
    {
        l_desc.children_position_type = desc->children_position_type;
        l_desc.padding                = desc->padding;
    }
    ui_elem *c_box = __ui_create_box("",
                                     TYPE_LAYOUT_NODE,
                                     TYPE_SIZE_BASED_ON_CHILD,
                                     l_desc.children_position_type,
                                     TYPE_ACTION_NONE,
                                     TYPE_AXIS_BASED_ON_PARENT,
                                     TYPE_AXIS_COLUMN,
                                     TYPE_RENDER_NONE,
                                     (vector2d){0},
                                     (rectangle){0});

    c_box->padding = l_desc.padding;
    db_stack_push(state->curr_axis, TYPE_AXIS_COLUMN);
    return true; // macro hack
}

b8 __ui_column_end(void)
{
    db_stack_pop(state->curr_axis); // this will crash if the stack is empty
    db_stack_pop(state->curr_parent);
    return true; // macro hack
}

void ui_print_elements(db_array(ui_elem) array)
{
    ui_elem *elem = NULL;
    s32      i    = 0;
    db_array_for_each_ptr(array, i, elem)
    {
        printf("{\n index : %lld \n id : %llu \n label : %s \n }\n", elem->index, elem->id, elem->label);
    }
}

void __ui_sort_window_z_indexes()
{
    ui_elem *node = NULL;
    u32      i    = 0;

    db_array_for_each_ptr(state->elements, i, node)
    {
        if (node->type & TYPE_WINDOW)
        {
            db_array_append(state->windows, node);
        }
    }

    s32 count = db_array_get_count(state->windows);

    // bubble sort
    for (s32 j = 0; j < count; j++)
    {
        for (s32 i = 1; i < count; i++)
        {
            if (state->windows[i - 1]->position.z > state->windows[i]->position.z)
            {
                ui_elem *temp         = state->windows[i];
                state->windows[i]     = state->windows[i - 1];
                state->windows[i - 1] = temp;
            }
        }
    }
    // make sures that the window's z indexes are initialized correctly
    if (state->curr_top_parent->position.z != count - 1) // if the current top window is already the top one then dont do anything
    {
        state->curr_top_parent->position.z = count - 1;
        for (s32 j = 0; j < count; j++)
        {
            if (state->curr_top_parent != state->windows[j] && state->windows[j]->position.z != 0)
            {
                state->windows[j]->position.z -= 1;
            }
        }
    }
}

db_array(ui_render_element) ui_get_render_commands()
{
    db_array_clear(state->render_elements);

    __ui_calculate_element_sizes();

    __ui_sort_window_z_indexes();

    __ui_calculate_position(1); // because 0 is the null node

    __ui_resize_n_reposition_elements(1);

    ui_elem *window = NULL;
    s32      i      = 0;
    db_array_for_each(state->windows, i, window) // get the sorted window
    {
        ui_render_element w_node = {};
        w_node.position          = window->position;
        w_node.dimensions        = window->dimensions;
        w_node.color             = window->background_color;
        w_node.type              = TYPE_RENDER_RECTANGLE;
        db_array_append(state->render_elements, w_node);

        ui_elem *node = db_array_get_index_ptr(state->elements, window->first_child_index);

        for (s32 j = node->index; j < db_array_get_count(state->elements); j++)
        {
            node = db_array_get_index_ptr(state->elements, j);
            if (node->type & TYPE_WINDOW) // base case
            {
                break;
            }
            // debug specfic
#if 0 
            if(node->type & TYPE_LAYOUT_NODE)
            {
                ui_render_element elem = {};

                elem.dimensions = node->dimensions;
                elem.position   = node->position;
                elem.type       = TYPE_RENDER_RECTANGLE;
                elem.color      = (color){255,255,255,255};
                db_array_append(state->render_elements, elem);
            }
#endif
            // no matter the node type there will always be a box
            if (node->render_type & TYPE_RENDER_NONE)
                continue;

            if (node->render_type & TYPE_RENDER_RECTANGLE)
            {
                ui_render_element elem = {};

                elem.dimensions = node->dimensions;
                elem.position   = node->position;
                elem.type       = TYPE_RENDER_RECTANGLE;
                elem.color      = node->background_color;
                db_array_append(state->render_elements, elem);
            }
            if (node->render_type & TYPE_RENDER_TEXT)
            {
                ui_render_element elem = {};
                elem.dimensions        = node->text_dimensions;
                elem.position          = node->text_position;
                elem.type              = TYPE_RENDER_TEXT;
                elem.color             = node->text_color;
                elem.label             = node->label;
                db_array_append(state->render_elements, elem);
            }
            if (node->render_type & TYPE_RENDER_CIRCLE)
            {
                f32               half_x = node->position.x + (node->dimensions.width * 0.5);
                f32               half_y = node->position.y + (node->dimensions.height * 0.5);
                ui_render_element elem   = {};

                elem.center = (vector3d){half_x, half_y, node->position.z};
                elem.radius = node->dimensions.width * 0.5;
                elem.type   = TYPE_RENDER_CIRCLE;
                elem.color  = node->background_color;
                db_array_append(state->render_elements, elem);
            }
            // ui_elem->type specific render commands.
            //@note: we create these at the end, should we doing this here?
            // or the ui_create_box should take care of this... to be decided
            if (node->type & TYPE_CHECKBOX)
            {
                ASSERT(node->checkbox_val != NULL);
                if (*node->checkbox_val)
                {
                    // @refactor: maybe we should do this upstream? Maybe as soon in the ui_create_box function?
                    f32 two_thirds_y = node->position.y + (node->dimensions.height * 0.666666);
                    f32 half_x       = node->position.x + (node->dimensions.width * 0.5);
                    f32 scale        = 6.5;

                    node->check_common_start = (vector2d){half_x, two_thirds_y};

                    node->check_second_half_end = (vector2d){1.35, -1.28};   // -------->   thanks  desmos
                    node->check_first_half_end  = (vector2d){-1.023, -0.71}; // -- /

                    node->check_first_half_end.x  *= scale;
                    node->check_first_half_end.y  *= scale;
                    node->check_second_half_end.x *= scale;
                    node->check_second_half_end.y *= scale;

                    node->check_first_half_end.x += half_x;
                    node->check_first_half_end.y += two_thirds_y;

                    node->check_second_half_end.x += half_x;
                    node->check_second_half_end.y += two_thirds_y;

                    ui_render_element start = {};
                    start.type              = TYPE_RENDER_LINE;
                    start.start_pos         = (vector3d){half_x, two_thirds_y, node->position.z};
                    start.end_pos           = (vector3d){node->check_first_half_end.x, node->check_first_half_end.y, node->position.z};
                    start.color             = node->text_color; //@fix: make it dynamic
                    db_array_append(state->render_elements, start);

                    ui_render_element second = {};
                    second.type              = TYPE_RENDER_LINE;
                    second.start_pos         = (vector3d){half_x, two_thirds_y, node->position.z};
                    second.end_pos           = (vector3d){node->check_second_half_end.x, node->check_second_half_end.y, node->position.z};
                    second.color             = node->text_color; //@fix: make it dynamic
                    db_array_append(state->render_elements, second);
                }
            }
            else if (node->type & TYPE_SLIDER)
            {
                //@warn: should we be doing this here?
                vector3d s_pos = node->position;
                if (node->is_active)
                {
                    f32 point_in_box = state->mouse_pos.x - node->position.x;
                    point_in_box     = db_clamp_integer(0, point_in_box, node->dimensions.width);
                    f32 rel_val      = point_in_box * node->slider_max * (1 / node->dimensions.width);
                    ASSERT(rel_val <= node->slider_max);
                    *node->val_ptrs[node->val_ind] = rel_val;
                }

                s_pos.x  = *node->val_ptrs[node->val_ind] * (1 / node->slider_max) * node->dimensions.width;
                s_pos.x += node->position.x - 8;
                s_pos.x  = db_clamp_integer(node->position.x, s_pos.x, node->position.x + node->dimensions.width);

                s_pos.y = node->position.y;

                char buffer[16];
                snprintf(buffer, 16, "%2.f", *node->val_ptrs[node->val_ind]);

                ui_render_element slider_val = {};
                db_string         txt        = db_string_make(buffer); // @FIX: we are actually leaking mem here
                slider_val.label             = txt;                    // just copy the parent pointer
                slider_val.position          = (vector3d){node->position.x + 3, node->position.y + 3, node->position.z};
                slider_val.type              = TYPE_RENDER_TEXT;
                slider_val.color             = node->text_color;
                db_array_append(state->render_elements, slider_val);

                ui_render_element slider_box = {};

                slider_box.position   = s_pos;
                slider_box.dimensions = (rectangle){8, node->dimensions.height};
                slider_box.type       = TYPE_RENDER_RECTANGLE;
                slider_box.color      = (color){52, 66, 56, 255}; // green
                db_array_append(state->render_elements, slider_box);
            }
            else if (node->type & TYPE_RADIO_BUTTON && node->radio_button_id == *node->choice)
            {
                ui_render_element shadow_button = {};
                f32               half_x        = node->position.x + (node->dimensions.width * 0.5);
                f32               half_y        = node->position.y + (node->dimensions.height * 0.5);

                shadow_button.type   = TYPE_RENDER_CIRCLE;
                shadow_button.center = (vector3d){half_x, half_y, node->position.z};

                shadow_button.dimensions.width = node->dimensions.width - 3;
                shadow_button.radius           = node->dimensions.width / 2;
                shadow_button.color            = (color){255, 0, 0, 255}; // red

                db_array_append(state->render_elements, shadow_button);
            }
        }
    }

    // ui_print_elements(state->prev_elem_state);
    db_array_clear(state->windows);

    db_array_copy(state->prev_elem_state, state->elements); // this will also clear the array
    db_array_clear(state->elements);
    db_stack_clear(state->curr_parent);

    db_array_append(state->elements, (ui_elem){0});

    state->window_counter = 0; // reset the counter

    return state->render_elements;
}

// private implementation
void __ui_resize_n_reposition_elements(s32 index)
{

    for (ui_elem *window = db_array_get_index_ptr(state->elements, index);
         window->index != 0;
         window = db_array_get_index_ptr(state->elements, window->next_sibling_index))
    {
        ASSERT(window->type & TYPE_WINDOW);

        s32 w_child_count = window->child_count;

        for (ui_elem *l_node = db_array_get_index_ptr(state->elements, window->first_child_index);
             w_child_count != 0 && l_node->index != 0;
             w_child_count--, l_node = db_array_get_index_ptr(state->elements, l_node->next_sibling_index))
        {
            b8                children_have_flex_property = false;
            db_array(ui_elem) elements                    = NULL;
            db_array_init_arena(elements, &state->arena);
        }
    }
}

void __ui_calculate_position(s32 index)
{
    if (index == 0)
    {
        return;
    }

    static u32 padding_x = 2;
    static u32 padding_y = 2;

    ui_elem *first_child = db_array_get_index_ptr(state->elements, index);

    ui_elem *parent = db_array_get_index_ptr(state->elements, first_child->parent_index);

    vector3d cursor = {parent->position.x + padding_x, parent->position.y + padding_y, parent->position.z};

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

                cursor = parent->position;
            }
        }
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

        if (node->render_type & TYPE_RENDER_TEXT)
        {
            rectangle *n_dimen    = &node->dimensions;
            vector3d  *n_pos      = &node->position;
            rectangle *text_dimen = &node->text_dimensions;
            vector2d   center     = {(n_dimen->width * 0.5) + n_pos->x,
                                     (n_dimen->height * 0.5) + n_pos->y};
            switch (node->children_pos_type)
            {
                case TYPE_POS_PLACE_CHILDREN_AT_CENTER:
                    node->text_position.x = center.x - (text_dimen->width * 0.5);
                    node->text_position.y = center.y - (text_dimen->height * 0.5);
                    break;
                case TYPE_POS_PLACE_CHILDREN_AT_END:
                {
                }
                break;
                case TYPE_POS_PLACE_CHILDREN_AT_START:
                {
                }
                break;
                default:
                    break;
            }
        }

        __ui_calculate_position(node->first_child_index);
    }
    return;
}

void __ui_calculate_element_sizes()
{
    // well what if the pane was part of a window and then the user dragged it
    // and then it became a window?
    static s32 padding_x = 2;
    static s32 padding_y = 2;

    u64 length = db_array_get_count(state->elements);

    ui_elem          *elem   = &state->elements[0];
    ui_elem          *parent = &state->elements[0];
    db_array(ui_elem) arr    = state->elements;

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
            case TYPE_WINDOW:
            {
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
            case TYPE_LABEL:
            case TYPE_BUTTON:
            {
                if (elem->size_type & TYPE_SIZE_BASED_ON_TEXT)
                {
                    rectangle text_size = state->measure_text_size(elem->label, state->font_size * state->scale);
                    e_dim->width        = text_size.width + 2 * padding_x;
                    e_dim->height       = text_size.height + 2 * padding_y;

                    e_dim->width  *= state->scale;
                    e_dim->height *= state->scale;
                }
            }
            break;
            case TYPE_LAYOUT_NODE:
            {
                // initial padding
                e_dim->width  += padding_x;
                e_dim->height += padding_y;
            }
            break;
            case TYPE_CHECKBOX:
            case TYPE_RADIO_BUTTON:
            {
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
            if (parent->axis_child_type & TYPE_AXIS_ROW)
            {
                p_dim->width  += e_dim->width + padding_x;
                p_dim->height  = db_max(p_dim->height, e_dim->height + padding_y);
            }
            else if (parent->axis_child_type & TYPE_AXIS_COLUMN)
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

ui_elem *__ui_create_box(const char                    *label,
                         ui_elem_type                   type,
                         ui_elem_size_type              size_type,
                         ui_elem_children_position_type children_pos_type,
                         ui_elem_action_type            action_type,
                         ui_axis_type                   axis_type,
                         ui_axis_type                   axis_child_type,
                         ui_render_type                 render_command_type,
                         vector2d                       position,
                         rectangle                      dimensions)
{
    ui_elem box     = {};
    // see if there is a parent for this
    ui_elem *parent = &state->elements[0];

    if (db_stack_get_count(state->curr_parent))
    {
        parent = db_array_get_index_ptr(state->elements, db_stack_peek(state->curr_parent));
    }

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
    box.type              = type;
    box.action_type       = action_type;
    box.size_type         = size_type;
    // @fix: check if they are incompatible
    box.children_pos_type = children_pos_type;
    box.index             = db_array_get_count(state->elements);
    box.position          = (vector3d){position.x, position.y, 0};
    if (prev)
    {
        box.position = prev->position;
    }
    box.dimensions        = dimensions;
    box.axis_type         = axis_type;
    box.axis_child_type   = axis_child_type;
    box.child_count       = 0;
    box.parent_index      = parent->index;
    box.render_type       = render_command_type;
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
    if (render_command_type & TYPE_RENDER_TEXT)
    {
        box.text_dimensions = state->measure_text_size(box.label, state->font_size);
    }
    if (!(type & TYPE_LAYOUT_NODE) && !(type & TYPE_WINDOW))
    {
        box.text_color       = (color){255, 255, 255, 255};
        box.background_color = (color){90, 128, 133, 255};
    }
    // @refactor: pull this in a seprate func and explain the logic
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
            // find the parent_window
            ui_elem *parent_window = parent;
            while (!(parent_window->type & TYPE_WINDOW))
            {
                parent_window = db_array_get_index_ptr(state->elements, parent_window->parent_index);
            }
            state->curr_top_parent = parent_window;
        }
        else if (state->mouse_btn_state & TYPE_MOUSE_LEFT_BUTTON_RELEASED)
        {
            is_active             = false;
            state->active_elem_id = 0;
        }

        // if the user is still pressing the button then
        if (action_type & TYPE_ACTION_DRAGGABLE
            && state->mouse_btn_state & TYPE_MOUSE_LEFT_BUTTON_PRESSED
            && state->prev_mouse_btn_state & TYPE_MOUSE_LEFT_BUTTON_PRESSED
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
