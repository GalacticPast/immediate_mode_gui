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
    TYPE_GROWABLE_BASED_ON_CHILD = bit1,
    TYPE_FIXED                   = bit2,
} ui_elem_size_type;

typedef struct
{
    const char  *label;
    ui_elem_type type;
    vector3d     pos; // absolute position. the z val used for windows/panels sorting.
    rectangle    rect;
    s32          parent_ind;
    s32          child_ind;
    s32          next_ind;
    s32          prev_ind;
} ui_elem;

typedef struct
{
    vector2d  mouse_pos;
    dbh_arena arena;
    dbh_array(ui_elem) elements;
    dbh_array(ui_elem) prev_elem_state;
} ui_state;

dbh_return_code ui_init(f32 screen_width, f32 screen_height);
dbh_return_code ui_update_mouse_pos(vector2d mouse_pos);
dbh_array(ui_elem) ui_get_render_commands();

// cannot be a standalone button, it has to be a part of a window.
bool ui_window(const char *title);
bool ui_button(const char *label);
