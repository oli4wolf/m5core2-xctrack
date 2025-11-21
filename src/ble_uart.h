#ifndef BLEUART_H_
#define BLEUART_H_

#include <stdint.h>

void ble_uart_init();
void ble_uart_transmit_LK8EX1(int32_t altm, int32_t cps, float batVoltage);
void ble_task(void *pvParameter);

#endif