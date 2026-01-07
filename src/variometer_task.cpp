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

// Declare extern global variables from main.cpp
extern float globalPressure;    // Still used for initialization
extern float globalTemperature; // Added for global temperature
extern SemaphoreHandle_t xSensorMutex;

// Declare extern pressure queue from sensor_task.cpp
extern QueueHandle_t xPressureQueue;

// Kalman filter instance
static KalmanFilter* kalmanFilter = nullptr;

// Moving average buffer for vertical speed
static std::vector<float> verticalSpeedHistory;

// Global variables for variometer
float globalAltitude_m = 0.0; // Current altitude in meters
float globalVerticalSpeed_mps = 0.0; // Vertical speed in meters per second
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

    // Initial altitude reading and set Kalman filter initial state
    // Wait for valid sensor data before initializing
    float initialPressure = 0.0f;
    int attempts = 0;
    while (initialPressure <= 0.0f && attempts < 100) {
        if (xSemaphoreTake(xSensorMutex, portMAX_DELAY) == pdTRUE) {
            initialPressure = globalPressure;
            xSemaphoreGive(xSensorMutex);
        }
        if (initialPressure <= 0.0f) {
            ESP_LOGW("Variometer", "Waiting for valid pressure data (attempt %d/100): %.2f hPa", attempts + 1, initialPressure);
            vTaskDelay(pdMS_TO_TICKS(100)); // Wait 100ms before retry
            attempts++;
        }
    }
    
    if (initialPressure > 0.0f) {
        float initialAltitude = pressureToAltitude(initialPressure);
        ESP_LOGI("Variometer", "Initializing with pressure=%.2f hPa, altitude=%.2f m", initialPressure, initialAltitude);
        kalmanFilter->setInitialState(initialAltitude, 0.0f); // Initial vertical speed 0
        previousAltitude = initialAltitude; // For tone logic
    } else {
        ESP_LOGE("Variometer", "Failed to get valid pressure after %d attempts, using default altitude 0m", attempts);
        kalmanFilter->setInitialState(0.0f, 0.0f);
        previousAltitude = 0.0f;
    }

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

            if (xSemaphoreTake(xVariometerMutex, portMAX_DELAY) == pdTRUE) {
                globalAltitude_m = filteredAltitude; // Already in meters
                globalVerticalSpeed_mps = avgVerticalSpeed; // Moving averaged vertical speed in m/s
                xSemaphoreGive(xVariometerMutex);
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