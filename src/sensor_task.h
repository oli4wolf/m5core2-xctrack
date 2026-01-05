#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <Arduino.h> // Required for Wire.h and other Arduino types
#include <Wire.h>    // Required for I2C communication
#include <freertos/queue.h>  // Required for QueueHandle_t

#ifdef __cplusplus
extern "C" {
#endif

// Pressure queue handle for thread-safe communication between sensor and variometer tasks
extern QueueHandle_t xPressureQueue;

void initSensor();
void sensorReadTask(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_TASK_H