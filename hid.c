/*
 * Copyright 2019 Mike Ryan
 *
 * This file is part of Uberducky and is released under the terms of the
 * GPL version 2. Refer to COPYING for more information.
 */

#include <ctype.h>
#include <string.h>

#include "hid.h"

typedef struct _enc_t {
    uint8_t val;
    int shift;
} enc_t;

// 0x20 - 0x40
enc_t enc_20[] = {
    { 0x2c, 0 } /*   */, { 0x1e, 1 } /* ! */, { 0x34, 1 } /* " */,
    { 0x20, 1 } /* # */, { 0x21, 1 } /* $ */, { 0x22, 1 } /* % */,
    { 0x24, 1 } /* & */, { 0x34, 0 } /* ' */, { 0x26, 1 } /* ( */,
    { 0x27, 1 } /* ) */, { 0x25, 1 } /* * */, { 0x2e, 1 } /* + */,
    { 0x36, 0 } /* , */, { 0x2d, 0 } /* - */, { 0x37, 0 } /* . */,
    { 0x38, 0 } /* / */, { 0x27, 0 } /* 0 */, { 0x1e, 0 } /* 1 */,
    { 0x1f, 0 } /* 2 */, { 0x20, 0 } /* 3 */, { 0x21, 0 } /* 4 */,
    { 0x22, 0 } /* 5 */, { 0x23, 0 } /* 6 */, { 0x24, 0 } /* 7 */,
    { 0x25, 0 } /* 8 */, { 0x26, 0 } /* 9 */, { 0x33, 1 } /* : */,
    { 0x33, 0 } /* ; */, { 0x36, 1 } /* < */, { 0x2e, 0 } /* = */,
    { 0x37, 1 } /* > */, { 0x38, 1 } /* ? */, { 0x1f, 1 } /* @ */,
};

enc_t enc_5b[] = {
    { 0x2f, 0 } /* [ */, { 0x31, 0 } /* \ */, { 0x30, 0 } /* ] */,
    { 0x23, 1 } /* ^ */, { 0x2d, 1 } /* _ */, { 0x35, 0 } /* ` */,
};

enc_t enc_7b[] = {
    { 0x2f, 1 } /* { */, { 0x31, 1 } /* | */, { 0x30, 1 } /* } */,
    { 0x35, 1 } /* ~ */,
};

// encode a single character for a HID report
// input: l, a printable char to encode
// output: o, a single uint8_t
// returns: 0 normally, 1 if character requires shift
// corner case: if non-printable char is input, will encode ' ' (space)
static int encode_char(char l, uint8_t *o) {
    int shift = 0;
    enc_t *e;
    if (l >= 0x20 && l <= 0x40) {
        e = &enc_20[l-0x20];
        shift = e->shift;
        *o = e->val;
    } else if (l >= 0x5b && l <= 0x60) {
        e = &enc_5b[l-0x5b];
        shift = e->shift;
        *o = e->val;
    } else if (l >= 0x7b && l <= 0x7e) {
        e = &enc_7b[l-0x7b];
        shift = e->shift;
        *o = e->val;
    } else if (isalpha(l)) { // a letter
        if (isupper(l)) {
            shift = 1;
            l = tolower(l);
        }
        *o = l - 'a' + 0x04;
    } else { // TODO error? encode a space i guess
        return encode_char(' ', o);
    }
    return shift;
}

// encode a keystroke into an 8-byte HID report
void hid_encode(keystroke_t *s, uint8_t *r) {
    int mod = s->mod;
    int shift;
    char l;
    enc_t *e;

    memset(r, 0, 8);

    switch (s->type) {
        case K_CHAR:
            shift = encode_char(s->chr, &r[2]);
            if (shift)
                mod |= M_SHIFT;
            break;
        case K_ENTER:
            r[2] = 0x28;
            break;
        case K_TAB:
            r[2] = 0x2b;
            break;
        case K_ESC:
            r[2] = 0x29;
            break;
        case K_BACK:
            r[2] = 0x2A;
            break;
        case K_RAW:
            r[2] = s->chr;
            break;
    }
    r[0] = mod;
}
