#ifndef VARIOMETER_TASK_H
#define VARIOMETER_TASK_H

#include <Arduino.h>
#include <freertos/semphr.h>

// Variometer data structure for atomic updates
struct VariometerData {
    float altitude_m;           // Current altitude in meters
    float verticalSpeed_mps;    // Vertical speed in meters per second
    
    // Constructor for initialization
    VariometerData() : altitude_m(0.0f), verticalSpeed_mps(0.0f) {}
    
    // Constructor with values
    VariometerData(float alt, float vspeed) : altitude_m(alt), verticalSpeed_mps(vspeed) {}
    
    // Copy assignment for thread-safe updates
    VariometerData& operator=(const VariometerData& other) {
        if (this != &other) {
            altitude_m = other.altitude_m;
            verticalSpeed_mps = other.verticalSpeed_mps;
        }
        return *this;
    }
};

#ifdef __cplusplus
extern "C" {
#endif

void initVariometer();
void variometerTask(void *pvParameters);
void updateDisplayWithTelemetry(float pressure, float temperature, float baroAltitude, float verticalSpeed);

#ifdef __cplusplus
}
#endif

// Global variometer data - access must be protected by xVariometerMutex
extern VariometerData globalVariometerData;
extern SemaphoreHandle_t xVariometerMutex;

#endif // VARIOMETER_TASK_H