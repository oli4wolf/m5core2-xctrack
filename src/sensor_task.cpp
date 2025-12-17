#include "sensor_task.h"
#include <M5Unified.h>
#include <SparkFun_MS5637_Arduino_Library.h>
#include <freertos/semphr.h> // Required for mutex

// Declare extern global variables and mutex from main.cpp
extern float globalPressure;
extern float globalTemperature;
extern SemaphoreHandle_t xSensorMutex;

// Declare pressure readings vector and mutex
std::vector<float> pressureReadings;
SemaphoreHandle_t xPressureMutex = NULL;

MS5637 barometricSensor;
static uint32_t sensor_count = 0;

void initSensor() {
     M5.Ex_I2C.release();
     
     delay(100); // Short delay to ensure I2C bus is released
     Wire.begin(M5.Ex_I2C.getSDA(),  M5.Ex_I2C.getSCL(), 400000); // Use external I2C pins with 400kHz
     if (barometricSensor.begin(Wire) == false)
     {
         ESP_LOGE("Climb", "MS5637 sensor did not respond. Please check wiring and I2C address.");
         // DO NOT hang - sensor failure will be handled by showing error state
         // The sensor task will return dummy values until sensor is available
     } else {
         ESP_LOGI("Climb", "MS5637 sensor initialized successfully with: %d, %d.", M5.Ex_I2C.getSDA(), M5.Ex_I2C.getSCL());
     }
     // Create mutex for pressure readings vector
     xPressureMutex = xSemaphoreCreateMutex();
     xSemaphoreGive(xPressureMutex);
}

void sensorReadTask(void *pvParameters) {
    (void) pvParameters; // Suppress unused parameter warning
    
    bool sensorAvailable = barometricSensor.begin(Wire);
    uint32_t retryAttempts = 0;
    uint32_t consecutiveFailures = 0;

    ESP_LOGI("sensor_task.cpp", "DIAG: Initial sensor detection attempt: %s", sensorAvailable ? "SUCCESS" : "FAILED");

    for (;;) {
        float pressure = 0.0f;
        float temperature = 0.0f;
        
        if (sensorAvailable) {
            ESP_LOGD("sensor_task.cpp", "Starting I2C pressure read");
            pressure = barometricSensor.getPressure();
            temperature = barometricSensor.getTemperature();
            ESP_LOGD("sensor_task.cpp", "Pressure: %.2f, Temperature read: %.2f", pressure, temperature);
            consecutiveFailures = 0;  // Reset failure counter on success
        } else {
            // Sensor not available - provide safe dummy values
            pressure = 1013.25f;  // Standard sea level pressure
            temperature = 20.0f;  // Room temperature
            consecutiveFailures++;
            ESP_LOGW("sensor_task.cpp", "DIAG: Sensor unavailable (failure #%d), using dummy values", consecutiveFailures);
            ESP_LOGI("sensor_task.cpp", "DIAG: sensorAvailable flag is currently: %s", sensorAvailable ? "true" : "false");
            ESP_LOGE("sensor_task.cpp", "DIAG: NO RETRY LOGIC - sensor will never be rechecked!");
        }

        globalPressure = pressure;
        if (xSemaphoreTake(xSensorMutex, (TickType_t)10) == pdTRUE) { // Attempt to take mutex with a timeout
            globalTemperature = temperature;
            xSemaphoreGive(xSensorMutex);
            ESP_LOGD("sensor_task.cpp", "Sensor mutex released");
        } else {
            ESP_LOGE("Climb", "SensorReadTask: Could not take sensor mutex.");
        }

        // Add pressure to readings vector for Kalman filter
        if (xSemaphoreTake(xPressureMutex, (TickType_t)10) == pdTRUE) {
            pressureReadings.push_back(pressure);
            xSemaphoreGive(xPressureMutex);
            ESP_LOGD("sensor_task.cpp", "Pressure added to readings vector");
        } else {
            ESP_LOGE("Climb", "SensorReadTask: Could not take pressure mutex.");
        }

        sensor_count++;
        // Todo change the timing.
        vTaskDelay(pdMS_TO_TICKS(200)); // Wait 0.2 second
    }
}