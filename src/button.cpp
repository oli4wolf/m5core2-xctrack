#include "button.h"
#include <M5Unified.h>
#include "esp_log.h"
#include "gui.h"
#include <algorithm>

// External global variables
extern bool globalSoundEnabled;
extern int32_t globalBatteryLevel;
extern bool globalChargingState;

/**
 * @brief Handles button input from M5Stack buttons
 * 
 * Button A: Toggle sound on/off
 * Button B: Increase volume
 * Button C: Decrease volume
 */
void handleButtonInput()
{
  // Button A: Sound toggle
  if (M5.BtnA.wasPressed())
  {
    globalSoundEnabled = !globalSoundEnabled;
    drawFooter(globalSoundEnabled);
    ESP_LOGI("button", "Sound toggled: %s", globalSoundEnabled ? "ON" : "MUTED");
  }
  
  // Button B: Volume up
  if (M5.BtnB.wasPressed())
  {
    uint8_t currentVolume = M5.Speaker.getVolume();
    uint8_t newVolume = std::min(VOLUME_MAX, static_cast<uint8_t>(currentVolume + VOLUME_STEP));
    M5.Speaker.setVolume(newVolume);
    ESP_LOGI("button", "Volume increased: %d -> %d", currentVolume, newVolume);
  }
  
  // Button C: Volume down
  if (M5.BtnC.wasPressed())
  {
    uint8_t currentVolume = M5.Speaker.getVolume();
    // Prevent underflow by checking before subtraction
    uint8_t newVolume = (currentVolume > VOLUME_STEP) 
                        ? (currentVolume - VOLUME_STEP) 
                        : VOLUME_MIN;
    M5.Speaker.setVolume(newVolume);
    ESP_LOGI("button", "Volume decreased: %d -> %d", currentVolume, newVolume);
  }
}
