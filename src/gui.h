#ifndef GUI_H_
#define GUI_H_

#include <M5Unified.h>
#include "ble_uart.h"  // For BLEConnectionState enum

// Global LCD object - defined in gui.cpp
extern M5GFX lcd;

// Display initialization functions
void startupScreen();
void initializeDisplay();

// Display update functions
void drawHeader(int32_t battery, float altitude, BLEConnectionState bleState);
void drawMainDisplay(float vSpeed);
void drawFooter(bool soundEnabled);

#endif
