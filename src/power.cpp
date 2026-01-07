#include "power.h"
#include <M5Unified.h>
#include "esp_log.h"

// External global variables defined in main.cpp
extern int32_t globalBatteryLevel;
extern bool globalChargingState;
extern bool globalUsbPowerState;

void updateBatteryStatus()
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
  static int32_t lastValidBatteryLevel = -1;
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
             voltage, zeroReadingCount, totalReadings, lastValidBatteryLevel);
    
    // Use last known good value instead of the invalid 0
    if (lastValidBatteryLevel >= 0) {
      batteryLevel = lastValidBatteryLevel;
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
  
  // Update last valid reading if this one is valid
  if (batteryLevel > 0 || !isInvalidZero) {
    lastValidBatteryLevel = batteryLevel;
  }
  
  // Log battery level changes and filtering stats
  if (batteryLevel != globalBatteryLevel) {
    ESP_LOGI("Battery", "Battery level: %ld%% (Charging: %d, Voltage: %.0fmV, Filtered: %lu/%lu)",
             batteryLevel, isCharging, voltage, filteredReadingCount, totalReadings);
  }
  
  // Update global state with validated reading
  globalBatteryLevel = batteryLevel;
  globalChargingState = isCharging;      // TRUE only when actively charging
  globalUsbPowerState = usbConnected;    // TRUE when USB power is present
}
