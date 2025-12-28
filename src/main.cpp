#include <esp_system.h> // For PRO_CPU_NUM and APP_CPU_NUM
#include <M5Unified.h>  // Make the M5Unified library available to your program.
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h> // Required for mutex

#include "esp_log.h" // For ESP logging functions

#include "ble_uart.h"        // Include the BLE UART header
#include "sensor_task.h"     // Include the new sensor task header
#include "variometer_task.h" // Include the new variometer task header
#include "sound.h"           // Include sound management header
#include "config.h"          // Include configuration constants
#include "gui.h"             // Include GUI functions

// BLE Server example
// https://github.com/naoki-sawada/m5stack-ble/blob/master/m5stack-ble/m5stack-ble.ino

// Variometer global variables
extern float globalAltitude_m;
extern float globalVerticalSpeed_mps;
float globalPressure = 0.0f;
float globalTemperature = 0.0f;                  // Added for global temperature
bool globalSoundEnabled = DEFAULT_SOUND_ENABLED; // Global variable to control sound output at runtime
int32_t globalBatteryLevel = -1; // Default sea-level pressure in hPa

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

  M5.begin(cfg);
  lcd.init();
  ESP_LOGI("Display", "lcd.init() completed, width=%d, height=%d", lcd.width(), lcd.height());

  M5.Ex_I2C.release();
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
  startupScreen(); // used to delay simplifies loading new code when something goes wrong.
  initSound();     // Initialize the sound module
  initSensor();    // Start the sensor reading task
  initializeSensorTask();
  initVariometerTask(); // Start the variometer task

  initializeVariometerTask(); // Initialize the variometer task components after sensor Task.
  initializeBLE();

  // Initialize new display layout
  initializeDisplay();
}

// Display update at configured interval
float pressure = 0.0f;
float temperature = 0.0f;
float altitude = 0.0f;
float verticalSpeed = 0.0f;
bool charging = false;
void loop()
{
  // Check for button press (sound toggle)
  M5.update();
  if (M5.BtnA.wasPressed())
  {
    globalSoundEnabled = !globalSoundEnabled;
    drawFooter(globalSoundEnabled);
    ESP_LOGI("main.cpp", "Sound toggled: %s", globalSoundEnabled ? "ON" : "MUTED");
  }

  int32_t batteryLevel = M5.Power.getBatteryLevel();
  if(batteryLevel > 1)
  {
    globalBatteryLevel = batteryLevel;
    ESP_LOGI("main.cpp", "Battery level updated: %d %%", batteryLevel);
  }else if( M5.Power.isCharging()){
    ESP_LOGI("main.cpp", "Battery level reading invalid (%d %%). Device is charging; retaining last known level: %d %%", batteryLevel, globalBatteryLevel);
    globalBatteryLevel = 100; // retain last known good value
    charging = true;
  }

  // Read sensor data with mutex protection
  if (xSemaphoreTake(xSensorMutex, portMAX_DELAY) == pdTRUE)
  {
    pressure = globalPressure;
    temperature = globalTemperature;
    xSemaphoreGive(xSensorMutex);
  }

  // Read variometer data with mutex protection
  if (xSemaphoreTake(xVariometerMutex, portMAX_DELAY) == pdTRUE)
  {
    altitude = globalAltitude_m;
    verticalSpeed = globalVerticalSpeed_mps;
    xSemaphoreGive(xVariometerMutex);
  }

  // Get BLE connection state
  BLEConnectionState bleState = getBLEState();

  // Update header if battery or altitude changed significantly
  drawHeader(globalBatteryLevel, charging, altitude, bleState);

  // Always update main display (vertical speed changes frequently)
  drawMainDisplay(verticalSpeed);

  // Log for debugging (keep existing log)
  ESP_LOGI("main.cpp", "Altitude: %.1f, V-Speed: %.2f, Pressure: %.1f, Temperature: %.2f, Battery: %d %",
           altitude, verticalSpeed, pressure, temperature, globalBatteryLevel);

  // Short delay to check buttons frequently while not blocking
  vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_INTERVAL_MS));
}
