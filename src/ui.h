#pragma once
#define DBH_IMPLEMENTATION
#include "dbh.h"

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

typedef enum
{
    TYPE_WINDOW        = bit1,
    TYPE_BUTTON        = bit2,
    TYPE_RADIO_BUTTONS = bit3,
    TYPE_CHECKBOXES    = bit4,
    TYPE_SLIDER        = bit5,
    TYPE_TEXT_FIELD    = bit6,
    TYPE_SCROLL_BAR    = bit7,
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
} ui_elem_action_type;

typedef enum
{
    TYPE_RIGHT_BUTTON_PRESSED  = bit1,
    TYPE_LEFT_BUTTON_PRESSED   = bit2,
    TYPE_RIGHT_BUTTON_RELEASED = bit3,
    TYPE_LEFT_BUTTON_RELEASED  = bit4,
} ui_mouse_state;

typedef struct
{
    // this is also a linked list/intrusive list
    s32 parent_ind;
    s32 child_ind;
    s32 next_elem_ind;
    s32 prev_elem_ind;

    // element traits 
    ui_elem_type        type;
    ui_elem_action_type action_type;
    ui_elem_size_type   size_type;
    vector3d            pos; // absolute position. the z val used for windows/panels sorting.
    rectangle           rect;
    
    // text label
    const char         *label;
} ui_elem;

typedef struct
{
    vector2d  mouse_pos;
    dbh_arena arena;

    dbh_array(ui_elem) elements;
    dbh_array(ui_elem) prev_elem_state;
} ui_state;

dbh_return_code ui_init(f32 screen_width, f32 screen_height);
dbh_return_code ui_update_mouse(vector2d mouse_pos, s32 mouse_button_state);
dbh_array(ui_elem) ui_get_render_commands();

// this is optional, because by default a window will be TYPE_ACTION_DRAGGABLE | TYPE_ACTION_RESIZABLE |
// TYPE_BASED_ON_CHILD
bool ui_set_next_window_state(rectangle size, ui_elem_size_type size_type, vector3d pos,
                              ui_elem_action_type action_type);
bool ui_window(const char *title);

bool ui_label(const char* label);
// cannot be a standalone button, it has to be a part of a window/panel
bool ui_button(const char *label);
