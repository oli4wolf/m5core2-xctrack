#include <esp_system.h> // For PRO_CPU_NUM and APP_CPU_NUM
#include <M5Unified.h>  // Make the M5Unified library available to your program.
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h> // Required for mutex

#include "esp_log.h"       // For ESP logging functions

#include "ble_uart.h"        // Include the BLE UART header
#include "sensor_task.h"     // Include the new sensor task header
#include "variometer_task.h" // Include the new variometer task header
#include "sound.h"         // Include sound management header
#include "config.h"          // Include configuration constants

// BLE Server example
// https://github.com/naoki-sawada/m5stack-ble/blob/master/m5stack-ble/m5stack-ble.ino

M5GFX lcd;

// Variometer global variables
extern float globalAltitude_m;
extern float globalVerticalSpeed_mps;
float globalPressure = 0.0f;
float globalTemperature=0.0f; // Added for global temperature
bool globalSoundEnabled = DEFAULT_SOUND_ENABLED; // Global variable to control sound output at runtime

extern SemaphoreHandle_t xVariometerMutex;
SemaphoreHandle_t xSensorMutex;

// =============================================================================
// DISPLAY FUNCTION DECLARATIONS
// =============================================================================

void drawHeader(float battery, float altitude);
void drawMainDisplay(float vSpeed);
void drawFooter(bool soundEnabled);
void initializeDisplay();

// =============================================================================
// TASK INITIALIZATION FUNCTIONS
// =============================================================================

/**
 * @brief Initializes the BLE task for sending values over Bluetooth
 */
void initializeBLE() {
    xTaskCreatePinnedToCore(
        ble_task,     // Task function
        "BLE Task",   // Name of the task
        4096,         // Stack size in bytes
        NULL,         // Task input parameter
        1,            // Priority of the task
        NULL,         // Task handle
        APP_CPU_NUM   // Run on APP CPU
    );
}

/**
 * @brief Initializes the sensor reading task for barometer data
 */
void initializeSensorTask() {
    xTaskCreatePinnedToCore(
        sensorReadTask,     // Task function
        "Sensor Read Task", // Name of the task
        8192,               // Stack size in bytes
        NULL,               // Task input parameter
        1,                  // Priority of the task
        NULL,               // Task handle
        APP_CPU_NUM         // Run on APP CPU
    );
}

/**
 * @brief Initializes the variometer task for calculating averages
 */
void initializeVariometerTask() {
    xTaskCreatePinnedToCore(
        variometerTask,     // Task function
        "Variometer Task",  // Name of the task
        8192,               // Stack size in bytes
        NULL,               // Task input parameter
        1,                  // Priority of the task
        NULL,               // Task handle
        APP_CPU_NUM         // Run on APP CPU
    );
}

void initializeM5Stack()
{
  // M5Stack Core2 Initialization
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  cfg.internal_imu = true;  // default=true. use internal IMU.
  cfg.internal_rtc = true;  // default=true. use internal RTC.
  cfg.internal_spk = true;  // default=true. use internal speaker.
  cfg.internal_mic = false; // default=true. use internal microphone.
  cfg.external_imu = false; // default=false. use Unit Accel & Gyro.
  cfg.external_rtc = false; // default=false. use Unit RTC.

  M5.begin(cfg);
  lcd.init();
  M5.Ex_I2C.release();
}

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
  lcd.printf("Battery Level: %.2f V\n", M5.Power.getBatteryLevel());
  delay(5000);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  ESP_LOGI("main.cpp", "Serial initialized");

  float batVoltage = M5.Power.getBatteryVoltage();
  ESP_LOGI("main.cpp", "Battery voltage: %.2f", batVoltage);
  xSensorMutex = xSemaphoreCreateMutex(); // Initialize the sensor mutex
  xSemaphoreGive(xSensorMutex);
  ESP_LOGI("main.cpp", "Sensor mutex created");

  // Suppress debug logs for BLE-related tags (adjust tags as needed based on library output)
  esp_log_level_set("BLE*", ESP_LOG_WARN);  // Example: Suppress for BLE components
  esp_log_level_set("bt", ESP_LOG_WARN);

  initializeM5Stack();
  startupScreen(); // used to delay simplifies loading new code when something goes wrong.
  initSound(); // Initialize the sound module
  initSensor(); // Start the sensor reading task
  initializeSensorTask();
  initVariometerTask(); // Start the variometer task
  delay(2000); // Wait for 2 seconds to ensure everything is initialized

  initializeVariometerTask(); // Initialize the variometer task components after sensor Task.
  initializeBLE();
  
  // Initialize new display layout
  initializeDisplay();
}

void loop()
{
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

// =============================================================================
// DISPLAY FUNCTION IMPLEMENTATIONS
// =============================================================================

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

void drawFooter(bool soundEnabled) {
  // Clear footer area
  lcd.fillRect(0, DISPLAY_HEIGHT - FOOTER_HEIGHT, DISPLAY_WIDTH, FOOTER_HEIGHT, FOOTER_BG_COLOR);
  
  // Set text properties
  lcd.setTextSize(FOOTER_TEXT_SIZE);
  
  // Draw speaker icon and status
  lcd.setCursor(5, DISPLAY_HEIGHT - FOOTER_HEIGHT + 8);
  if (soundEnabled) {
    lcd.setTextColor(TFT_WHITE, FOOTER_BG_COLOR);
    lcd.print("[SPK] ON");
  } else {
    lcd.setTextColor(MUTED_GRAY_COLOR, FOOTER_BG_COLOR);
    lcd.print("[X] MUTED");
  }
}
