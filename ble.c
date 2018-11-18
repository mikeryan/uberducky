/*
 * Copyright 2019 Mike Ryan
 *
 * This file is part of Uberducky and is released under the terms of the
 * GPL version 2. Refer to COPYING for more information.
 */

#include "ble.h"

#include "ubertooth.h"

// advertising channel 38
uint16_t rf_channel = 2426;

// initialize RF and strobe FSON
static void cc2400_init_rf(void) {
    u16 grmdm, mdmctrl;
    uint32_t sync = rbit(0x8e89bed6);

    mdmctrl = 0x0040; // 250 kHz frequency deviation
    grmdm = 0x4CE1; // un-buffered mode, packet w/ sync word detection
    // 0 10 01 1 001 11 0 00 0 1
    //   |  |  | |   |  +--------> CRC off
    //   |  |  | |   +-----------> sync word: 32 MSB bits of SYNC_WORD
    //   |  |  | +---------------> 1 preamble byte of 01010101
    //   |  |  +-----------------> packet mode
    //   |  +--------------------> buffered mode
    //   +-----------------------> sync error bits: 2

    cc2400_set(MANAND,  0x7ffe);
    cc2400_set(LMTST,   0x2b22);

    cc2400_set(MDMTST0, 0x124b);
    // 1      2      4b
    // 00 0 1 0 0 10 01001011
    //    | | | | |  +---------> AFC_DELTA = ??
    //    | | | | +------------> AFC settling = 4 pairs (8 bit preamble)
    //    | | | +--------------> no AFC adjust on packet
    //    | | +----------------> do not invert data
    //    | +------------------> TX IF freq 1 0Hz
    //    +--------------------> PRNG off
    //
    // ref: CC2400 datasheet page 67
    // AFC settling explained page 41/42

    cc2400_set(GRMDM,   grmdm);

    cc2400_set(SYNCL,   sync & 0xffff);
    cc2400_set(SYNCH,   (sync >> 16) & 0xffff);

    cc2400_set(FSDIV,   rf_channel - 1); // 1 MHz IF
    cc2400_set(MDMCTRL, mdmctrl);

    // XOSC16M should always be stable, but leave this test anyway
    while (!(cc2400_status() & XOSC16M_STABLE));

    // wait for FS_LOCK
    cc2400_strobe(SFSON);
    while (!(cc2400_status() & FS_LOCK)) ;
}

// dewhiten and reverse the bit order of a packet in place
static void ble_dewhiten(uint8_t *pkt, unsigned len) {
    unsigned i;
    uint32_t *pkt_out = (void *)pkt;

    uint32_t dewhiten[] = {
        0x2044c5d6, 0x8fe1de59, 0x42afa51b, 0x60cd4e7b, 0x902262eb, 0xc7f0ef2c,
        0xa157d28d, 0xb066a73d, 0x48113175, 0xe3f87796, 0xd0abe946, 0xd833539e,
    };

    for (i = 0; i < len; i+= 4) {
        uint32_t v = pkt[i+0] << 24
                   | pkt[i+1] << 16
                   | pkt[i+2] << 8
                   | pkt[i+3] << 0;
        pkt_out[i/4] = rbit(v) ^ dewhiten[i/4];
    }
}

void ble_init(void) {
    cc2400_strobe(SRFOFF);
    while (cc2400_status() & FS_LOCK) ;

    cc2400_init_rf();
    cc2400_strobe(SRX);
}

int ble_get_packet(uint8_t *pkt) {
    unsigned i;

    // when the FIFO is full the radio state returns to FS_ON
    if ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON)
        return 0;

    // load all bytes into buffer
    for (i = 0; i < BLE_PACKET_SIZE; ++i)
        pkt[i] = cc2400_get8(FIFOREG);

    // dewhiten
    ble_dewhiten(pkt, BLE_PACKET_SIZE);

    // restart RF
    while (!(cc2400_status() & FS_LOCK)) ;
    cc2400_strobe(SRX);
    while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_RX) ;

    return 1;
}
