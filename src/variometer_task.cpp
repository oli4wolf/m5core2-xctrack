#include "variometer_task.h"
#include <M5Unified.h>
#include "sensor_task.h"     // For globalPressure and xSensorMutex
#include "kalman_filter.h"  // For KalmanFilter class
#include <freertos/semphr.h>
#include <math.h> // For pow()
#include <vector> // For std::vector
#include "config.h" // Include configuration constants

// Declare extern global variables from main.cpp
extern float globalPressure;
extern float globalTemperature; // Added for global temperature
extern SemaphoreHandle_t xSensorMutex;
extern bool globalSoundEnabled; // Declare global sound enable flag

// Kalman filter instance
static KalmanFilter* kalmanFilter = nullptr;

// Global variables for variometer
float globalAltitude_m = 0.0; // Current altitude in meters
float globalVerticalSpeed_mps = 0.0; // Vertical speed in meters per second
SemaphoreHandle_t xVariometerMutex;

// Constants for altitude calculation (standard atmosphere)
// P0 is now defined in config.h as STANDARD_SEA_LEVEL_PRESSURE_HPA
// Function to convert pressure (hPa) to altitude (meters)
float pressureToAltitude(float pressure_hPa) {
    return ALTITUDE_CONSTANT_A * (1.0 - pow(pressure_hPa / STANDARD_SEA_LEVEL_PRESSURE_HPA, 1.0 / ALTITUDE_CONSTANT_B));
}

void initVariometerTask() {
    xVariometerMutex = xSemaphoreCreateMutex();
    if (xVariometerMutex == NULL) {
        ESP_LOGE("Variometer", "Failed to create variometer mutex");
    }
    // Initialize Kalman filter
    kalmanFilter = new KalmanFilter(KALMAN_DT, KALMAN_PROCESS_NOISE, KALMAN_MEASUREMENT_NOISE);
    if (kalmanFilter == nullptr) {
        ESP_LOGE("Variometer", "Failed to create Kalman filter");
    }
    M5.Speaker.begin(); // Initialize the speaker
    M5.Speaker.setVolume(SPEAKER_DEFAULT_VOLUME); // Set a default volume (0-255)
    ESP_LOGI("Variometer", "Variometer task initialized with Kalman filter. Speaker enabled.");
}

void variometerTask(void *pvParameters) {
    (void) pvParameters;

    float previousAltitude = 0.0;
    unsigned long previousMillis = millis();
    const unsigned long updateIntervalMs = VARIOMETER_UPDATE_INTERVAL_MS; // Update every VARIOMETER_UPDATE_INTERVAL_MS
    const float altitudeChangeThreshold_mps = ALTITUDE_CHANGE_THRESHOLD_MPS; // meters per second for tone trigger

    // Initial altitude reading and set Kalman filter initial state
    if (xSemaphoreTake(xSensorMutex, portMAX_DELAY) == pdTRUE) {
        float initialPressure = globalPressure;
        xSemaphoreGive(xSensorMutex);
        float initialAltitude = pressureToAltitude(initialPressure);
        kalmanFilter->setInitialState(initialAltitude, 0.0f); // Initial vertical speed 0
        previousAltitude = initialAltitude; // For tone logic
    }

    for (;;) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= updateIntervalMs) {
            float currentPressure = 0;
            if (xSemaphoreTake(xSensorMutex, portMAX_DELAY) == pdTRUE) {
                currentPressure = globalPressure;
                xSemaphoreGive(xSensorMutex);
            }

            float rawAltitude = pressureToAltitude(currentPressure);

            // Kalman filter prediction and update
            kalmanFilter->predict();
            kalmanFilter->update(rawAltitude);

            // Get filtered values
            float filteredAltitude = kalmanFilter->getAltitude();
            float filteredVerticalSpeed = kalmanFilter->getVerticalSpeed();

            if (xSemaphoreTake(xVariometerMutex, portMAX_DELAY) == pdTRUE) {
                globalAltitude_m = filteredAltitude;
                globalVerticalSpeed_mps = filteredVerticalSpeed;
                xSemaphoreGive(xVariometerMutex);
            }

            // Tone generation logic
            if (globalSoundEnabled) {
                if (filteredVerticalSpeed > altitudeChangeThreshold_mps) {
                    // Rising tone: higher frequency, frequency increases with climb rate
                    int frequency = RISING_TONE_BASE_FREQ_HZ + (int)(filteredVerticalSpeed * RISING_TONE_MULTIPLIER_HZ_PER_MPS);
                    M5.Speaker.tone(frequency, TONE_DURATION_MS); // Short tone
                } else if (filteredVerticalSpeed < -altitudeChangeThreshold_mps) {
                    // Sinking tone: lower frequency, frequency decreases with sink rate
                    int frequency = SINKING_TONE_BASE_FREQ_HZ - (int)(fabs(filteredVerticalSpeed) * SINKING_TONE_MULTIPLIER_HZ_PER_MPS);
                    if (frequency < MIN_TONE_FREQ_HZ) frequency = MIN_TONE_FREQ_HZ; // Minimum frequency
                    M5.Speaker.tone(frequency, TONE_DURATION_MS); // Short tone
                } else {
                    // Stable or minor changes, no tone
                    M5.Speaker.stop();
                }
            } else {
                M5.Speaker.stop(); // Ensure speaker is off if sound is disabled
            }

            previousAltitude = filteredAltitude; // Update previous altitude with the filtered value
            previousMillis = currentMillis;
        }

        vTaskDelay(pdMS_TO_TICKS(VARIOMETER_TASK_DELAY_MS)); // Check more frequently than updateIntervalMs
    }
}