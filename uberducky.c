/*
 * Copyright 2019 Mike Ryan
 *
 * This file is part of Uberducky and is released under the terms of the
 * GPL version 2. Refer to COPYING for more information.
 */

#include "hid.h"
#include "type.h"
#include "ubertooth.h"
#include "usbapi.h"

#include <string.h>

#define INTR_IN_EP      0x81

#define LE_WORD(x)      ((x)&0xFF),((x)>>8)
#define READ_LE(x)      ((script[x+1] << 8) | script[x])

#define NOW T0TC

// script state
#define ST_IDLE     0
#define ST_READY    1
#define ST_RUNNING  2

// run state
#define R_IDLE      0
#define R_KEY_DOWN  1
#define R_DELAY     2
#define R_STRING    3

// string state
#define S_IDLE      0
#define S_KEY_DOWN  1

int script_state = ST_IDLE;
int run_state = R_IDLE;
int string_state = S_IDLE;

unsigned script_len = 0;
unsigned script_pos = 0;
unsigned string_len = 0;
unsigned string_pos = 0;

// opcodes
//
// encoding: <op> [<arg> .. ]
// 
// NOP - no args
// KEY - key type, modifier, character (each 1 byte)
// DELAY - delay in ms (16 bit little endian)
// STRING - length (16 bit little endian), chars
#define OP_NOP    0
#define OP_KEY    1
#define OP_DELAY  2
#define OP_STRING 3

#define DELAY(X) OP_DELAY, LE_WORD(X)

// demo script - print hello world
uint8_t script[] = {
    LE_WORD(24),  // script len
    OP_STRING,    // string
    LE_WORD(6),   // string len
    'h', 'e', 'l', 'l', 'o', ' ',
    DELAY(1000),  // delay
    OP_STRING,    // string
    LE_WORD(5),
    'w', 'o', 'r', 'l', 'd',
    OP_KEY,       // key - enter with no modifier
    K_ENTER, 0x00, 0x00,
};

static void timer0_start(void) {
    T0TCR = TCR_Counter_Reset;
    T0PR = 50000 - 1; // 1 ms
    T0TCR = TCR_Counter_Enable;

    // set up interrupt handler
    ISER0 = ISER0_ISE_TIMER0;
}

static void timer0_set_match(uint32_t match) {
    T0MR0 = match;
    T0MCR |= TMCR_MR0I;
}

static void timer0_clear_match(void) {
    T0MCR &= ~TMCR_MR0I;
}

void TIMER0_IRQHandler(void) {
    uint8_t report[8] = { 0, };
    keystroke_t next_key = { 0, };

    if (T0IR & TIR_MR0_Interrupt) {
        // ack the interrupt
        T0IR = TIR_MR0_Interrupt;

        if (script_state == ST_READY) {
            script_pos = 0;
            script_state = ST_RUNNING;
            run_state = R_IDLE;

            script_len = READ_LE(script_pos);
            script_pos += 2;

            // re-enter in 1 ms
            timer0_set_match(NOW + 1);
        } else if (script_state == ST_RUNNING) {
            uint8_t opcode;
            uint16_t delay;
            switch (run_state) {
                // idle -- get next opcode
                case R_IDLE:
                    if (script_pos >= script_len) {
                        script_state = ST_IDLE;
                        return;
                    }
                    opcode = script[script_pos++];
                    switch (opcode) {
                        case OP_NOP:
                            ++script_pos;
                            timer0_set_match(NOW + 1); // re-enter in 1 ms
                            return;

                        case OP_KEY:
                            // TODO bounds check
                            next_key.type = script[script_pos++];
                            next_key.mod = script[script_pos++];
                            next_key.chr = script[script_pos++];

                            // encode and inject key
                            hid_encode(&next_key, report);
                            USBHwEPWrite(INTR_IN_EP, report, 8);
                            run_state = R_KEY_DOWN;
                            timer0_set_match(NOW + DOWN_TIME);
                            return;

                        case OP_DELAY:
                            // TODO bounds check
                            delay = READ_LE(script_pos);
                            script_pos += 2;
                            run_state = R_DELAY;
                            timer0_set_match(NOW + delay);
                            return;

                        case OP_STRING:
                            // TODO bounds check
                            string_len = READ_LE(script_pos);
                            string_pos = 0;
                            script_pos += 2;
                            run_state = R_STRING;
                            string_state = S_IDLE;
                            timer0_set_match(NOW + 1); // re-enter in 1 ms
                            return;
                    }
                    return;

                // key down - lift key
                case R_KEY_DOWN:
                    // all keys up
                    USBHwEPWrite(INTR_IN_EP, report, 8);
                    timer0_set_match(NOW + DOWN_TIME);
                    run_state = R_IDLE;
                    // timer0_set_match(NOW + 1);
                    return;

                case R_DELAY:
                    run_state = R_IDLE;
                    timer0_set_match(NOW + 1);
                    return;

                // string - sub state machine
                case R_STRING:
                    switch (string_state) {
                        // idle - get next key
                        case S_IDLE:
                            // end of string, next
                            if (string_pos >= string_len) {
                                script_pos += string_len;
                                run_state = R_IDLE;
                                timer0_set_match(NOW + 1);
                                return;
                            }

                            next_key.type = K_CHAR;
                            next_key.mod = 0;
                            next_key.chr = script[script_pos + string_pos];
                            ++string_pos;

                            hid_encode(&next_key, report);
                            USBHwEPWrite(INTR_IN_EP, report, 8);
                            string_state = S_KEY_DOWN;
                            timer0_set_match(NOW + DOWN_TIME);
                            return;

                        // key down - lift and go back to idle state
                        case S_KEY_DOWN:
                            USBHwEPWrite(INTR_IN_EP, report, 8);
                            timer0_set_match(NOW + DOWN_TIME);
                            string_state = S_IDLE;
                            return;
                    }
                    return;
            }
        }
    }
}

// from usb.c
void usb_init(void);

int main() {
    ubertooth_init();

    timer0_start();

    script_state = ST_READY;
    timer0_set_match(NOW + 2000);

    usb_init();

    // call USB interrupt handler continuously
    while (1) {
        USBHwISR();
    }

    return 0;
}
