#ifndef GUI_H_
#define GUI_H_

#include <M5Unified.h>
#include "ble_uart.h"  // For BLEConnectionState enum
#include "power.h"     // For BatteryState struct

// Display initialization functions
void startupScreen();
void initializeDisplay();

// Display update functions
void drawHeader(const BatteryState& batteryState, float altitude, BLEConnectionState bleState);
void drawMainDisplay(float vSpeed);
void drawFooter(bool soundEnabled);

#endif
