#ifndef BLEUART_H_
#define BLEUART_H_

#include <stdint.h>

// BLE Connection State
enum BLEConnectionState {
    BLE_DISCONNECTED,  // Not initialized or disconnected
    BLE_CONNECTING,    // Attempting initialization
    BLE_CONNECTED,     // Successfully connected and advertising
    BLE_FAILED         // Last initialization attempt failed
};

bool ble_uart_init();
bool ble_uart_transmit_LK8EX1(int32_t altm, int32_t cps, int32_t batteryLevel);
void ble_task(void *pvParameter);
BLEConnectionState getBLEState();

#endif