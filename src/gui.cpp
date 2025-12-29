#include "gui.h"
#include <M5Unified.h>
#include "config.h"
#include "ble_uart.h"
#include "esp_log.h"

// Global LCD object definition
M5GFX lcd;

// =============================================================================
// DISPLAY FUNCTION IMPLEMENTATIONS
// =============================================================================

void startupScreen()
{
  lcd.fillScreen(TFT_BLACK);
  lcd.setCursor(0, 0);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(1);
  lcd.println("M5Stack Core2 XCTrack");
  lcd.println("v0.1");
  lcd.println("by @oli4wolf on github");
  lcd.println("This is a non-commercial project.");
  int32_t batteryLevel = M5.Power.getBatteryLevel();
  lcd.printf("Battery Level: %d %%\n", batteryLevel);
  ESP_LOGI("Display", "Startup screen drawn, battery=%d%%, waiting 5s", batteryLevel);
  delay(5000);
}

void initializeDisplay()
{
  // Ensure display is on and bright
  lcd.wakeup();
  lcd.setBrightness(255);
  lcd.fillScreen(TFT_BLACK);
  // Draw static header background
  lcd.fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, HEADER_BG_COLOR);

  // Draw static footer background
  lcd.fillRect(0, DISPLAY_HEIGHT - FOOTER_HEIGHT, DISPLAY_WIDTH, FOOTER_HEIGHT, FOOTER_BG_COLOR);

  // Initial footer with sound status
  extern bool globalSoundEnabled;
  drawFooter(globalSoundEnabled);
}

void drawHeader(int32_t battery, bool charging, float altitude, BLEConnectionState bleState)
{
  ESP_LOGI("Display", "drawHeader called - battery=%d, altitude=%.1fm charging=%s", battery, altitude, charging ? "true" : "false");
  // Clear header area (maintains background color)
  lcd.fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, HEADER_BG_COLOR);

  // Set text properties for header
  lcd.setTextSize(HEADER_TEXT_SIZE);
  lcd.setTextColor(TFT_WHITE, HEADER_BG_COLOR);

  // Draw battery on left side
  lcd.setCursor(5, 12);
  if( charging ) {
    lcd.printf("%d%% (C)", battery);
  } else {
    lcd.printf("%d%%", battery);
  }

  // Draw BLE status indicator if enabled
  if (BLE_SHOW_STATUS_ON_DISPLAY) {
    switch (bleState) {
      case BLE_CONNECTED:
        lcd.setTextColor(TFT_GREEN, HEADER_BG_COLOR);
        lcd.print("  BLE");
        break;
      case BLE_CONNECTING:
        lcd.setTextColor(TFT_YELLOW, HEADER_BG_COLOR);
        lcd.print("  BLE");
        break;
      case BLE_FAILED:
      case BLE_DISCONNECTED:
        lcd.setTextColor(TFT_RED, HEADER_BG_COLOR);
        lcd.print("  BLE");
        break;
    }
    lcd.setTextSize(HEADER_TEXT_SIZE);
    lcd.setTextColor(TFT_WHITE, HEADER_BG_COLOR);
  }

  // Draw altitude on right side (right-aligned)
  char altStr[20];
  snprintf(altStr, sizeof(altStr), "Alt: %.1fm", altitude);
  int textWidth = lcd.textWidth(altStr);
  lcd.setCursor(DISPLAY_WIDTH - textWidth - 5, 12);
  lcd.print(altStr);
}

void drawMainDisplay(float vSpeed)
{
  ESP_LOGI("Display", "drawMainDisplay called - vSpeed=%.2f m/s", vSpeed);
  // Determine color based on vertical speed
  uint16_t color;
  if (vSpeed > VSPEED_CLIMB_THRESHOLD)
  {
    color = TFT_GREEN; // Climbing
  }
  else if (vSpeed < VSPEED_SINK_THRESHOLD)
  {
    color = TFT_RED; // Sinking
  }
  else
  {
    color = TFT_YELLOW; // Neutral
  }

  // Clear main display area
  lcd.fillRect(0, HEADER_HEIGHT, DISPLAY_WIDTH, MAIN_DISPLAY_HEIGHT, TFT_BLACK);

  // Set text properties for main display
  lcd.setTextSize(MAIN_TEXT_SIZE);
  lcd.setTextColor(color, TFT_BLACK);

  // Format vertical speed string with sign
  char vSpeedStr[20];
  if (vSpeed >= 0)
  {
    snprintf(vSpeedStr, sizeof(vSpeedStr), "+%.1f m/s", vSpeed);
  }
  else
  {
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

void drawFooter(bool soundEnabled)
{
  // Clear footer area
  lcd.fillRect(0, DISPLAY_HEIGHT - FOOTER_HEIGHT, DISPLAY_WIDTH, FOOTER_HEIGHT, FOOTER_BG_COLOR);

  // Set text properties
  lcd.setTextSize(FOOTER_TEXT_SIZE);

  // Draw speaker icon and status
  lcd.setCursor(5, DISPLAY_HEIGHT - FOOTER_HEIGHT + 8);
  if (soundEnabled)
  {
    lcd.setTextColor(TFT_WHITE, FOOTER_BG_COLOR);
    lcd.print("[SPK] ON");
  }
  else
  {
    lcd.setTextColor(MUTED_GRAY_COLOR, FOOTER_BG_COLOR);
    lcd.print("[X] MUTED");
  }
    lcd.setCursor(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - FOOTER_HEIGHT + 8);
    lcd.print("+");
    lcd.setCursor(DISPLAY_WIDTH -40, DISPLAY_HEIGHT - FOOTER_HEIGHT + 8);
    lcd.print("-");
}
