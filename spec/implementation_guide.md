# Display Redesign - Implementation Guide

## Summary
This guide provides step-by-step instructions for implementing the new display layout focusing on large vertical speed display with compact header information.

## Implementation Steps

### Step 1: Update config.h
Add the following constants at the end of the file:

```cpp
// Display Layout Constants
const int DISPLAY_UPDATE_INTERVAL_MS = 500; // 2 Hz update rate (faster than current 1Hz)
const int HEADER_HEIGHT = 40;
const int FOOTER_HEIGHT = 30;
const int MAIN_DISPLAY_HEIGHT = 170;
const int DISPLAY_WIDTH = 320;
const int DISPLAY_HEIGHT = 240;

// Vertical Speed Color Thresholds
const float VSPEED_CLIMB_THRESHOLD = 0.3;   // m/s - Green when above
const float VSPEED_SINK_THRESHOLD = -0.3;   // m/s - Red when below
// Between these values: Yellow (neutral)

// Display Colors (RGB565 format)
const uint16_t HEADER_BG_COLOR = 0x1A0E;    // Dark blue-gray
const uint16_t FOOTER_BG_COLOR = 0x2A2E;    // Dark gray
const uint16_t MUTED_GRAY_COLOR = 0x8410;   // Gray for muted status

// Update Thresholds (prevent flicker from small changes)
const float ALTITUDE_UPDATE_THRESHOLD = 1.0;  // meters
const float BATTERY_UPDATE_THRESHOLD = 0.1;   // volts
const float VSPEED_UPDATE_THRESHOLD = 0.05;   // m/s

// Text Sizes
const int HEADER_TEXT_SIZE = 2;
const int MAIN_TEXT_SIZE = 10;  // Very large for vertical speed
const int FOOTER_TEXT_SIZE = 2;
```

### Step 2: Update main.cpp - Add Display Functions

Add these function declarations before `setup()`:

```cpp
// Display function declarations
void drawHeader(float battery, float altitude);
void drawMainDisplay(float vSpeed);
void drawFooter(bool soundEnabled);
void initializeDisplay();
```

Add these function implementations after `loop()`:

#### initializeDisplay()
```cpp
void initializeDisplay() {
    lcd.fillScreen(TFT_BLACK);
    
    // Draw static header background
    lcd.fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, HEADER_BG_COLOR);
    
    // Draw static footer background
    lcd.fillRect(0, DISPLAY_HEIGHT - FOOTER_HEIGHT, DISPLAY_WIDTH, FOOTER_HEIGHT, FOOTER_BG_COLOR);
    
    // Initial footer with sound status
    drawFooter(globalSoundEnabled);
    
    ESP_LOGI("Display", "Display initialized with new layout");
}
```

#### drawHeader()
```cpp
void drawHeader(float battery, float altitude) {
    // Clear header area (maintains background color)
    lcd.fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, HEADER_BG_COLOR);
    
    // Set text properties for header
    lcd.setTextSize(HEADER_TEXT_SIZE);
    lcd.setTextColor(TFT_WHITE, HEADER_BG_COLOR);
    
    // Draw battery on left side
    lcd.setCursor(5, 12);
    lcd.printf("Bat: %.1fV", battery);
    
    // Draw altitude on right side (right-aligned)
    char altStr[20];
    snprintf(altStr, sizeof(altStr), "Alt: %.1fm", altitude);
    int textWidth = lcd.textWidth(altStr);
    lcd.setCursor(DISPLAY_WIDTH - textWidth - 5, 12);
    lcd.print(altStr);
}
```

