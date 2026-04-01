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
    f32 width;
    f32 height;
} rectangle;

typedef enum
{
    TYPE_NONE          = bit0,
    TYPE_WINDOW        = bit1,
    TYPE_BUTTON        = bit2,
    TYPE_RADIO_BUTTONS = bit3,
    TYPE_CHECKBOX      = bit4,
    TYPE_SLIDER        = bit5,
    TYPE_TEXT_FIELD    = bit6,
    TYPE_SCROLL_BAR    = bit7,
    TYPE_LAYOUT_NODE   = bit8,
    // i think we should seperate out the render commands from this
    TYPE_TEXT          = bit9,
} ui_elem_type;

typedef enum
{
    TYPE_SIZE_NONE          = bit0,
    TYPE_BASED_ON_CHILD     = bit1,
    TYPE_SIZE_FIXED         = bit2,
    TYPE_BASED_ON_TEXT_SIZE = bit3,
} ui_elem_size_type;

typedef enum
{
    TYPE_ACTION_NONE      = bit0,
    TYPE_ACTION_FIXED     = bit1,
    TYPE_ACTION_DRAGGABLE = bit2,
    TYPE_ACTION_RESIZABLE = bit3,
    TYPE_ACTION_PRESSABLE = bit4,
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

typedef struct
{
    ui_elem_action_type action_type;
    ui_elem_size_type   size_type;
    vector2d            position;
    rectangle           dimensions;
} ui_window_desc;

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
    ui_elem_type        type;
    ui_elem_action_type action_type;
    ui_elem_size_type   size_type;
    ui_axis_type        axis_type;       // which axis does this elem follow
    ui_axis_type        axis_child_type; // which axis does this elements children follow
                                         // this will affect how it will be laid out in the final.
                                         //

    vector3d  position; // absolute position. the z val used for windows/panels sorting.
    rectangle dimensions;

    // element makeup 💅
    color background_color;
    color text_color;

    // persistent state
    b8 is_hot;
    b8 is_active;

    // type specific data // lol
    b8      *checkbox_val;
    vector2d check_common_start;
    vector2d check_first_half_end;
    vector2d check_second_half_end;

    // text label
    db_string label;
} ui_elem;

typedef struct
{
    ui_mouse_button_state mouse_btn_state;
    vector2d              mouse_pos;

    db_arena arena; // this is just used once actually ?
                    //  we call this and ask for mem only when we do ui_init

    u64 prev_elem_index; // so that the pushed elem knows what it's sibiling is

    db_stack(ui_axis_type) curr_axis; // this is for the elements which are inside the row/column scope{}.
    db_stack(s32) curr_parent;        // index of the curr parent
    db_array(ui_elem) elements;
    db_array(ui_elem) prev_elem_state; // previous frame elements
    u32 font_size;
    rectangle (*measure_text_width)(const char *text, u32 font_size);
} ui_state;

db_return_code ui_init(rectangle (*measure_text_width)(const char *text, u32 font_size));
db_return_code ui_update_mouse(vector2d mouse_pos, ui_mouse_button_state mouse_button_state);

// this should be called in the end. Because this will reset the ui.
db_array(ui_elem) ui_get_render_commands();

#define ui_row() defer_loop(__ui_row_begin(), __ui_row_end())
#define ui_column() defer_loop(__ui_column_begin(), __ui_column_end())

#define ui_window(title) defer_loop(__ui_window_begin(title, &(ui_window_desc){0}), __ui_window_end())
#define ui_window_ext(title, ...) defer_loop(__ui_window_begin(title, &(ui_window_desc){__VA_ARGS__}), __ui_window_end())

b8 __ui_window_begin(const char *title, ui_window_desc *desc);
b8 __ui_window_end(void);
b8 __ui_row_begin(void);
b8 __ui_row_end(void);
b8 __ui_column_begin(void);
b8 __ui_column_end(void);

b8   ui_button(const char *label);
b8   ui_label(const char *label);
void ui_checkbox(const char *label, b8 *boolean); //
