#pragma once
#include "db.h"

typedef enum
{
    TYPE_NONE          = bit1,
    TYPE_WINDOW        = bit2,
    TYPE_BUTTON        = bit3,
    TYPE_RADIO_BUTTONS = bit4,
    TYPE_CHECKBOXES    = bit5,
    TYPE_SLIDER        = bit6,
    TYPE_TEXT_FIELD    = bit7,
    TYPE_SCROLL_BAR    = bit8,
} ui_elem_type;

typedef enum
{
    TYPE_BASED_ON_CHILD     = bit1,
    TYPE_SIZE_FIXED         = bit2,
    TYPE_BASED_ON_TEXT_SIZE = bit3,
} ui_elem_size_type;

typedef enum
{
    TYPE_ACTION_FIXED     = bit1,
    TYPE_ACTION_DRAGGABLE = bit2,
    TYPE_ACTION_RESIZABLE = bit3,
    TYPE_ACTION_PRESSABLE = bit4,
} ui_elem_action_type;

typedef enum
{
    TYPE_RIGHT_BUTTON_PRESSED  = bit1,
    TYPE_LEFT_BUTTON_PRESSED   = bit2,
    TYPE_RIGHT_BUTTON_RELEASED = bit3,
    TYPE_LEFT_BUTTON_RELEASED  = bit4,
} ui_mouse_state;

typedef enum
{
    TYPE_AXIS_COLUMN = bit1,
    TYPE_AXIS_ROW    = bit2,
} ui_axis_type;

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
    f32 width;
    f32 height;
} rectangle;

typedef struct
{
    ui_axis_type axis;

    // this is also a linked list/intrusive list
    u64 id;
    s64 index; // index to itself
    s64 parent_index;
    s64 child_index;
    s64 next_elem_index; // usually = index + 1 if index - 1 < array count
    s64 prev_elem_index; // usually = index - 1 if index > 1

    // element traits
    ui_elem_type        type;
    ui_elem_action_type action_type;
    ui_elem_size_type   size_type;
    ui_axis_type        axis_type; // which axis does this elem follow
                                   // this will affect how it will be laid out in the final.
    // position stuff
    vector3d  pos; // absolute position. the z val used for windows/panels sorting.
    rectangle dimensions;

    // text label
    db_string label;
} ui_elem;

typedef struct
{
    vector2d mouse_pos;
    db_arena arena; // this is just used once actually ?
                    //  we call this and ask for mem only when we do ui_init

    u64 prev_elem_index; // so that the pushed elem knows what it's sibiling is

    db_stack(s32) curr_axis; // this is for the elements which are inside the row/column scope{}.
    db_stack(ui_elem) curr_parent;
    db_array(ui_elem) elements;
    db_array(ui_elem) prev_elem_state; // previous frame elements
    u32 font_size;
    rectangle (*measure_text_width)(const char *text, u32 font_size);
} ui_state;

db_return_code ui_init(rectangle (*measure_text_width)(const char *text, u32 font_size));
db_return_code ui_update_mouse(vector2d mouse_pos, s32 mouse_button_state);
// this should be called in the end. Because this will reset the ui.
db_array(ui_elem) ui_get_render_commands();

// this is optional, because by default a window will be TYPE_ACTION_DRAGGABLE | TYPE_ACTION_RESIZABLE |
// TYPE_BASED_ON_CHILD
b8 ui_set_next_window_state(rectangle size, ui_elem_size_type size_type, vector2d pos, ui_elem_action_type action_type);
b8 ui_window(const char *title);
b8 ui_label(const char *label);
// cannot be a standalone button, it has to be a part of a window/panel
b8 ui_button(const char *label);

b8 __ui_row_begin(void);
b8 __ui_row_end(void);
b8 __ui_column_begin(void);
b8 __ui_column_end(void);

#define ui_row() defer_loop(__ui_row_begin(), __ui_row_end())
#define ui_column() defer_loop(__ui_column_begin(), __ui_column_end())