#### drawMainDisplay()
```cpp
void drawMainDisplay(float vSpeed) {
    // Determine color based on vertical speed
    uint16_t color;
    if (vSpeed > VSPEED_CLIMB_THRESHOLD) {
        color = TFT_GREEN;  // Climbing
    } else if (vSpeed < VSPEED_SINK_THRESHOLD) {
        color = TFT_RED;    // Sinking
    } else {
        color = TFT_YELLOW; // Neutral
    }
    
    // Clear main display area
    lcd.fillRect(0, HEADER_HEIGHT, DISPLAY_WIDTH, MAIN_DISPLAY_HEIGHT, TFT_BLACK);
    
    // Set text properties for main display
    lcd.setTextSize(MAIN_TEXT_SIZE);
    lcd.setTextColor(color, TFT_BLACK);
    
    // Format vertical speed string with sign
    char vSpeedStr[20];
    if (vSpeed >= 0) {
        snprintf(vSpeedStr, sizeof(vSpeedStr), "+%.1f m/s", vSpeed);
    } else {
        snprintf(vSpeedStr, sizeof(vSpeedStr), "%.1f m/s", vSpeed);
    }
    
    // Calculate center position
    int textWidth = lcd.textWidth(vSpeedStr);
    int textHeight = lcd.fontHeight();
    int x = (DISPLAY_WIDTH - textWidth) / 2;
    int y = HEADER_HEIGHT + (MAIN_DISPLAY_HEIGHT - textHeight) / 2;
    
    // Draw vertical speed
    lcd.setCursor(x, y);
    lcd.print(vSpeedStr);
}
```

#### drawFooter()
```cpp
void drawFooter(bool soundEnabled) {
    // Clear footer area
    lcd.fillRect(0, DISPLAY_HEIGHT - FOOTER_HEIGHT, DISPLAY_WIDTH, FOOTER_HEIGHT, FOOTER_BG_COLOR);
    
    // Set text properties
    lcd.setTextSize(FOOTER_TEXT_SIZE);
    
    // Draw speaker icon (simple representation: [🔊] or [🔇])
    lcd.setCursor(5, DISPLAY_HEIGHT - FOOTER_HEIGHT + 8);
    if (soundEnabled) {
        lcd.setTextColor(TFT_WHITE, FOOTER_BG_COLOR);
        lcd.print("[");
        lcd.print((char)0x0F); // Musical note character or use "SPK"
        lcd.print("] ON");
    } else {
        lcd.setTextColor(MUTED_GRAY_COLOR, FOOTER_BG_COLOR);
        lcd.print("[X] MUTED");
    }
}
```

### Step 3: Update setup() Function

Replace the `gui()` call (or remove it) and add:

```cpp
void setup() {
    // ... existing setup code ...
    
    initializeVariometerTask();
    initializeBLE();
    
    // Initialize new display layout
    initializeDisplay();
}
```

### Step 4: Replace loop() Function

Replace the entire `loop()` function with:

```cpp
void loop() {
    static unsigned long lastDisplayUpdate = 0;
    static float prevBattery = 0;
    static float prevAltitude = 0;
    static float prevVSpeed = 0;
    
    unsigned long currentMillis = millis();
    
    // Check for button press (sound toggle)
    M5.update();
    if (M5.BtnA.wasPressed()) {
        globalSoundEnabled = !globalSoundEnabled;
        drawFooter(globalSoundEnabled);
        ESP_LOGI("main.cpp", "Sound toggled: %s", globalSoundEnabled ? "ON" : "MUTED");
    }
    
    // Display update at configured interval
    if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
        float pressure = 0.0f;
        float temperature = 0.0f;
        float altitude = 0.0f;
        float verticalSpeed = 0.0f;
        float battery = M5.Power.getBatteryVoltage();
        
        // Read sensor data with mutex protection
        if (xSemaphoreTake(xSensorMutex, portMAX_DELAY) == pdTRUE) {
            pressure = globalPressure;
            temperature = globalTemperature;
            xSemaphoreGive(xSensorMutex);
        }
        
        // Read variometer data with mutex protection
        if (xSemaphoreTake(xVariometerMutex, portMAX_DELAY) == pdTRUE) {
            altitude = globalAltitude_m;
            verticalSpeed = globalVerticalSpeed_mps;
            xSemaphoreGive(xVariometerMutex);
        }
        
        // Update header if battery or altitude changed significantly
        if (abs(battery - prevBattery) >= BATTERY_UPDATE_THRESHOLD ||
            abs(altitude - prevAltitude) >= ALTITUDE_UPDATE_THRESHOLD) {
            drawHeader(battery, altitude);
            prevBattery = battery;
            prevAltitude = altitude;
        }
        
        // Always update main display (vertical speed changes frequently)
        if (abs(verticalSpeed - prevVSpeed) >= VSPEED_UPDATE_THRESHOLD) {
            drawMainDisplay(verticalSpeed);
            prevVSpeed = verticalSpeed;
        }
        
        // Log for debugging (keep existing log)
        ESP_LOGI("main.cpp", "Altitude: %.1f, V-Speed: %.2f, Pressure: %.1f, Temperature: %.2f, Battery: %.2f V", 
                 altitude, verticalSpeed, pressure, temperature, battery);
        
        lastDisplayUpdate = currentMillis;
    }
    
    // Short delay to check buttons frequently while not blocking
    vTaskDelay(pdMS_TO_TICKS(50));
}
```

