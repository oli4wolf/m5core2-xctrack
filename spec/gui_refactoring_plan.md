# GUI Module Refactoring Plan

## Overview

This document outlines the plan to extract all graphical/display methods from `src/main.cpp` into a separate `src/gui.cpp` module with corresponding header `src/gui.h`.

## Identified Graphical Methods to Move

| Function | Current Lines | Description |
|----------|---------------|-------------|
| `startupScreen()` | 116-130 | Draws initial boot screen with version info |
| `initializeDisplay()` | 219-234 | Sets up display layout and backgrounds |
| `drawHeader()` | 236-277 | Renders battery, altitude, BLE status |
| `drawMainDisplay()` | 279-325 | Renders vertical speed with color coding |
| `drawFooter()` | 327-347 | Renders sound status indicator |

## Global Variable to Relocate

- `M5GFX lcd` (line 18) - The LCD display object will be defined in `gui.cpp` and declared as `extern` in `gui.h`

## New Files Structure

### gui.h (new file)

```cpp
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
```

### gui.cpp (new file)

Dependencies:
- `gui.h`
- `<M5Unified.h>`
- `"config.h"` - for display constants
- `"ble_uart.h"` - for BLEConnectionState
- `"esp_log.h"` - for logging macros

Contents:
- Definition: `M5GFX lcd;`
- All 5 function implementations moved from main.cpp

### main.cpp (modifications)

Changes required:
- **Add**: `#include "gui.h"`
- **Remove**: `M5GFX lcd;` declaration (line 18)
- **Remove**: Function declarations (lines 34-37)
- **Remove**: `startupScreen()` implementation (lines 116-130)
- **Remove**: Display function implementations section (lines 215-347)

Functions that **remain** in main.cpp:
- `initializeBLE()` - BLE task initialization
- `initializeSensorTask()` - Sensor task initialization  
- `initializeVariometerTask()` - Variometer task initialization
- `initializeM5Stack()` - M5Stack hardware initialization
- `setup()` - Main setup function
- `loop()` - Main loop function

## Dependencies Diagram

```
main.cpp
    │
    ├── includes: gui.h
    │                │
    │                ├── extern M5GFX lcd
    │                └── function declarations
    │
    └── calls: startupScreen(), initializeDisplay(),
               drawHeader(), drawMainDisplay(), drawFooter()

gui.cpp
    │
    ├── includes: gui.h
    ├── includes: M5Unified.h
    ├── includes: config.h (display constants)
    ├── includes: ble_uart.h (BLEConnectionState)
    ├── includes: esp_log.h (logging)
    │
    ├── defines: M5GFX lcd
    └── implements: all 5 graphical functions
```

## Notes

1. **`initializeM5Stack()`** remains in `main.cpp` because it handles overall M5Stack hardware initialization (serial, IMU, RTC, speaker), not just display. The `lcd.init()` call inside it will work via the extern declaration.

2. The `globalSoundEnabled` variable is used by `drawFooter()` but is passed as a parameter, so no additional extern declarations are needed.

3. Display constants from `config.h`:
   - `DISPLAY_WIDTH`, `DISPLAY_HEIGHT`
   - `HEADER_HEIGHT`, `FOOTER_HEIGHT`, `MAIN_DISPLAY_HEIGHT`
   - `HEADER_BG_COLOR`, `FOOTER_BG_COLOR`, `MUTED_GRAY_COLOR`
   - `VSPEED_CLIMB_THRESHOLD`, `VSPEED_SINK_THRESHOLD`
   - `HEADER_TEXT_SIZE`, `MAIN_TEXT_SIZE`, `FOOTER_TEXT_SIZE`
   - `BLE_SHOW_STATUS_ON_DISPLAY`

## Implementation Order

1. Create `src/gui.h` header file
2. Create `src/gui.cpp` with all implementations
3. Update `src/main.cpp` to remove moved code and include gui.h
4. Build and verify compilation succeeds
