#ifndef __BLE_H__
#define __BLE_H__

#include <stdint.h>

#define BLE_PACKET_SIZE 32

void ble_init(void);
int ble_get_packet(uint8_t *pkt);

#endif /* __BLE_H__ */
