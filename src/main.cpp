#include <esp_system.h> // For PRO_CPU_NUM and APP_CPU_NUM
#include <M5Unified.h>  // Make the M5Unified library available to your program.
#include <M5GFX.h>  // Include M5GFX for display handling
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h> // Required for mutex
#include <Preferences.h>     // For NVS access to clear cached board type

#include "esp_log.h" // For ESP logging functions

#include "ble_uart.h"        // Include the BLE UART header
#include "sensor_task.h"     // Include the new sensor task header
#include "variometer_task.h" // Include the new variometer task header
#include "sound.h"           // Include sound management header
#include "config.h"          // Include configuration constants
#include "gui.h"             // Include GUI functions
#include "button.h"          // Include button handling functions
#include "power.h"           // Include power management functions
#include "Arduino.h"

// BLE Server example
// https://github.com/naoki-sawada/m5stack-ble/blob/master/m5stack-ble/m5stack-ble.ino

// Variometer global variables
bool globalSoundEnabled = DEFAULT_SOUND_ENABLED; // Global variable to control sound output at runtime
BatteryState globalBatteryState = {-1, false, false}; // Battery state: level=-1 (invalid), not charging, no USB

extern SemaphoreHandle_t xVariometerMutex;
SemaphoreHandle_t xSensorMutex;

// =============================================================================
// TASK INITIALIZATION FUNCTIONS
// =============================================================================

/**
 * @brief Initializes the BLE task for sending values over Bluetooth
 */
void initializeBLE()
{
  xTaskCreatePinnedToCore(
      ble_task,   // Task function
      "BLE Task", // Name of the task
      4096,       // Stack size in bytes
      NULL,       // Task input parameter
      1,          // Priority of the task
      NULL,       // Task handle
      APP_CPU_NUM // Run on APP CPU
  );
}

/**
 * @brief Initializes the sensor reading task for barometer data
 */
void initializeSensorTask()
{
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
void initializeVariometerTask()
{
  xTaskCreatePinnedToCore(
      variometerTask,    // Task function
      "Variometer Task", // Name of the task
      8192,              // Stack size in bytes
      NULL,              // Task input parameter
      1,                 // Priority of the task
      NULL,              // Task handle
      APP_CPU_NUM        // Run on APP CPU
  );
}

void initializeM5Stack()
{
  ESP_LOGI("Display", "Starting M5Stack initialization");

  // M5Stack Core2 Initialization
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  cfg.internal_imu = true;  // default=true. use internal IMU.
  cfg.internal_rtc = true;  // default=true. use internal RTC.
  cfg.internal_spk = true;  // default=true. use internal speaker.
  cfg.internal_mic = false; // default=true. use internal microphone.
  cfg.external_imu = false; // default=false. use Unit Accel & Gyro.
  cfg.external_rtc = false; // default=false. use Unit RTC.
  cfg.clear_display = true;
  cfg.output_power = true;
  
  M5.begin(cfg);

  ESP_LOGI("Display", "M5.begin() completed - Board: %d, Display: %dx%d",
           M5.getBoard(), M5.Display.width(), M5.Display.height());
}

void setup()
{
  Serial.begin(115200);
  ESP_LOGD("main.cpp", "Starting BLE work!");

  xSensorMutex = xSemaphoreCreateMutex(); // Initialize the sensor mutex
  xSemaphoreGive(xSensorMutex);
  ESP_LOGI("main.cpp", "Sensor mutex created");

  // Suppress debug logs for BLE-related tags (adjust tags as needed based on library output)
  esp_log_level_set("BLE*", ESP_LOG_WARN); // Example: Suppress for BLE components
  esp_log_level_set("bt", ESP_LOG_WARN);

  // MUST initialize M5Stack BEFORE reading battery level
  initializeM5Stack();
  //startupScreen(); // used to delay simplifies loading new code when something goes wrong.
  initSound();     // Initialize the sound module
  initSensor();    // Start the sensor reading task
  initializeSensorTask();
  initVariometer(); // Start the variometer task

  initializeVariometerTask(); // Initialize the variometer task components after sensor Task.
  initializeBLE();

  // Initialize new display layout
  initializeDisplay();
}

// Display update at configured interval
float altitude = 0.0f;
float verticalSpeed = 0.0f;

void loop()
{
  // Check for button press (sound toggle)
  M5.update();

  handleButtonInput();
  globalBatteryState = updateBatteryStatus();

  // Read variometer data with mutex protection
  if (xSemaphoreTake(xVariometerMutex, portMAX_DELAY) == pdTRUE)
  {
    altitude = globalVariometerData.altitude_m;
    verticalSpeed = globalVariometerData.verticalSpeed_mps;
    xSemaphoreGive(xVariometerMutex);
  }

  // Get BLE connection state
  BLEConnectionState bleState = getBLEState();

  // Update header with battery state, altitude, and BLE state
  drawHeader(globalBatteryState, altitude, bleState);

  // Always update main display (vertical speed changes frequently)
  drawMainDisplay(verticalSpeed);

  // Log for debugging
  ESP_LOGI("main.cpp", "Altitude: %.1f, V-Speed: %.2f, Battery: %d %%",
           altitude, verticalSpeed, globalBatteryState.level);
  
  vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_INTERVAL_MS));
}