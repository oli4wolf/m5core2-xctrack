#ifndef POWER_H
#define POWER_H

#include <stdint.h>

/**
 * @brief Battery state structure containing all battery-related information
 */
struct BatteryState {
    int32_t level;        // Battery level 0-100%
    bool isCharging;      // TRUE when actively charging battery
    bool usbConnected;    // TRUE when USB/external power is present
};

/**
 * @brief Reads and validates battery status from hardware
 * @return BatteryState structure with validated battery information
 * 
 * This function reads battery level, charging state, voltage, and current.
 * It filters invalid readings (e.g., spurious 0% readings when voltage is high)
 * and returns a validated battery state.
 */
BatteryState updateBatteryStatus();

#endif // POWER_H
