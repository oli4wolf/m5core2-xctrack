#include "sensor_task.h"
#include <M5Unified.h>
#include <SparkFun_MS5637_Arduino_Library.h>
#include <freertos/semphr.h> // Required for mutex

// Declare extern global variables and mutex from main.cpp
extern float globalPressure;
extern float globalTemperature;
extern SemaphoreHandle_t xSensorMutex;

MS5637 barometricSensor;
static uint32_t sensor_count = 0;

void initSensorTask() {
    if (barometricSensor.begin(Wire) == false)
    {
        ESP_LOGE("Climb", "MS5637 sensor did not respond. Please check wiring and I2C address.");
        while (1)
            ;
    } else {
        ESP_LOGI("Climb", "MS5637 sensor initialized successfully.");
    }
}

void sensorReadTask(void *pvParameters) {
    (void) pvParameters; // Suppress unused parameter warning

    Wire.begin(M5.Ex_I2C.getSDA(), M5.Ex_I2C.getSCL()); // Reconfigure default Wire to use M5.Ex_I2C pins
    Wire.setClock(400000); // Set I2C frequency to 400kHz for MS5637

    for (;;) {
        ESP_LOGD("sensor_task.cpp", "Sensor task iteration start");
        ESP_LOGD("sensor_task.cpp", "Starting I2C pressure read");
        float pressure = barometricSensor.getPressure();
        ESP_LOGD("sensor_task.cpp", "Pressure read: %.2f", pressure);
        ESP_LOGD("sensor_task.cpp", "Starting I2C temperature read");
        float temperature = barometricSensor.getTemperature();
        ESP_LOGD("sensor_task.cpp", "Temperature read: %.2f", temperature);

        if (xSemaphoreTake(xSensorMutex, (TickType_t)10) == pdTRUE) { // Attempt to take mutex with a timeout
        globalPressure = pressure;
        globalTemperature = temperature;
        xSemaphoreGive(xSensorMutex);
        ESP_LOGD("sensor_task.cpp", "Sensor mutex released");
        } else {
            ESP_LOGE("Climb", "SensorReadTask: Could not take sensor mutex.");
        }

        sensor_count++;
        ESP_LOGD("sensor_task.cpp", "Sensor task iteration end, delaying 200ms");
        vTaskDelay(pdMS_TO_TICKS(200)); // Wait 0.2 second
    }
}