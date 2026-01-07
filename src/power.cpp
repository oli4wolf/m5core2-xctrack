#include "power.h"
#include <M5Unified.h>
#include "esp_log.h"

BatteryState updateBatteryStatus()
{
  int32_t batteryLevel = M5.Power.getBatteryLevel();
  bool isCharging = M5.Power.isCharging();
  float voltage = M5.Power.getBatteryVoltage();
  int32_t batteryCurrent = M5.Power.getBatteryCurrent();
  
  // Detect USB/external power presence:
  // USB is connected when:
  // 1. Battery is actively charging (isCharging == true), OR
  // 2. Battery voltage is high (>4.0V) AND current is non-negative (not discharging)
  //    This covers the case when battery is full and not actively charging
  bool usbConnected = isCharging || (voltage > 4000.0f && batteryCurrent >= 0);
  
  // Log battery reading details for debugging
  static BatteryState lastValidState = {-1, false, false};
  static uint32_t zeroReadingCount = 0;
  static uint32_t filteredReadingCount = 0;
  static uint32_t totalReadings = 0;
  
  totalReadings++;
  
  // Validate the reading: If battery level is 0 but voltage is high, the API is giving bad data
  // A battery at 0% would have voltage < 3.0V. If voltage > 3.0V, reading is invalid.
  bool isInvalidZero = (batteryLevel == 0 && voltage > 3000.0f);
  
  if (isInvalidZero) {
    zeroReadingCount++;
    filteredReadingCount++;
    ESP_LOGW("Battery", "Invalid zero reading filtered! Voltage: %.0fmV indicates battery not empty (Count: %lu/%lu, Using last valid: %ld%%)",
             voltage, zeroReadingCount, totalReadings, lastValidState.level);
    
    // Use last known good value instead of the invalid 0
    if (lastValidState.level >= 0) {
      batteryLevel = lastValidState.level;
    } else {
      // No valid reading yet, estimate from voltage (rough approximation)
      // LiPo: 4.2V=100%, 3.7V=50%, 3.0V=0%
      if (voltage >= 4100.0f) batteryLevel = 100;
      else if (voltage >= 3700.0f) batteryLevel = 50;
      else if (voltage >= 3300.0f) batteryLevel = 20;
      else batteryLevel = 5;
      ESP_LOGI("Battery", "Estimated battery level from voltage: %ld%%", batteryLevel);
    }
  } else if (batteryLevel == 0) {
    // Genuine low battery (voltage confirms it)
    zeroReadingCount++;
    ESP_LOGW("Battery", "Genuine low battery detected! Voltage: %.0fmV, Level: 0%%", voltage);
  }
  
  // Validate and clamp battery level to valid range [0, 100]
  int32_t validatedLevel = batteryLevel;
  if (validatedLevel < 0) {
    ESP_LOGW("Battery", "Battery level out of range (%ld), clamping to 0", validatedLevel);
    validatedLevel = 0;
  } else if (validatedLevel > 100) {
    ESP_LOGW("Battery", "Battery level out of range (%ld), clamping to 100", validatedLevel);
    validatedLevel = 100;
  }
  
  // Validate state consistency: charging implies USB connected
  // This catches hardware/firmware issues
  if (isCharging && !usbConnected) {
    ESP_LOGW("Battery", "Inconsistent state detected: charging=%d but USB=%d. Correcting USB state.",
             isCharging, usbConnected);
    usbConnected = true; // If charging, USB must be connected
  }
  
  // Create current battery state
  BatteryState currentState;
  currentState.level = validatedLevel;
  currentState.isCharging = isCharging;
  currentState.usbConnected = usbConnected;
  
  // Detect comprehensive state changes (not just battery level)
  bool stateChanged = (currentState.level != lastValidState.level) ||
                      (currentState.isCharging != lastValidState.isCharging) ||
                      (currentState.usbConnected != lastValidState.usbConnected);
  
  // Log state changes with all relevant information
  if (stateChanged) {
    ESP_LOGI("Battery", "State change: Level=%ld%%, Charging=%d, USB=%d, Voltage=%.0fmV, Filtered=%lu/%lu",
             currentState.level, currentState.isCharging, currentState.usbConnected,
             voltage, filteredReadingCount, totalReadings);
  }
  
  // Update last valid reading if this one is valid
  if (batteryLevel > 0 || !isInvalidZero) {
    lastValidState = currentState;
  }
  
  return currentState;
}
