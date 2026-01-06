#ifndef POWER_H
#define POWER_H

#include <stdint.h>

/**
 * @brief Updates the battery status by reading current battery level,
 *        charging state, and voltage. Filters invalid readings and
 *        updates global battery state variables.
 */
void updateBatteryStatus();

#endif // POWER_H
