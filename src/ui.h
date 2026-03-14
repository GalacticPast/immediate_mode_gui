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
    TYPE_BUTTON        = bit1,
    TYPE_RADIO_BUTTONS = bit2,
    TYPE_CHECKBOXES    = bit3,
    TYPE_SLIDER        = bit4,
    TYPE_TEXT_FIELD    = bit5,
    TYPE_sCROLL_BAR    = bit6,
} ui_elem_type;

typedef struct
{
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
   dbh_array(ui_elem) elements; 
} ui_state;
