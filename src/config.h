#pragma once

#include <cstddef> // For size_t

// Configuration
const bool DEFAULT_SOUND_ENABLED = true; // Default sound setting
extern bool globalSoundEnabled; // Global variable to control sound output at runtime

// Zoom Constants
const int MIN_ZOOM_LEVEL = 1;
const int DEFAULT_MAP_ZOOM_LEVEL = 15;
const int MAX_ZOOM_LEVEL = 19;
const int ZOOM_THRESHOLD = 50; // Pixels for distance change to trigger zoom
const int TOUCH_TASK_STACK_SIZE = 4096; // Stack size for touch monitoring task
const int TOUCH_TASK_DELAY_MS = 20;    // Delay for touch monitoring task
const int DOUBLE_TAP_THRESHOLD_MS = 300; // Time in ms to detect a double tap

// Task Stack Sizes
const int SENSOR_TASK_STACK_SIZE = 8192;
const int VARIOMETER_TASK_STACK_SIZE = 4096;
const int BUTTON_TASK_STACK_SIZE = 2048; // New: Stack size for button monitoring task
const int BUTTON_TASK_DELAY_MS = 50;    // New: Delay for button monitoring task

// Variometer Constants
const float STANDARD_SEA_LEVEL_PRESSURE_HPA = 1013.25;
const float ALTITUDE_CONSTANT_A = 44330.0;
const float ALTITUDE_CONSTANT_B = 5.255;
const int SPEAKER_DEFAULT_VOLUME = 64;
const unsigned long VARIOMETER_UPDATE_INTERVAL_MS = 500; // 2Hz update
const float ALTITUDE_CHANGE_THRESHOLD_MPS = 0.5;
const int RISING_TONE_BASE_FREQ_HZ = 1000;
const int RISING_TONE_MULTIPLIER_HZ_PER_MPS = 50;
const int TONE_DURATION_MS = 50;
const int SINKING_TONE_BASE_FREQ_HZ = 500;
const int SINKING_TONE_MULTIPLIER_HZ_PER_MPS = 50;
const int MIN_TONE_FREQ_HZ = 100;
const int MAX_TONE_FREQ_HZ = 4000;
const int MOVING_AVERAGE_WINDOW_SIZE = 10;
const int VARIOMETER_TASK_DELAY_MS = 250;

// Kalman Filter Constants
const float KALMAN_DT = VARIOMETER_UPDATE_INTERVAL_MS / 1000.0f; // Time step in seconds
const float KALMAN_PROCESS_NOISE = 0.01f; // Process noise variance
const float KALMAN_MEASUREMENT_NOISE = 0.04f; // Measurement noise variance (0.2m ^2)

// Display Layout Constants
const int DISPLAY_UPDATE_INTERVAL_MS = 500; // 2 Hz update rate (faster than current 1Hz)
const int HEADER_HEIGHT = 40;
const int FOOTER_HEIGHT = 30;
const int MAIN_DISPLAY_HEIGHT = 170;
const int DISPLAY_WIDTH = 320;
const int DISPLAY_HEIGHT = 240;

// Vertical Speed Color Thresholds
const float VSPEED_CLIMB_THRESHOLD = 0.3;   // m/s - Green when above
const float VSPEED_SINK_THRESHOLD = -0.3;   // m/s - Red when below
// Between these values: Yellow (neutral)

// Display Colors (RGB565 format)
const uint16_t HEADER_BG_COLOR = 0x1A0E;    // Dark blue-gray
const uint16_t FOOTER_BG_COLOR = 0x2A2E;    // Dark gray
const uint16_t MUTED_GRAY_COLOR = 0x8410;   // Gray for muted status

// Update Thresholds (prevent flicker from small changes)
const float ALTITUDE_UPDATE_THRESHOLD = 1.0;  // meters
const float BATTERY_UPDATE_THRESHOLD = 0.1;   // volts
const float VSPEED_UPDATE_THRESHOLD = 0.05;   // m/s

// Text Sizes
const int HEADER_TEXT_SIZE = 2;
const int MAIN_TEXT_SIZE = 6;  // Very large for vertical speed
const int FOOTER_TEXT_SIZE = 2;
