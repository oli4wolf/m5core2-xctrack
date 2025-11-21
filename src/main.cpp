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
bool globalSoundEnabled = false; // Global variable to control sound output at runtime

extern SemaphoreHandle_t xVariometerMutex;
SemaphoreHandle_t xSensorMutex;

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

  float batVoltage = M5.Power.getBatteryVoltage();
  xSensorMutex = xSemaphoreCreateMutex(); // Initialize the sensor mutex
  xSemaphoreGive(xSensorMutex);

  // Suppress debug logs for BLE-related tags (adjust tags as needed based on library output)
  esp_log_level_set("BLE*", ESP_LOG_WARN);  // Example: Suppress for BLE components
  esp_log_level_set("bt", ESP_LOG_WARN);

  startupScreen();
  initializeM5Stack();
  initSound(); // Initialize the sound module
  initSensor(); // Start the sensor reading task
  initializeSensorTask();
  initVariometerTask(); // Start the variometer task
  delay(2000); // Wait for 2 seconds to ensure everything is initialized
  initializeVariometerTask(); // Initialize the variometer task components
  initializeBLE();
  
}

void loop()
{
  lcd.fillScreen(TFT_BLACK);
  lcd.setCursor(0, 0);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(2);
  lcd.printf("Pressure: %.1f \n", globalPressure);
  lcd.printf("Temperature: %.2f°\n", globalTemperature);
  lcd.printf("Altitude: %.1f m\n", globalAltitude_m);
  lcd.printf("V-Speed: %.2f m/s\n", globalVerticalSpeed_mps);
  lcd.printf("Battery: %.2f V\n", M5.Power.getBatteryCurrent());

  ESP_LOGI("main.cpp","Altitude: %.1f, V-Speed: %.2f, Pressure: %.1f m, Temperature: %.2f°, Battery: %.2f V",globalAltitude_m, globalVerticalSpeed_mps, globalPressure, globalTemperature, M5.Power.getBatteryCurrent());
  // put your main code here, to run repeatedly:
  delay(2000);
}
