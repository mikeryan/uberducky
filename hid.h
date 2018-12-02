/*
 * Copyright 2019 Mike Ryan
 *
 * This file is part of Uberducky and is released under the terms of the
 * GPL version 2. Refer to COPYING for more information.
 */

#ifndef __HID_H__
#define __HID_H__

#include <stdint.h>

// time in ms a key is held down
#define DOWN_TIME 10

// key types
#define K_CHAR  0
#define K_ENTER 1
#define K_TAB   2
#define K_ESC   3
#define K_BACK  4
#define K_RAW   5

// modifiers
#define M_NONE  0
#define M_CTRL  1
#define M_SHIFT 2
#define M_ALT   4
#define M_META  8
typedef struct _keystroke_t {
    int type;
    int mod;
    uint8_t chr;
} keystroke_t;

void hid_encode(keystroke_t *s, uint8_t *r);

#endif /* __HID_H__ */
