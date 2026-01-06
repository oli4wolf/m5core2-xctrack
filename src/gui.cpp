#include "gui.h"
#include <M5Unified.h>
#include "config.h"
#include "ble_uart.h"
#include "esp_log.h"

// =============================================================================
// DISPLAY FUNCTION IMPLEMENTATIONS
// =============================================================================

void startupScreen()
{
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(1);
  M5.Display.println("M5Stack Core2 XCTrack");
  M5.Display.println("by @oli4wolf on github");
  M5.Display.println("This is a non-commercial project.");
  int32_t batteryLevel = M5.Power.getBatteryLevel();
  M5.Display.printf("Battery Level: %d %%\n", batteryLevel);
  ESP_LOGI("Display", "Startup screen drawn, battery=%d%%, waiting 5s", batteryLevel);
  vTaskDelay(5000);
}

void initializeDisplay()
{
  // Ensure display is on and bright
  M5.Display.wakeup();
  M5.Display.setBrightness(255);
  M5.Display.fillScreen(TFT_BLACK);
  // Draw static header background
  M5.Display.fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, HEADER_BG_COLOR);

  // Draw static footer background
  M5.Display.fillRect(0, DISPLAY_HEIGHT - FOOTER_HEIGHT, DISPLAY_WIDTH, FOOTER_HEIGHT, FOOTER_BG_COLOR);

  // Initial footer with sound status
  extern bool globalSoundEnabled;
  drawFooter(globalSoundEnabled);
}

void drawHeader(int32_t battery, bool charging, float altitude, BLEConnectionState bleState)
{
  ESP_LOGI("Display", "drawHeader called - battery=%d, altitude=%.1fm charging=%s", battery, altitude, charging ? "true" : "false");
  // Clear header area (maintains background color)
  M5.Display.fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, HEADER_BG_COLOR);

  // Set text properties for header
  M5.Display.setTextSize(HEADER_TEXT_SIZE);
  M5.Display.setTextColor(TFT_WHITE, HEADER_BG_COLOR);

  // Draw battery on left side
  M5.Display.setCursor(5, 12);
  if( charging ) {
    M5.Display.printf("%d%% (C)", battery);
  } else {
    M5.Display.printf("%d%%", battery);
  }

  // Draw BLE status indicator if enabled
  if (BLE_SHOW_STATUS_ON_DISPLAY) {
    M5.Display.setCursor(DISPLAY_WIDTH/2 - 50, 12);
    switch (bleState) {
      case BLE_CONNECTED:
        M5.Display.setTextColor(TFT_GREEN, HEADER_BG_COLOR);
        break;
      case BLE_CONNECTING:
        M5.Display.setTextColor(TFT_YELLOW, HEADER_BG_COLOR);
        break;
      case BLE_FAILED:
      case BLE_DISCONNECTED:
        M5.Display.setTextColor(TFT_RED, HEADER_BG_COLOR);
        break;
    }
    M5.Display.print("BLE");
    M5.Display.setTextSize(HEADER_TEXT_SIZE);
    M5.Display.setTextColor(TFT_WHITE, HEADER_BG_COLOR);
  }

  // Draw altitude on right side (right-aligned)
  char altStr[20];
  snprintf(altStr, sizeof(altStr), "Alt: %.1fm", altitude);
  int textWidth = M5.Display.textWidth(altStr);
  M5.Display.setCursor(DISPLAY_WIDTH - textWidth - 5, 12);
  M5.Display.print(altStr);
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
  M5.Display.fillRect(0, HEADER_HEIGHT, DISPLAY_WIDTH, MAIN_DISPLAY_HEIGHT, TFT_BLACK);

  // Set text properties for main display
  M5.Display.setTextSize(MAIN_TEXT_SIZE);
  M5.Display.setTextColor(color, TFT_BLACK);

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
  int textWidth = M5.Display.textWidth(vSpeedStr);
  int textHeight = M5.Display.fontHeight();
  int x = (DISPLAY_WIDTH - textWidth) / 2;
  int y = HEADER_HEIGHT + (MAIN_DISPLAY_HEIGHT - textHeight) / 2;

  // Draw vertical speed
  M5.Display.setCursor(x, y);
  M5.Display.print(vSpeedStr);
}

void drawFooter(bool soundEnabled)
{
  // Clear footer area
  M5.Display.fillRect(0, DISPLAY_HEIGHT - FOOTER_HEIGHT, DISPLAY_WIDTH, FOOTER_HEIGHT, FOOTER_BG_COLOR);

  // Set text properties
  M5.Display.setTextSize(FOOTER_TEXT_SIZE);

  // Draw speaker icon and status
  M5.Display.setCursor(5, DISPLAY_HEIGHT - FOOTER_HEIGHT + 8);
  if (soundEnabled)
  {
    M5.Display.setTextColor(TFT_WHITE, FOOTER_BG_COLOR);
    M5.Display.print("[SPK] ON");
  }
  else
  {
    M5.Display.setTextColor(MUTED_GRAY_COLOR, FOOTER_BG_COLOR);
    M5.Display.print("[X] MUTED");
  }
    M5.Display.setCursor(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - FOOTER_HEIGHT + 8);
    M5.Display.print("+");
    M5.Display.printf("  %d", M5.Speaker.getVolume());
    M5.Display.setCursor(DISPLAY_WIDTH -40, DISPLAY_HEIGHT - FOOTER_HEIGHT + 8);
    M5.Display.print("-");
}
