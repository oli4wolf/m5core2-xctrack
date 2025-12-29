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
bool globalSensorInitialized = false; // Track if sensor initialized successfully

void initSensor()
{
    M5.Ex_I2C.release();
    delay(100); // Short delay to ensure I2C bus is released
    Wire.begin(M5.Ex_I2C.getSDA(), M5.Ex_I2C.getSCL(), 400000); // Use external I2C pins with 400kHz
    
    // Create mutex for pressure readings vector
    xPressureMutex = xSemaphoreCreateMutex();
    xSemaphoreGive(xPressureMutex);
}

void sensorReadTask(void *pvParameters)
{
    (void)pvParameters; // Suppress unused parameter warning

    // Initialize sensor here AFTER Wire.begin() was called in initSensor()
    bool sensorAvailable = barometricSensor.begin(Wire);
    
    if (sensorAvailable) {
        ESP_LOGI("Climb", "MS5637 sensor initialized successfully.");
        globalSensorInitialized = true;
    } else {
        ESP_LOGE("Climb", "MS5637 sensor did not respond. Please check wiring.");
        globalSensorInitialized = false;
    }
    
    uint32_t consecutiveFailures = 0;

    for (;;)
    {
        float pressure = 0.0f;
        float temperature = 0.0f;

        if (sensorAvailable)
        {
            pressure = barometricSensor.getPressure();
            temperature = barometricSensor.getTemperature();
            consecutiveFailures = 0; // Reset failure counter on success

            globalPressure = pressure;
            if (xSemaphoreTake(xSensorMutex, (TickType_t)10) == pdTRUE)
            {
                globalTemperature = temperature;
                xSemaphoreGive(xSensorMutex);
            }
            else
            {
                ESP_LOGE("Climb", "SensorReadTask: Could not take sensor mutex.");
            }

            // Add pressure to readings vector for Kalman filter
            if (xSemaphoreTake(xPressureMutex, (TickType_t)10) == pdTRUE)
            {
                pressureReadings.push_back(pressure);
                xSemaphoreGive(xPressureMutex);
            }
            else
            {
                ESP_LOGE("Climb", "SensorReadTask: Could not take pressure mutex.");
            }
        }
        else
        {
            // Sensor not available - provide safe dummy values
            consecutiveFailures++;
            
            // Try to recover every 10 failures
            if(consecutiveFailures % 10 == 0) {
                ESP_LOGW("Climb", "Sensor unavailable after %d attempts, retrying initialization...", consecutiveFailures);
                sensorAvailable = barometricSensor.begin(Wire);
                if (sensorAvailable) {
                    ESP_LOGI("Climb", "Sensor re-initialized successfully!");
                    globalSensorInitialized = true;
                    consecutiveFailures = 0;
                } else {
                    globalSensorInitialized = false;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200)); // Wait 0.2 second
    }
}