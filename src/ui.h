#pragma once
#include "db.h"

typedef struct
{
    f32 x;
    f32 y;
} vector2d;

typedef struct
{
    f32 x;
    f32 y;
    f32 z;
} vector3d;

typedef struct
{
    u8 r; // Color red value
    u8 g; // Color green value
    u8 b; // Color blue value
    u8 a; // Color alpha value
} color;

typedef struct
{
    s32 width;
    s32 height;
} rectangle;

typedef enum
{
    TYPE_NONE         = bit0,
    TYPE_WINDOW       = bit1,
    TYPE_BUTTON       = bit2,
    TYPE_RADIO_BUTTON = bit3,
    TYPE_CHECKBOX     = bit4,
    TYPE_SLIDER       = bit5,
    TYPE_TEXT_FIELD   = bit6,
    TYPE_SCROLL_BAR   = bit7,
    TYPE_CONTAINER    = bit8,
    TYPE_LABEL        = bit9,
    TYPE_TREE         = bit10, //@todo: think of a better name
} ui_elem_type;

typedef enum
{
    TYPE_SIZE_NONE            = bit0,
    TYPE_SIZE_FIXED           = bit1, // this means you passed the size yourself
    TYPE_SIZE_BASED_ON_CHILD  = bit2,
    TYPE_SIZE_BASED_ON_PARENT = bit3,
    TYPE_SIZE_BASED_ON_TEXT   = bit4,

    // can be 0 1 2 3
    // grow and shrink can be used simultaneously
    // if specified shink/grow then you have to provide the basis
    // otherwise it will default to 0
    TYPE_SIZE_FLEX_GROW   = bit5,
    TYPE_SIZE_FLEX_SHRINK = bit6,
} ui_elem_size_type;

typedef enum
{
    TYPE_POS_NONE = bit0, // this is also saying just follow the parent position rules

    TYPE_POS_PLACE_CHILDREN_AT_END    = bit1,
    TYPE_POS_PLACE_CHILDREN_AT_CENTER = bit2,
    TYPE_POS_PLACE_CHILDREN_AT_START  = bit3,

    // if you are familiar with css then you'll know the following:
    // If any of the children have the property TYPE_SIZE_FLEX_GROW then
    // the following enums will be discarded.
    // padding + position
    TYPE_POS_SPACE_CHILDREN_EVENLY  = bit4,
    TYPE_POS_SPACE_CHILDREN_BETWEEN = bit5,
    TYPE_POS_SPACE_CHILDREN_AROUND  = bit6,

    // if this is set then you have to provide a padding value
    // this is incompatible with TYPE_POS_CHILDREN_EVENLY // hmmmm is it really ?
    TYPE_POS_CHIDREN_PADDING = bit7,

} ui_elem_children_position_type; // this is mostly only for layouts

typedef enum
{
    TYPE_ACTION_NONE               = bit0, // it will react to nothing
    TYPE_ACTION_HOVERABLE          = bit1,
    TYPE_ACTION_DRAGGABLE          = bit2,
    TYPE_ACTION_RESIZABLE          = bit3,
    TYPE_ACTION_PRESSABLE          = bit4,
    // meant to be used with DRAGGABLE AND RESIZABLE
    // if you drag an element with this proprerty, it will also reflect on the parent
    TYPE_ACTION_ANCHORED_TO_PARENT = bit4,
} ui_elem_action_type;

typedef enum
{
    TYPE_AXIS_NONE            = bit0,
    TYPE_AXIS_COLUMN          = bit1,
    TYPE_AXIS_ROW             = bit2,
    TYPE_AXIS_BASED_ON_PARENT = bit3,
} ui_axis_type;

// ui actions states
typedef enum
{
    TYPE_MOUSE_NONE                  = bit0,
    TYPE_MOUSE_RIGHT_BUTTON_PRESSED  = bit1, // hmmm
    TYPE_MOUSE_LEFT_BUTTON_PRESSED   = bit2,
    TYPE_MOUSE_RIGHT_BUTTON_RELEASED = bit3,
    TYPE_MOUSE_LEFT_BUTTON_RELEASED  = bit4,
} ui_mouse_button_state;

typedef enum
{
    TYPE_RENDER_NONE      = bit0,
    TYPE_RENDER_RECTANGLE = bit1,
    TYPE_RENDER_CIRCLE    = bit2,
    TYPE_RENDER_TEXT      = bit3,
    TYPE_RENDER_LINE      = bit4,
} ui_render_type;

typedef struct
{
    ui_elem_action_type action_type;
    ui_elem_size_type   size_type;
    vector2d            position;
    rectangle           dimensions;
    color               background_color;
} ui_window_desc;

typedef struct
{
    ui_elem_action_type            action_type;
    ui_render_type                 render_command_type;
    ui_elem_children_position_type children_position_type;
    vector2d                       padding;
} ui_layout_desc;

typedef struct
{
    f32  number_of_values; // can be 1, 2, 3 or 4
    f32 *first_val;
    f32 *second_val;
    f32 *third_val;
    f32 *fourth_val;
    f32  min;
    f32  max;
} ui_slider_desc;

