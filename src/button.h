#ifndef BUTTON_H
#define BUTTON_H

#include <cstdint>

// Constants for button handling
constexpr uint8_t VOLUME_STEP = 16;
constexpr uint8_t VOLUME_MAX = 255;
constexpr uint8_t VOLUME_MIN = 0;
constexpr int32_t BATTERY_LEVEL_MIN_VALID = 1;

/**
 * @brief Handles button input from M5Stack buttons
 * 
 * Button A: Toggle sound on/off
 * Button B: Increase volume
 * Button C: Decrease volume
 * 
 * @note Requires M5.update() to be called before this function
 */
void handleButtonInput();

#endif // BUTTON_H
