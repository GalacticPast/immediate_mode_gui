#include "ui.h"

static ui_state *state;

b8 array_elem_compare(ui_elem *find, ui_elem *with);

ui_elem *__ui_get_prev_sibling(s32 parent_index);
b8       __ui_check_mouse_hover(vector3d position, rectangle dimensions);
void     __ui_check_action(ui_elem *elem);
void     __ui_calculate_element_sizes();
void     __ui_calculate_position(s32 index);
void     __ui_resize_n_reposition_elements(s32 index);
void     __ui_set_clip_regions();

ui_elem *__ui_create_box(db_string           str,
                         ui_elem_type        type,
                         ui_elem_size_type   size_type,
                         ui_elem_pos_type    pos_type,
                         ui_elem_pos_type    children_pos_type,
                         ui_elem_action_type action_type,
                         ui_axis_type        axis_type,
                         ui_axis_type        axis_child_type,
                         ui_render_type      render_command_type,
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

db_return_code ui_init(u32 *required_size, void *memory, rectangle (*measure_text_size)(const char *text, u32 font_size))
{
    ASSERT_WITH_MSG(required_size, "UI Required size must be a valid u32 pointer");
    if (*required_size == 0)
    {
        *required_size = sizeof(ui_state);
        return DB_ERROR;
    }
    state = memory;

    state->ui_permanent_arena    = db_arena_init();
    state->ui_frame_arena        = db_arena_init();
    state->ui_string_chunk_arena = db_arena_init(.type = TYPE_ARENA_CHUNKED, .chunk_size = 64);

    state->font_size         = 14; // for now
    state->measure_text_size = measure_text_size;
    state->scale             = 1.2;
    state->window_counter    = 0;

    state->ui_elements          = db_array_ui_elements_init(&state->ui_frame_arena);
    state->previous_ui_elements = db_array_ui_elements_init(&state->ui_permanent_arena);
    state->windows              = db_array_ui_elem_ptr_init(&state->ui_frame_arena);
    state->render_elements      = db_array_render_elements_init(&state->ui_permanent_arena);

    state->curr_parent_index = db_stack_s32_init(&state->ui_frame_arena);
    state->curr_axis         = db_stack_s32_init(&state->ui_frame_arena);

    // sentinel node/ ancestor node
    db_array_ui_elements_append(&state->ui_elements, (ui_elem){0});
    db_array_ui_elements_append(&state->previous_ui_elements, (ui_elem){0});

    // default axis will be column
    db_stack_s32_push(&state->curr_axis, TYPE_AXIS_COLUMN);

    state->curr_top_parent = &state->ui_elements.data[0];

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
    db_string window_label = db_string_make(&state->ui_frame_arena, str);

    ui_elem *elem = __ui_create_box(window_label,
                                    TYPE_WINDOW, // can it be a floating container?
                                    size_type,
                                    TYPE_POS_NONE,
                                    TYPE_POS_NONE, //@note: for now
                                    action_type,
                                    TYPE_AXIS_NONE,
                                    TYPE_AXIS_COLUMN,
                                    TYPE_RENDER_RECTANGLE,
                                    position,
                                    dimensions);
    state->window_counter++;
    elem->padding = (vector2d){3, 3};

    ui_row(.children_position_type = TYPE_POS_PLACE_CHILDREN_AT_CENTER,
           .render_command_type    = TYPE_RENDER_RECTANGLE,
           .action_type            = TYPE_ACTION_HOVERABLE | TYPE_ACTION_DRAGGABLE
                                     | TYPE_ACTION_PRESSABLE | TYPE_ACTION_ANCHORED_TO_PARENT)
    {
        db_string title_str = db_string_make(&state->ui_frame_arena, title);
        ui_elem  *title_box = __ui_create_box(
            title_str,
            TYPE_LABEL,
            TYPE_SIZE_BASED_ON_TEXT,
            TYPE_POS_NONE,
            TYPE_POS_PLACE_CHILDREN_AT_CENTER, // for the text
            TYPE_ACTION_NONE,
            TYPE_AXIS_BASED_ON_PARENT,
            TYPE_AXIS_NONE,
            TYPE_RENDER_TEXT,
            (vector2d){0},
            (rectangle){0});
    }
    elem->background_color = background_color;
    elem->text_color       = (color){255, 255, 255, 255};
    return true;
}

b8 __ui_window_end(void)
{
    // get the window dimensions + position.
    ui_elem *window = &state->ui_elements.data[0]; // get the sentinel node
    if (state->windows.length)
    {
        window = db_array_ui_elem_ptr_get_last(&state->windows);
    }
    rectangle button_dimen = {5, 5};
    vector2d  position;
    position.x = window->position.x + window->dimensions.width - button_dimen.width;
    position.y = window->position.y + window->dimensions.height - button_dimen.height;

    db_string str               = db_string_make(&state->ui_frame_arena, "increase_size_box");
    ui_elem  *increase_size_box = __ui_create_box(str,
                                                  TYPE_BUTTON,
                                                  TYPE_SIZE_FIXED,
                                                  TYPE_POS_FIXED | TYPE_POS_PLACE_SELF_AT_END,
                                                  TYPE_POS_NONE,
                                                  TYPE_ACTION_PRESSABLE | TYPE_ACTION_HOVERABLE | TYPE_ACTION_DRAGGABLE | TYPE_ACTION_REFLECT_TO_PARENT,
                                                  TYPE_AXIS_BASED_ON_PARENT,
                                                  TYPE_AXIS_NONE,
                                                  TYPE_RENDER_RECTANGLE,
                                                  position,
                                                  (rectangle){25, 25});
    __ui_check_action(increase_size_box);
    db_stack_s32_pop(&state->curr_parent_index);
    return true;
}

b8 ui_button(const char *label)
{
    db_string str  = db_string_make(&state->ui_frame_arena, label);
    ui_elem  *elem = __ui_create_box(str,
                                     TYPE_BUTTON,
                                     TYPE_SIZE_BASED_ON_TEXT,
                                     TYPE_POS_NONE,
                                     TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                     TYPE_ACTION_HOVERABLE | TYPE_ACTION_PRESSABLE,
                                     TYPE_AXIS_BASED_ON_PARENT,
                                     TYPE_AXIS_NONE,
                                     TYPE_RENDER_RECTANGLE | TYPE_RENDER_TEXT,
                                     (vector2d){0},
                                     (rectangle){0});

    __ui_check_action(elem);
    elem->growth_factor = 1;

    return elem->is_active;
}
b8 ui_label(const char *label)
{
    db_string str  = db_string_make(&state->ui_frame_arena, label);
    ui_elem  *elem = __ui_create_box(str,
                                     TYPE_LABEL,
                                     TYPE_SIZE_BASED_ON_TEXT,
                                     TYPE_POS_NONE,
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
        db_string str      = db_string_make(&state->ui_frame_arena, label);
        ui_elem  *checkbox = __ui_create_box(str,
                                             TYPE_CHECKBOX,
                                             TYPE_SIZE_FIXED,
                                             TYPE_POS_NONE,
                                             TYPE_POS_NONE,
                                             TYPE_ACTION_HOVERABLE | TYPE_ACTION_PRESSABLE,
                                             TYPE_AXIS_BASED_ON_PARENT,
                                             TYPE_AXIS_NONE,
                                             TYPE_RENDER_RECTANGLE,
                                             (vector2d){0},
                                             min_checkbox_dimen);
        __ui_check_action(checkbox);

        ui_elem *checkbox_label = __ui_create_box(str,
                                                  TYPE_LABEL,
                                                  TYPE_SIZE_BASED_ON_TEXT,
                                                  TYPE_POS_NONE,
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
        db_string str    = db_string_make(&state->ui_frame_arena, label);
        ui_elem  *circle = __ui_create_box(str,
                                           TYPE_RADIO_BUTTON,
                                           TYPE_SIZE_FIXED,
                                           TYPE_POS_NONE,
                                           TYPE_POS_NONE,
                                           TYPE_ACTION_HOVERABLE | TYPE_ACTION_PRESSABLE,
                                           TYPE_AXIS_BASED_ON_PARENT,
                                           TYPE_AXIS_NONE,
                                           TYPE_RENDER_CIRCLE,
                                           (vector2d){0},
                                           (rectangle){20, 20}); // @info: this is actually the radius and the dimensions of the square

        __ui_check_action(circle);

        circle->radio_button_id = id;
        circle->choice          = choice;

        if (circle->is_active)
        {
            *circle->choice = *circle->choice == id ? -1 : id; // if it was already set set it to null
        }

        ui_elem *text = __ui_create_box(str,
                                        TYPE_LABEL,
                                        TYPE_SIZE_BASED_ON_TEXT,
                                        TYPE_POS_NONE,
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
        db_string str  = db_string_make(&state->ui_frame_arena, label);
        ui_elem  *text = __ui_create_box(str,
                                         TYPE_LABEL,
                                         TYPE_SIZE_BASED_ON_TEXT,
                                         TYPE_POS_NONE,
                                         TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                         TYPE_ACTION_NONE,
                                         TYPE_AXIS_BASED_ON_PARENT,
                                         TYPE_AXIS_NONE,
                                         TYPE_RENDER_TEXT,
                                         (vector2d){0},
                                         (rectangle){0});

        if (desc->first_val)
        {
            db_string first_label = db_string_make(&state->ui_frame_arena, label);
            db_string_append(first_label, "##1");
            ui_elem *first_slider = __ui_create_box(first_label,
                                                    TYPE_SLIDER,
                                                    TYPE_SIZE_BASED_ON_TEXT | TYPE_SIZE_FLEX_GROW | TYPE_SIZE_FLEX_SHRINK,
                                                    TYPE_POS_NONE,
                                                    TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                                    TYPE_ACTION_HOVERABLE | TYPE_ACTION_DRAGGABLE,
                                                    TYPE_AXIS_BASED_ON_PARENT,
                                                    TYPE_AXIS_NONE,
                                                    TYPE_RENDER_RECTANGLE,
                                                    (vector2d){0},
                                                    (rectangle){50, 25});
            __ui_check_action(first_slider);

            // do the logic
            first_slider->slider_min    = desc->min;
            first_slider->slider_max    = desc->max;
            first_slider->val_ind       = 0;
            first_slider->val_ptrs[0]   = desc->first_val;
            first_slider->growth_factor = 1;
        }
        if (desc->second_val)
        {
            db_string second_label = db_string_make(&state->ui_frame_arena, label);
            db_string_append(second_label, "##2");
            ui_elem *second_slider = __ui_create_box(second_label,
                                                     TYPE_SLIDER,
                                                     TYPE_SIZE_BASED_ON_TEXT | TYPE_SIZE_FLEX_GROW | TYPE_SIZE_FLEX_SHRINK,
                                                     TYPE_POS_NONE,
                                                     TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                                     TYPE_ACTION_HOVERABLE | TYPE_ACTION_DRAGGABLE,
                                                     TYPE_AXIS_BASED_ON_PARENT,
                                                     TYPE_AXIS_NONE,
                                                     TYPE_RENDER_RECTANGLE,
                                                     (vector2d){0},
                                                     (rectangle){50, 25});
            // do the logic
            __ui_check_action(second_slider);
            second_slider->slider_min    = desc->min;
            second_slider->slider_max    = desc->max;
            second_slider->val_ind       = 1;
            second_slider->val_ptrs[1]   = desc->second_val;
            second_slider->growth_factor = 1;
        }
        if (desc->third_val)
        {
            db_string third_label = db_string_make(&state->ui_frame_arena, label);
            db_string_append(third_label, "##3");
            ui_elem *third_slider = __ui_create_box(third_label,
                                                    TYPE_SLIDER,
                                                    TYPE_SIZE_BASED_ON_TEXT | TYPE_SIZE_FLEX_GROW | TYPE_SIZE_FLEX_SHRINK,
                                                    TYPE_POS_NONE,
                                                    TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                                    TYPE_ACTION_HOVERABLE | TYPE_ACTION_DRAGGABLE,
                                                    TYPE_AXIS_BASED_ON_PARENT,
                                                    TYPE_AXIS_NONE,
                                                    TYPE_RENDER_RECTANGLE,
                                                    (vector2d){0},
                                                    (rectangle){50, 25});
            // do the logic
            __ui_check_action(third_slider);
            third_slider->slider_min    = desc->min;
            third_slider->slider_max    = desc->max;
            third_slider->val_ind       = 2;
            third_slider->val_ptrs[2]   = desc->third_val;
            third_slider->growth_factor = 1;
        }
        if (desc->fourth_val)
        {
            // try
            db_string fourth_label = db_string_make(&state->ui_frame_arena, label);
            db_string_append(fourth_label, "##4");
            ui_elem *fourth_slider = __ui_create_box(fourth_label,
                                                     TYPE_SLIDER,
                                                     TYPE_SIZE_BASED_ON_TEXT | TYPE_SIZE_FLEX_GROW | TYPE_SIZE_FLEX_SHRINK,
                                                     TYPE_POS_NONE,
                                                     TYPE_POS_PLACE_CHILDREN_AT_CENTER,
                                                     TYPE_ACTION_HOVERABLE | TYPE_ACTION_DRAGGABLE,
                                                     TYPE_AXIS_BASED_ON_PARENT,
                                                     TYPE_AXIS_NONE,
                                                     TYPE_RENDER_RECTANGLE,
                                                     (vector2d){0},
                                                     (rectangle){50, 25});
            // do the logic
            __ui_check_action(fourth_slider);
            fourth_slider->slider_min    = desc->min;
            fourth_slider->slider_max    = desc->max;
            fourth_slider->val_ind       = 3;
            fourth_slider->val_ptrs[3]   = desc->fourth_val;
            fourth_slider->growth_factor = 1;
        }
    }
}

b8 __ui_row_begin(ui_layout_desc *desc)
{
    ui_layout_desc l_desc = {.children_position_type = TYPE_POS_NONE,
                             .render_command_type    = TYPE_RENDER_NONE,
                             .action_type            = TYPE_ACTION_NONE,
                             .padding                = {5, 5}};
    if (desc)
    {
        l_desc.children_position_type = desc->children_position_type;
        l_desc.render_command_type    = desc->render_command_type;
        l_desc.action_type            = desc->action_type;
        l_desc.padding                = desc->padding;
    }
    //@todo: check if this correct
    db_string str     = {};
    ui_elem  *row_box = __ui_create_box(str,
                                        TYPE_CONTAINER,
                                        TYPE_SIZE_FLEX_GROW | TYPE_SIZE_BASED_ON_CHILD,
                                        TYPE_POS_NONE,
                                        l_desc.children_position_type,
                                        l_desc.action_type,
                                        TYPE_AXIS_BASED_ON_PARENT,
                                        TYPE_AXIS_ROW,
                                        l_desc.render_command_type,
                                        (vector2d){0}, (rectangle){0});

    __ui_check_action(row_box);
    db_stack_s32_push(&state->curr_axis, TYPE_AXIS_ROW);
    return true; // macro hack
}

b8 __ui_row_end(void)
{
    db_stack_s32_pop(&state->curr_axis); // this will crash if the stack is empty
    db_stack_s32_pop(&state->curr_parent_index);
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
    db_string str   = {0};
    ui_elem  *c_box = __ui_create_box(str,
                                      TYPE_CONTAINER,
                                      TYPE_SIZE_FLEX_GROW | TYPE_SIZE_BASED_ON_CHILD,
                                      TYPE_POS_NONE,
                                      l_desc.children_position_type,
                                      TYPE_ACTION_NONE,
                                      TYPE_AXIS_BASED_ON_PARENT,
                                      TYPE_AXIS_COLUMN,
                                      TYPE_RENDER_NONE,
                                      (vector2d){0},
                                      (rectangle){0});

    __ui_check_action(c_box);
    c_box->padding = l_desc.padding;
    db_stack_s32_push(&state->curr_axis, TYPE_AXIS_COLUMN);
    return true; // macro hack
}

b8 __ui_column_end(void)
{
    db_stack_s32_pop(&state->curr_axis); // this will crash if the stack is empty
    db_stack_s32_pop(&state->curr_parent_index);
    return true; // macro hack
}

void __ui_sort_window_z_indexes()
{
    ui_elem *node = NULL;
    u32      i    = 0;

    s32 count = state->windows.length;

    // bubble sort
    for (s32 j = 0; j < count; j++)
    {
        for (s32 i = 1; i < count; i++)
        {
            if (state->windows.data[i - 1]->position.z > state->windows.data[i]->position.z)
            {
                ui_elem *temp              = state->windows.data[i];
                state->windows.data[i]     = state->windows.data[i - 1];
                state->windows.data[i - 1] = temp;
            }
        }
    }
    // make sures that the window's z indexes are initialized correctly
    if (state->curr_top_parent->position.z != count - 1) // if the current top window is already the top one then dont do anything
    {
        state->curr_top_parent->position.z = count - 1;
        for (s32 j = 0; j < count; j++)
        {
            if (state->curr_top_parent != state->windows.data[j] && state->windows.data[j]->position.z != 0)
            {
                state->windows.data[j]->position.z -= 1;
            }
        }
    }
}

// hmmmmm
db_array_render_elements ui_get_render_commands()
{
    db_array_render_elements_clear(&state->render_elements);

    __ui_calculate_element_sizes();

    __ui_sort_window_z_indexes();

    __ui_calculate_position(1); // because 0 is the null node

    __ui_resize_n_reposition_elements(1);

    __ui_set_clip_regions();

    // generate the render commands
    ui_elem *window = NULL;
    s32      i      = 0;
    db_array_for_each(state->windows, i, window) // get the sorted window
    {
        ui_render_element w_node = {};
        w_node.position          = window->position;
        w_node.dimensions        = window->dimensions;
        w_node.color             = window->background_color;
        w_node.type              = TYPE_RENDER_RECTANGLE;
        w_node.clip_position     = window->clip_position;
        w_node.clip_dimensions   = window->clip_dimensions;
        db_array_render_elements_append(&state->render_elements, w_node);

        ui_elem *node = db_array_ui_elements_get_index_ptr(&state->ui_elements, window->first_child_index);

        for (s32 j = node->index; j < state->ui_elements.length; j++)
        {
            node = db_array_ui_elements_get_index_ptr(&state->ui_elements, j);
            if (node->type & TYPE_WINDOW) // base case
            {
                break;
            }
            // debug specfic
#if 0
            if (node->type & TYPE_CONTAINER)
            {
                ui_render_element elem = {};

                elem.dimensions = node->dimensions;
                elem.position   = node->position;
                elem.type       = TYPE_RENDER_RECTANGLE;
                elem.color      = (color){255, 255, 255, 255};
                db_array_append(state->render_elements, elem);
            }
#endif
            // no matter the node type there will always be a box

            if (node->render_type & TYPE_RENDER_RECTANGLE)
            {
                ui_render_element border = {};

                border.dimensions      = (rectangle){node->dimensions.width + 4, node->dimensions.height + 4};
                border.position        = (vector3d){node->position.x - 2, node->position.y - 2, node->position.z};
                border.type            = TYPE_RENDER_RECTANGLE;
                border.clip_dimensions = node->clip_dimensions;
                border.clip_position   = node->clip_position;
                border.color           = (color){0, 0, 0, 255};
                db_array_render_elements_append(&state->render_elements, border);

                ui_render_element elem = {};

                elem.position        = node->position;
                elem.dimensions      = node->dimensions;
                elem.clip_dimensions = node->clip_dimensions;
                elem.clip_position   = node->clip_position;
                elem.type            = TYPE_RENDER_RECTANGLE;
                elem.color           = node->background_color;
                db_array_render_elements_append(&state->render_elements, elem);
            }
            if (node->render_type & TYPE_RENDER_TEXT)
            {
                ui_render_element elem = {};
                elem.dimensions        = node->text_dimensions;
                elem.position          = node->text_position;
                elem.clip_dimensions   = node->clip_dimensions;
                elem.clip_position     = node->clip_position;
                elem.type              = TYPE_RENDER_TEXT;
                elem.color             = node->text_color;
                elem.label             = node->label;
                db_array_render_elements_append(&state->render_elements, elem);

#if 0 // debug specific
                ui_render_element d_elem = {};

                d_elem.dimensions = node->dimensions;
                d_elem.position   = node->position;
                d_elem.type       = TYPE_RENDER_RECTANGLE;
                d_elem.color      = (color){255, 255, 255, 100};
                db_array_append(state->render_elements, d_elem);
#endif
            }
            if (node->render_type & TYPE_RENDER_CIRCLE)
            {
                f32               half_x = node->position.x + (node->dimensions.width * 0.5);
                f32               half_y = node->position.y + (node->dimensions.height * 0.5);
                ui_render_element elem   = {};

                elem.center          = (vector3d){half_x, half_y, node->position.z};
                elem.radius          = node->dimensions.width * 0.5;
                elem.clip_dimensions = node->clip_dimensions;
                elem.clip_position   = node->clip_position;
                elem.type            = TYPE_RENDER_CIRCLE;
                elem.color           = node->background_color;
                db_array_render_elements_append(&state->render_elements, elem);
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
                    start.clip_dimensions   = node->clip_dimensions;
                    start.clip_position     = node->clip_position;
                    start.start_pos         = (vector3d){half_x, two_thirds_y, node->position.z};
                    start.end_pos           = (vector3d){node->check_first_half_end.x, node->check_first_half_end.y, node->position.z};
                    start.color             = node->text_color; //@fix: make it dynamic
                    db_array_render_elements_append(&state->render_elements, start);

                    ui_render_element second = {};
                    start.start_pos          = (vector3d){half_x, two_thirds_y, node->position.z};
                    second.type              = TYPE_RENDER_LINE;
                    second.start_pos         = (vector3d){half_x, two_thirds_y, node->position.z};
                    second.clip_dimensions   = node->clip_dimensions;
                    second.clip_position     = node->clip_position;
                    second.end_pos           = (vector3d){node->check_second_half_end.x, node->check_second_half_end.y, node->position.z};
                    second.color             = node->text_color; //@fix: make it dynamic
                    db_array_render_elements_append(&state->render_elements, second);
                }
            }
            else if (node->type & TYPE_SLIDER)
            {
                //@warn: should we be doing this here?
                vector3d s_pos = node->position;
                if (node->is_active)
                {
                    // Ensure we don't divide by zero
                    if (node->dimensions.width > 0.0f)
                    {

                        f32 point_in_box = state->mouse_pos.x - node->position.x;
                        point_in_box     = db_clamp_integer(0, point_in_box, (f32)node->dimensions.width);

                        f32 percentage = point_in_box / (f32)node->dimensions.width;
                        f32 rel_val    = node->slider_min + (percentage * (node->slider_max - node->slider_min));

                        ASSERT(rel_val <= node->slider_max && rel_val >= node->slider_min);
                        *node->val_ptrs[node->val_ind] = rel_val;
                    }
                }

                s_pos.x  = *node->val_ptrs[node->val_ind] * (1 / node->slider_max) * node->dimensions.width;
                s_pos.x += node->position.x - 8;
                s_pos.x  = db_clamp_integer(node->position.x, s_pos.x, node->position.x + node->dimensions.width);

                s_pos.y = node->position.y;

                char buffer[16];
                snprintf(buffer, 16, "%2.f", *node->val_ptrs[node->val_ind]);

                rectangle *n_dimen    = &node->dimensions;
                vector3d  *n_pos      = &node->position;
                rectangle  text_dimen = state->measure_text_size(buffer, state->font_size);
                vector2d   center     = {(n_dimen->width * 0.5) + n_pos->x,
                                         (n_dimen->height * 0.5) + n_pos->y};

                ui_render_element slider_val = {};

                db_string txt              = db_string_make(&state->ui_string_chunk_arena, buffer); // @FIX: we are actually leaking mem here
                slider_val.label           = txt;                                                   // just copy the parent pointer
                slider_val.position        = (vector3d){center.x - (text_dimen.width * 0.5), center.y - (text_dimen.height * 0.5), node->position.z};
                slider_val.clip_dimensions = node->clip_dimensions;
                slider_val.clip_position   = node->clip_position;
                slider_val.type            = TYPE_RENDER_TEXT;
                slider_val.color           = node->text_color;

                db_array_render_elements_append(&state->render_elements, slider_val);

                ui_render_element slider_box = {};

                slider_box.position        = s_pos;
                slider_box.dimensions      = (rectangle){8, node->dimensions.height};
                slider_box.type            = TYPE_RENDER_RECTANGLE;
                slider_box.clip_dimensions = node->clip_dimensions;
                slider_box.clip_position   = node->clip_position;
                slider_box.color           = (color){52, 66, 56, 255}; // green
                db_array_render_elements_append(&state->render_elements, slider_box);
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
                shadow_button.clip_dimensions  = node->clip_dimensions;
                shadow_button.clip_position    = node->clip_position;
                shadow_button.color            = (color){255, 0, 0, 255}; // red

                db_array_render_elements_append(&state->render_elements, shadow_button);
            }
        }
    }

    db_array_ui_elem_ptr_clear(&state->windows);

    db_array_ui_elements_copy(&state->previous_ui_elements, &state->ui_elements); // this will also clear the array
    db_array_ui_elements_clear(&state->ui_elements);
    db_stack_s32_clear(&state->curr_parent_index);

    db_array_ui_elements_append(&state->ui_elements, (ui_elem){0});
    db_stack_s32_push(&state->curr_parent_index, 0);

    db_arena_reset(&state->ui_frame_arena);

    state->window_counter = 0; // reset the counter

    return state->render_elements;
}

void __ui_check_n_set_clip(ui_elem *parent, ui_elem *elem)
{
    rectangle *e_clip = &elem->clip_dimensions;
    rectangle *p_clip = &parent->clip_dimensions;

    // Calculate the overlap boundaries
    f32 min_x = db_max(parent->position.x, elem->position.x);
    f32 min_y = db_max(parent->position.y, parent->position.y);
    f32 max_x = db_min(parent->position.x + p_clip->width, elem->position.x + elem->dimensions.width);
    f32 max_y = db_min(parent->position.y + p_clip->height, elem->position.y + elem->dimensions.height);

    // If they don't overlap, the width/height will be negative
    if (max_x > min_x && max_y > min_y)
    {
        elem->clip_position.x = min_x;
        elem->clip_position.y = min_y;
        e_clip->width         = max_x - min_x;
        e_clip->height        = max_y - min_y;
    }
}
void __ui_set_clip_regions()
{
    // for (ui_elem *window = db_array_get_index_ptr(state->elements, 1);
    //      window->index != 0;
    //      window = db_array_get_index_ptr(state->elements, window->next_sibling_index))
    // {
    //     ASSERT(window->type & TYPE_WINDOW);
    //
    //     s32 w_child_count = window->child_count;
    //
    //     for (ui_elem *l_node = db_array_get_index_ptr(state->elements, window->first_child_index);
    //          w_child_count != 0 && l_node->index != 0;
    //          w_child_count--, l_node = db_array_get_index_ptr(state->elements, l_node->next_sibling_index))
    //     {
    //         __ui_check_n_set_clip(window, l_node);
    //         for (ui_elem *elem = db_array_get_index_ptr(state->elements, l_node->first_child_index);
    //              l_node->child_count != 0 && elem->index != 0;
    //              elem = db_array_get_index_ptr(state->elements, elem->next_sibling_index))
    //         {
    //             // hmmmmmm
    //             // @fix: think this troughhh
    //             __ui_check_n_set_clip(window, elem);
    //         }
    //     }
    // }
    s32      i      = 0;
    ui_elem *window = &state->ui_elements.data[0];
    db_array_for_each(state->windows, i, window)
    {
        for (ui_elem *l_node = db_array_ui_elements_get_index_ptr(&state->ui_elements, window->first_child_index);
             l_node->index != 0;
             l_node = db_array_ui_elements_get_index_ptr(&state->ui_elements, l_node->next_sibling_index))
        {
            l_node->clip_dimensions = window->clip_dimensions;
            l_node->clip_position   = window->clip_position;
            for (ui_elem *elem = db_array_ui_elements_get_index_ptr(&state->ui_elements, l_node->first_child_index);
                 l_node->child_count != 0 && elem->index != 0;
                 elem = db_array_ui_elements_get_index_ptr(&state->ui_elements, elem->next_sibling_index))
            {
                elem->clip_dimensions = window->clip_dimensions;
                elem->clip_position   = window->clip_position;
            }
        }
    }
}

void __ui_center_elem_text(ui_elem *elem)
{

    rectangle *n_dimen    = &elem->dimensions;
    vector3d  *n_pos      = &elem->position;
    rectangle *text_dimen = &elem->text_dimensions;
    vector2d   center     = {(n_dimen->width * 0.5) + n_pos->x,
                             (n_dimen->height * 0.5) + n_pos->y};
    // this is for the text itself
    switch (elem->children_pos_type)
    {
        case TYPE_POS_PLACE_CHILDREN_AT_CENTER:
            elem->text_position.x = center.x - (text_dimen->width * 0.5);
            elem->text_position.y = center.y - (text_dimen->height * 0.5);
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

// private implementation
void __ui_resize_n_reposition_elements(s32 index)
{
    //@fix: this kinda doesnt take care of the fact if we encounter an container inside an container
    // we could have a parent container have n + 1 containers inside it.
    for (ui_elem *window = db_array_ui_elements_get_index_ptr(&state->ui_elements, index);
         window->index != 0;
         window = db_array_ui_elements_get_index_ptr(&state->ui_elements, window->next_sibling_index))
    {
        ASSERT(window->type & TYPE_WINDOW);

        s32 w_child_count = window->child_count;

        for (ui_elem *l_node = db_array_ui_elements_get_index_ptr(&state->ui_elements, window->first_child_index);
             w_child_count != 0 && l_node->index != 0;
             w_child_count--, l_node = db_array_ui_elements_get_index_ptr(&state->ui_elements, l_node->next_sibling_index))
        {
            if (l_node->pos_type & TYPE_POS_FIXED)
            {
                continue;
            }
            //@todo: increase the width / height of the layout node depending on the width/height of the parent
            // check if the layout has the grow / shrink proerty though
            if (l_node->size_type & TYPE_SIZE_FLEX_GROW)
            {
                l_node->dimensions.width = window->dimensions.width - window->padding.x;
            }
            vector3d overriden_pos = l_node->position;

            // sanity check
            ASSERT(l_node->axis_type == window->child_axis_type);
            switch (l_node->pos_type)
            {
                case TYPE_POS_PLACE_SELF_AT_END:
                {
                    overriden_pos.x = l_node->position.x + (window->dimensions.width - l_node->dimensions.width - 3);
                    overriden_pos.y = l_node->position.y + (window->dimensions.height - l_node->dimensions.height - 3);
                }
                case TYPE_POS_PLACE_SELF_AT_CENTER:
                {
                    // @todo:
                }
                break;
                case TYPE_POS_PLACE_SELF_AT_START:
                {
                    // @todo:
                }
                break;
                    break;
                default:
                    break;
            }
            if (l_node->axis_type & TYPE_AXIS_ROW)
            {
                l_node->position.y = overriden_pos.y;
            }
            else if (l_node->axis_type & TYPE_AXIS_COLUMN)
            {
                l_node->position.x = overriden_pos.x;
            }

            b8 children_have_flex_property = false;

            s32 fixed_widths_sum            = 0;
            s32 child_with_flex_prop_shares = 0;
            s32 child_with_flex_props       = 0;

            ui_elem *elem = db_array_ui_elements_get_index_ptr(&state->ui_elements, l_node->first_child_index);
            for (s32 i = elem->index;
                 elem->parent_index == l_node->index;
                 elem = db_array_ui_elements_get_index_ptr(&state->ui_elements, elem->next_sibling_index))
            {
                if (elem->size_type & TYPE_SIZE_FLEX_GROW || elem->size_type & TYPE_SIZE_FLEX_SHRINK)
                {
                    children_have_flex_property  = true;
                    child_with_flex_prop_shares += elem->growth_factor;
                    child_with_flex_props++;
                }
                else // hmmm is this true?
                {
                    fixed_widths_sum += elem->dimensions.width;
                }
            }

            s32 remaining_space = l_node->dimensions.width - fixed_widths_sum;

            if (children_have_flex_property)
            {
                f32      remainder = (f32)remaining_space / child_with_flex_prop_shares;
                vector3d cursor    = l_node->position;

                elem = db_array_ui_elements_get_index_ptr(&state->ui_elements, l_node->first_child_index);
                for (s32 i = elem->index;
                     elem->parent_index == l_node->index;
                     elem = db_array_ui_elements_get_index_ptr(&state->ui_elements, elem->next_sibling_index))
                {
                    if (elem->size_type & TYPE_SIZE_FIXED)
                    {
                        elem->position = cursor;
                    }
                    else if (elem->size_type & TYPE_SIZE_FLEX_GROW)
                    {
                        elem->position         = cursor;
                        elem->dimensions.width = remainder * elem->growth_factor;

                        if (elem->render_type & TYPE_RENDER_TEXT)
                        {
                            __ui_center_elem_text(elem);
                        }
                    }
                    cursor.x += elem->dimensions.width;
                }
            }
            else if (!(l_node->children_pos_type & TYPE_POS_NONE))
            {
                // well its already placed as AT START so
                s32 remaining_space = l_node->dimensions.width - fixed_widths_sum;
                s32 children_count  = l_node->child_count;

                vector3d cursor  = (vector3d){l_node->position.x, l_node->position.y, l_node->position.z};
                vector2d padding = {0};

                switch (l_node->children_pos_type)
                {
                    case TYPE_POS_PLACE_CHILDREN_AT_END:
                    {
                        cursor.x = l_node->position.x + remaining_space;
                    }
                    break;
                    case TYPE_POS_PLACE_CHILDREN_AT_CENTER:
                    {
                        cursor.x = l_node->position.x + ((f32)remaining_space / 2);
                    }
                    break;
                    case TYPE_POS_SPACE_CHILDREN_EVENLY:
                    {
                        padding.x = (f32)remaining_space / (children_count + 1);
                    }
                    break;
                    case TYPE_POS_SPACE_CHILDREN_BETWEEN:
                    {
                        if (children_count > 1)
                        {
                            padding.x = (f32)remaining_space / (children_count - 1);
                        }
                    }
                    break;
                    case TYPE_POS_SPACE_CHILDREN_AROUND:
                    {
                        padding.x  = (f32)remaining_space / children_count;
                        cursor.x  += padding.x / 2;
                    }
                    break;
                    default:
                    {
                    }
                    break;
                }

                elem = db_array_ui_elements_get_index_ptr(&state->ui_elements, l_node->first_child_index);
                for (s32 i = elem->index;
                     elem->parent_index == l_node->index;
                     elem = db_array_ui_elements_get_index_ptr(&state->ui_elements, elem->next_sibling_index))
                {
                    elem->position.x = cursor.x;
                    elem->position.y = cursor.y;

                    switch (elem->pos_type)
                    {
                        case TYPE_POS_PLACE_SELF_AT_END:
                        {
                            ASSERT(l_node->child_axis_type == elem->axis_type);
                            if (elem->axis_type & TYPE_AXIS_ROW)
                            {
                                elem->position.y += l_node->dimensions.height - elem->dimensions.height;
                            }
                            else if (elem->axis_type & TYPE_AXIS_COLUMN)
                            {
                                elem->position.x += l_node->dimensions.width - elem->dimensions.width;
                            }
                        }
                        break;
                        default:
                            break;
                    }

                    if (elem->render_type & TYPE_RENDER_TEXT)
                    {
                        __ui_center_elem_text(elem);
                    }
                    cursor.x += elem->dimensions.width + padding.x;
                }
            }
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

    ui_elem *first_child = db_array_ui_elements_get_index_ptr(&state->ui_elements, index);

    ui_elem *parent       = db_array_ui_elements_get_index_ptr(&state->ui_elements, first_child->parent_index);
    //@fix: why is this here
    parent->clip_position = parent->position;

    vector3d cursor = {parent->position.x + padding_x, parent->position.y + padding_y, parent->position.z};

    ui_elem *node = first_child;

    for (; node->index != 0;
         node = db_array_ui_elements_get_index_ptr(&state->ui_elements, node->next_sibling_index))
    {
        if (node->pos_type & TYPE_POS_FIXED)
        {
            continue;
        }
        if (node->action_type & TYPE_ACTION_ANCHORED_TO_PARENT)
        {
            if (node->is_active)
            {
                f32 delta_x         = state->mouse_pos.x - state->prev_mouse_pos.x;
                f32 delta_y         = state->mouse_pos.y - state->prev_mouse_pos.y;
                // @warn: clamp needed ??
                parent->position.x += delta_x;
                parent->position.y += delta_y;

                cursor                = parent->position;
                // @fix: this should not be here
                parent->clip_position = parent->position;
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
            __ui_center_elem_text(node);
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

    ui_elem              *elem   = &state->ui_elements.data[0];
    ui_elem              *parent = &state->ui_elements.data[0];
    db_array_ui_elements *arr    = &state->ui_elements;

    // postorder traversal
    for (s32 i = arr->length - 1; i >= 0; i--)
    {
        elem   = &arr->data[i];
        parent = &arr->data[elem->parent_index];
        if (parent->index == 0)
            break;

        if (elem->action_type & TYPE_ACTION_REFLECT_TO_PARENT)
        {
            if (elem->is_active)
            {
                f32 delta_x = state->mouse_pos.x - state->prev_mouse_pos.x;
                f32 delta_y = state->mouse_pos.y - state->prev_mouse_pos.y;

                // @warn: clamp needed ??
                parent->dimensions.width  += delta_x;
                parent->dimensions.height += delta_y;
            }
        }

        if (elem->size_type & TYPE_SIZE_BASED_ON_TEXT)
        {
            elem->text_dimensions = state->measure_text_size(db_string_get_cstr(&state->ui_frame_arena, elem->label), state->font_size);

            elem->dimensions.width  = elem->text_dimensions.width + 2 * padding_x;
            elem->dimensions.height = elem->text_dimensions.height + 2 * padding_y;
        }

        if (parent->size_type & TYPE_SIZE_BASED_ON_CHILD)
        {
            if (parent->child_axis_type & TYPE_AXIS_COLUMN)
            {
                parent->dimensions.width   = db_max(elem->dimensions.width, parent->dimensions.width);
                parent->dimensions.height += elem->dimensions.height + 2 * padding_y;
            }
            else if (parent->child_axis_type & TYPE_AXIS_ROW)
            {
                parent->dimensions.width  += elem->dimensions.width + 2 * padding_x;
                parent->dimensions.height  = db_max(elem->dimensions.height, parent->dimensions.height);
            }
            //@fix: i dont think this should be heree
            parent->clip_dimensions = parent->dimensions;
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

void __ui_check_action(ui_elem *box)
{
    b8       is_hot    = false;
    b8       is_active = false;
    ui_elem *prev      = db_array_ui_elements_find(&state->previous_ui_elements, box, array_elem_compare);

    if (prev && box->action_type & TYPE_ACTION_HOVERABLE)
    {
        is_hot = __ui_check_mouse_hover(prev->position, prev->dimensions);
        if (is_hot)
        {
            state->hot_elem_id = box->id;
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
                state->active_elem_id = box->id;
            }
            // find the parent_window

            ui_elem *parent_window = box;
            while (!(parent_window->type & TYPE_WINDOW))
            {
                parent_window = db_array_ui_elements_get_index_ptr(&state->ui_elements, parent_window->parent_index);
            }
            state->curr_top_parent = parent_window;
        }
        else if (state->mouse_btn_state & TYPE_MOUSE_LEFT_BUTTON_RELEASED)
        {
            is_active             = false;
            state->active_elem_id = 0;
        }

        // if the user is still pressing the button then
        if (box->action_type & TYPE_ACTION_DRAGGABLE
            && state->mouse_btn_state & TYPE_MOUSE_LEFT_BUTTON_PRESSED
            && state->prev_mouse_btn_state & TYPE_MOUSE_LEFT_BUTTON_PRESSED
            && state->active_elem_id == box->id)
        {
            is_active             = true;
            state->active_elem_id = box->id;
        }

        if (is_hot)
        {
            box->background_color = (color){188, 188, 188, 255};
        }
    }

    box->is_hot    = is_hot;
    box->is_active = is_active;
}

// this is also the last call for the ui sys in the frame

ui_elem *__ui_get_prev_sibling(s32 parent_index)
{
    ui_elem *parent = db_array_ui_elements_get_index_ptr(&state->ui_elements, parent_index);
    ASSERT(parent != NULL);

    ui_elem *child = db_array_ui_elements_get_index_ptr(&state->ui_elements, parent->first_child_index);
    for (; child->next_sibling_index != 0; child = db_array_ui_elements_get_index_ptr(&state->ui_elements, child->next_sibling_index))
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

ui_elem *__ui_create_box(db_string           str,
                         ui_elem_type        type,
                         ui_elem_size_type   size_type,
                         ui_elem_pos_type    pos_type,
                         ui_elem_pos_type    children_pos_type,
                         ui_elem_action_type action_type,
                         ui_axis_type        axis_type,
                         ui_axis_type        axis_child_type,
                         ui_render_type      render_command_type,
                         vector2d            position,
                         rectangle           dimensions)
{
    ui_elem box     = {};
    // see if there is a parent for this
    ui_elem *parent = &state->ui_elements.data[0];

    parent = db_array_ui_elements_get_index_ptr(&state->ui_elements, db_stack_s32_peek(&state->curr_parent_index));
    // @todo: maybe we should push a root node after the sentinel node that takes up the entire screen
    // like how raddebugger has its root node.
    // this is to prevent write access on the sentinel node
    parent->child_count++;

    u64 id = db_murmur64A_seed(db_string_get_cstr(&state->ui_frame_arena, str), str.length, parent == NULL ? DB_HASH_SEED : parent->id);
    box.id = id;

    // this still deosnt take care of the fact if in the same layout row / column
    // we have two different ui elems with the same label
    // this will still generate the same id
    ui_elem *prev = db_array_ui_elements_find(&state->previous_ui_elements, &box, array_elem_compare); // shit

    if (prev)
    {
        box.label = prev->label;
    }
    else
    {
        box.label = db_string_make(&state->ui_string_chunk_arena, db_string_get_cstr(&state->ui_frame_arena, str));
    }

    // it doesnt matter if the parent.id is 0 because 0 is the sentinel node
    box.type              = type;
    box.action_type       = action_type;
    box.size_type         = size_type;
    // @fix: check if they are incompatible
    box.children_pos_type = children_pos_type;
    box.index             = state->ui_elements.length;
    box.position          = (vector3d){position.x, position.y, 0};
    if (prev)
    {
        box.position = prev->position;
    }
    box.dimensions = dimensions;
    // @todo: make this robust
    if (prev && type & TYPE_WINDOW)
    {
        box.size_type         = TYPE_SIZE_FIXED;
        box.dimensions.width  = db_max(dimensions.width, prev->dimensions.width);
        box.dimensions.height = db_max(dimensions.height, prev->dimensions.height);
        box.clip_dimensions   = box.dimensions;
    }
    box.axis_type       = axis_type;
    box.child_axis_type = axis_child_type;
    box.pos_type        = pos_type;
    box.child_count     = 0;
    box.parent_index    = parent->index;
    box.render_type     = render_command_type;

    ui_elem *prev_sibling = __ui_get_prev_sibling(parent->index);

    if (prev_sibling->parent_index == box.parent_index) // checking cause the prev sibling might be the sentinel node -> which means it didnt have any prev siblings
    {
        prev_sibling->next_sibling_index = box.index;
        box.prev_sibling_index           = prev_sibling->index;
    }

    if (pos_type & TYPE_POS_FIXED)
    {
        box.position = (vector3d){position.x, position.y, 0};
    }
    if (parent->first_child_index == 0) // set the first parent child index
    {
        parent->first_child_index = box.index;
    }
    if (axis_type & TYPE_AXIS_BASED_ON_PARENT)
    {
        box.axis_type = parent->child_axis_type;
        ASSERT(!(parent->child_axis_type & TYPE_AXIS_NONE));
    }

    if (render_command_type & TYPE_RENDER_RECTANGLE
        || render_command_type & TYPE_RENDER_TEXT
        || render_command_type & TYPE_RENDER_CIRCLE)
    {
        box.text_color       = (color){255, 255, 255, 255};
        box.background_color = (color){90, 128, 133, 255};
    }

    // do this at the last. Dummy
    db_array_ui_elements_append(&state->ui_elements, box);
    if (type & TYPE_WINDOW || type & TYPE_CONTAINER)
    {
        if (prev && type & TYPE_WINDOW)
        {
            box.dimensions = prev->dimensions;
            db_array_ui_elem_ptr_append(&state->windows, db_array_ui_elements_get_last_ptr(&state->ui_elements));
        }
        db_stack_s32_push(&state->curr_parent_index, box.index);
    }
    return db_array_ui_elements_get_last_ptr(&state->ui_elements);
}
