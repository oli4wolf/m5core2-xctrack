#include "variometer_task.h"
#include <M5Unified.h>
#include "sensor_task.h"     // For xPressureQueue
#include "kalman_filter.h"  // For KalmanFilter class
#include <freertos/semphr.h>
#include <freertos/queue.h>  // For queue operations
#include <math.h> // For pow()
#include <vector> // For std::vector
#include "config.h" // Include configuration constants
#include "sound.h" // Include sound module

// Declare extern pressure queue from sensor_task.cpp
extern QueueHandle_t xPressureQueue;

// Kalman filter instance
static KalmanFilter* kalmanFilter = nullptr;

// Moving average buffer for vertical speed
static std::vector<float> verticalSpeedHistory;

// Global variometer data structure and mutex
VariometerData globalVariometerData;
SemaphoreHandle_t xVariometerMutex;

// Constants for altitude calculation (standard atmosphere)
// P0 is now defined in config.h as STANDARD_SEA_LEVEL_PRESSURE_HPA
// Function to convert pressure (hPa) to altitude (meters)
float pressureToAltitude(float pressure_hPa) {
    // Guard against invalid pressure readings that would produce NaN
    if (pressure_hPa <= 0.0f || isnan(pressure_hPa) || isinf(pressure_hPa)) {
        ESP_LOGW("Variometer", "Invalid pressure: %.2f hPa, returning 0.0m altitude", pressure_hPa);
        return 0.0f;
    }
    
    float altitude = ALTITUDE_CONSTANT_A * (pow(STANDARD_SEA_LEVEL_PRESSURE_HPA / pressure_hPa, 1.0f / ALTITUDE_CONSTANT_B) - 1.0f);
    
    // Additional guard: check if calculated altitude is valid
    if (isnan(altitude) || isinf(altitude)) {
        ESP_LOGW("Variometer", "Calculated altitude is NaN/Inf for pressure %.2f hPa", pressure_hPa);
        return 0.0f;
    }
    
    return altitude;
}

void initVariometer() {
    xVariometerMutex = xSemaphoreCreateMutex();
    if (xVariometerMutex == NULL) {
        ESP_LOGE("Variometer", "Failed to create variometer mutex");
    }
    // Initialize Kalman filter
    kalmanFilter = new KalmanFilter(KALMAN_DT, KALMAN_PROCESS_NOISE, KALMAN_MEASUREMENT_NOISE);
    if (kalmanFilter == nullptr) {
        ESP_LOGE("Variometer", "Failed to create Kalman filter");
    }

    ESP_LOGI("Variometer", "Variometer task initialized with Kalman filter and moving average filter. Sound enabled.");
}

void variometerTask(void *pvParameters) {
    (void) pvParameters;

    float previousAltitude = 0.0;
    unsigned long previousMillis = millis();
    const unsigned long updateIntervalMs = VARIOMETER_UPDATE_INTERVAL_MS; // Update every VARIOMETER_UPDATE_INTERVAL_MS
    const float altitudeChangeThreshold_mps = ALTITUDE_CHANGE_THRESHOLD_MPS; // meters per second for tone trigger

    for (;;) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= updateIntervalMs) {
            // Process all available pressure readings from queue
            float pressure;
            int readingsProcessed = 0;
            
            while (xQueueReceive(xPressureQueue, &pressure, 0) == pdPASS) {
                float rawAltitude = pressureToAltitude(pressure);
                
                // Kalman filter prediction and update
                kalmanFilter->predict();
                kalmanFilter->update(rawAltitude);
                
                readingsProcessed++;
            }
            
            // Log diagnostics
            if (readingsProcessed == 0) {
                ESP_LOGW("Variometer", "No pressure readings available in queue");
            } else if (readingsProcessed > 1) {
                ESP_LOGD("Variometer", "Processed %d pressure readings", readingsProcessed);
            }

            // Get filtered values from Kalman filter
            float filteredAltitude = kalmanFilter->getAltitude();

            // Calculate raw vertical speed and apply moving average
            float dt_seconds = updateIntervalMs / 1000.0f;
            float rawVerticalSpeed = (filteredAltitude - previousAltitude) / dt_seconds;
            verticalSpeedHistory.push_back(rawVerticalSpeed);
            if (verticalSpeedHistory.size() > MOVING_AVERAGE_WINDOW_SIZE) {
                verticalSpeedHistory.erase(verticalSpeedHistory.begin());
            }
            float avgVerticalSpeed = 0.0f;
            if (!verticalSpeedHistory.empty()) {
                for (float v : verticalSpeedHistory) {
                    avgVerticalSpeed += v;
                }
                avgVerticalSpeed /= verticalSpeedHistory.size();
            }

            // Update global variables with validated values
            // Use reasonable physical limits to detect sensor/calculation errors
            const float MAX_ALTITUDE_M = 9000.0f;  // Typical paragliding max ~8000m
            const float MIN_ALTITUDE_M = -500.0f;  // Below sea level limit
            const float MAX_VSPEED_MPS = 50.0f;    // Realistic vertical speed limit (~180 km/h)
            
            // Validate filtered altitude
            bool altitudeValid = !isnan(filteredAltitude) && !isinf(filteredAltitude) &&
                                 filteredAltitude >= MIN_ALTITUDE_M &&
                                 filteredAltitude <= MAX_ALTITUDE_M;
            
            // Validate vertical speed
            bool vspeedValid = !isnan(avgVerticalSpeed) && !isinf(avgVerticalSpeed) &&
                               fabs(avgVerticalSpeed) <= MAX_VSPEED_MPS;
            
            if (!altitudeValid) {
                ESP_LOGW("Variometer", "Invalid altitude: %.2f m - skipping update", filteredAltitude);
            }
            
            if (!vspeedValid) {
                ESP_LOGW("Variometer", "Invalid vertical speed: %.2f m/s - skipping update", avgVerticalSpeed);
            }
            
            // Only update globals if both values are valid
            if (altitudeValid && vspeedValid) {
                if (xSemaphoreTake(xVariometerMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    // Atomic update of variometer data using struct
                    globalVariometerData.altitude_m = filteredAltitude;
                    globalVariometerData.verticalSpeed_mps = avgVerticalSpeed;
                    xSemaphoreGive(xVariometerMutex);
                } else {
                    ESP_LOGW("Variometer", "Failed to acquire mutex for global update");
                }
            }

            // Tone generation logic
            if (globalSoundEnabled) {
                playTone(avgVerticalSpeed);
            } else {
                M5.Speaker.stop(); // Ensure speaker is off if sound is disabled
            }

            previousAltitude = filteredAltitude; // Update previous altitude with the filtered value
            previousMillis = currentMillis;
        }

        vTaskDelay(pdMS_TO_TICKS(VARIOMETER_TASK_DELAY_MS)); // Check more frequently than updateIntervalMs
    }
}