typedef struct
{
    // this is also a linked list/intrusive list
    u64 id;
    s64 index; // index to itself
    s64 parent_index;

    // for graph traversal
    s64 first_child_index;
    s64 next_sibling_index;
    s64 prev_sibling_index;
    s64 child_count; // number of children

    // element traits
    ui_elem_type                   type;
    ui_elem_action_type            action_type;
    ui_elem_size_type              size_type;
    ui_elem_children_position_type children_pos_type;
    ui_axis_type                   axis_type;       // which axis does this elem follow
    ui_axis_type                   axis_child_type; // which axis does this elements children follow
                                                    // this will affect how it will be laid out in the final.
                                                    //

    vector3d  position; // absolute position. the z val used for windows/panels sorting.
    rectangle dimensions;
    color     background_color;
    vector2d  padding;
    s32       growth_factor; // will default to 0 if set TYPE_SIZE_FLEX_GROW
    s32       shrink_factor; // will default to 0 if set TYPE_SIZE_FLEX_GROW

    // persistent state
    b8 is_hot;
    b8 is_active;

    // type specific data // lol
    ui_render_type render_type;

    b8      *checkbox_val;
    vector2d check_common_start;
    vector2d check_first_half_end;
    vector2d check_second_half_end;

    s32      radio_button_id;
    s32     *choice;
    vector2d radio_button_center;

    f32  slider_min;
    f32  slider_max;
    s32  val_ind;
    f32 *val_ptrs[4];

    // text label
    db_string label;
    vector3d  text_position;
    rectangle text_dimensions;
    color     text_color;
} ui_elem;

typedef struct
{
    ui_render_type type;

    rectangle dimensions;
    vector3d  position;
    color     color;
    // if its TYPE_CIRCLE
    vector3d center;
    f32      radius;
    // if its TYPE_TEXT
    db_string label;
    // if its TYPE_LINE
    vector3d start_pos;
    vector3d end_pos;

} ui_render_element; // @note: better name?

typedef struct
{
    ui_mouse_button_state prev_mouse_btn_state;
    vector2d              prev_mouse_pos;
    ui_mouse_button_state mouse_btn_state;
    vector2d              mouse_pos;

    db_arena arena; // this is just used once actually ?
                    //  we call this and ask for mem only when we do ui_init

    u32 font_size;
    f32 scale; // @todo: better scaling logic. Actually I havent even implemented this

    // @todo: explain better
    u64 hot_elem_id;
    u64 active_elem_id; // to check if the current active element

    rectangle (*measure_text_size)(const char *text, u32 font_size);

    db_stack(ui_axis_type)      curr_axis;   // this is for the elements which are inside the row/column scope{}. I dont think we use this
    db_stack(s32)               curr_parent; // index of the curr parent
    db_array(ui_elem)           elements;
    db_array(ui_elem)           prev_elem_state; // previous frame elements
    db_array(ui_elem *)         windows;
    db_array(ui_render_element) render_elements; // resets every frame

    // @warn: Experimental
    s32      window_counter;
    ui_elem *curr_top_parent; // the parent which is at the top of other windows
} ui_state;

db_return_code ui_init(rectangle (*measure_text_size)(const char *text, u32 font_size));
db_return_code ui_update_mouse(vector2d mouse_pos, ui_mouse_button_state mouse_button_state);

// this should be called in the end. Because this will reset the ui.
db_array(ui_render_element) ui_get_render_commands();

#define ui_row(...) defer_loop(__ui_row_begin(&(ui_layout_desc){__VA_ARGS__}), __ui_row_end())
#define ui_column(...) defer_loop(__ui_column_begin(&(ui_layout_desc){__VA_ARGS__}), __ui_column_end())

#define ui_window(title) defer_loop(__ui_window_begin(title, &(ui_window_desc){0}), __ui_window_end())
#define ui_window_ext(title, ...) defer_loop(__ui_window_begin(title, &(ui_window_desc){__VA_ARGS__}), __ui_window_end())

b8 __ui_window_begin(const char *title, ui_window_desc *desc);
b8 __ui_window_end(void);
b8 __ui_row_begin(ui_layout_desc *desc);
b8 __ui_row_end(void);
b8 __ui_column_begin(ui_layout_desc *desc);
b8 __ui_column_end(void);

b8   ui_button(const char *label);
b8   ui_label(const char *label);
void ui_checkbox(const char *label, b8 *boolean); //

// id -> the ordering on the list of buttons
// For example :
//      0 car --> id = 0
//      0 motorcycle --> id = 1
//      0 bycycle --> id = 2
//
// based on what the user clicks the choice variable will be set to it.
// if the user picks car then the choice will be set to the passed id
// same thing with motorcycle etc.
void ui_radio_button(const char *label, s32 *choice, s32 id);

#define ui_slider(label, min, max, value) __ui_slider(label, &(ui_slider_desc){.number_of_values = 1, .first_val = &value, .min = min, .max = max})
#define ui_slider_ext(label, ...) __ui_slider(label, &(ui_slider_desc){__VA_ARGS__})
void __ui_slider(const char *label, ui_slider_desc *desc);