### Step 5: Remove Obsolete Code

Remove or comment out:
- `void gui()` function (lines 139-152) - no longer needed
- The `gui()` call in setup if it exists

## Testing Procedure

After implementation, test in this order:

1. **Compilation Check**
   - Build the project
   - Fix any compilation errors
   - Upload to device

2. **Display Layout Test**
   - Verify header shows battery and altitude
   - Verify main area shows large vertical speed
   - Verify footer shows sound status

3. **Color Coding Test**
   - Blow on sensor to create pressure change
   - Verify green color when climbing (>0.3 m/s)
   - Verify red color when sinking (<-0.3 m/s)
   - Verify yellow color when neutral

4. **Button Test**
   - Press left button (BtnA)
   - Verify footer updates to show MUTED
   - Press again to verify it returns to ON
   - Verify sound actually stops/starts

5. **Update Rate Test**
   - Observe display updates
   - Should update approximately every 500ms (2 Hz)
   - No visible flicker

6. **Accuracy Test**
   - Compare displayed values with serial log
   - Verify formatting is correct
   - Verify decimal places match specification

## Troubleshooting

### Text Size Too Large/Small
- Adjust `MAIN_TEXT_SIZE` constant in config.h
- Test values between 8-12 for best fit

### Flicker Issues
- Increase update thresholds in config.h
- Ensure only changed areas are redrawn

### Button Not Responsive
- Check `M5.update()` is called in loop
- Verify button hardware is working

### Colors Not Showing Correctly
- Verify RGB565 color values
- Check TFT color constants usage

### Display Alignment Issues
- Verify text width/height calculations
- Check cursor positioning math
- Adjust padding values

## Performance Notes

- The new 500ms update rate is 2x faster than before
- Button polling at 50ms provides responsive feedback
- Selective updates minimize LCD operations
- No impact on sensor or BLE tasks (run on APP_CPU_NUM)

## Code Modifications Summary

| File | Lines Changed | Type |
|------|---------------|------|
| src/config.h | ~30 lines added | New constants |
| src/main.cpp | ~150 lines modified | Functions + loop rewrite |

Total estimated changes: ~180 lines

## Next Steps After Implementation

Once the code is working:
1. Test in actual flight conditions (or simulated)
2. Fine-tune color thresholds based on user preference
3. Adjust text sizes if needed for readability
4. Consider adding battery percentage calculation
5. Add speaker icon graphic (currently using text)

## Reference Documents
- [`spec/display_redesign.md`](spec/display_redesign.md) - Full design specification
- [`spec/requirements.md`](spec/requirements.md) - Original requirements
- M5GFX documentation: https://github.com/m5stack/M5GFX