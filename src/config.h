#pragma once

#include <cstddef> // For size_t

// Device Constants M5Stack Tab5
const int TILE_SIZE = 256; // Standard size for map tiles (e.g., Swisstopo, OpenStreetMap)
const int SCREEN_WIDTH = 720; // Width of the M5Stack screen
const int SCREEN_HEIGHT = 1280; // Height of the M5Stack screen

// Configuration
const bool SPEAKER_ENABLED = false; // Set to false to disable speaker functionality
const bool USE_TESTDATA = true; // Set to true to use test GPS data when no valid GPS fix is available
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
const int GPS_TASK_STACK_SIZE = 4096;
const int VARIOMETER_TASK_STACK_SIZE = 4096;
const int BUTTON_TASK_STACK_SIZE = 2048; // New: Stack size for button monitoring task
const int BUTTON_TASK_DELAY_MS = 50;    // New: Delay for button monitoring task

// Variometer Constants
const float STANDARD_SEA_LEVEL_PRESSURE_HPA = 1013.25;
const float ALTITUDE_CONSTANT_A = 44330.0;
const float ALTITUDE_CONSTANT_B = 5.255;
const int SPEAKER_DEFAULT_VOLUME = 64;
const unsigned long VARIOMETER_UPDATE_INTERVAL_MS = 200;
const float ALTITUDE_CHANGE_THRESHOLD_MPS = 0.5;
const int RISING_TONE_BASE_FREQ_HZ = 1000;
const int RISING_TONE_MULTIPLIER_HZ_PER_MPS = 50;
const int TONE_DURATION_MS = 50;
const int SINKING_TONE_BASE_FREQ_HZ = 500;
const int SINKING_TONE_MULTIPLIER_HZ_PER_MPS = 50;
const int MIN_TONE_FREQ_HZ = 100;
const int VARIOMETER_TASK_DELAY_MS = 50;

// Altitude Filter
const int ALTITUDE_FILTER_SIZE = 10; // Number of samples for moving average filter

// GPS Constants
const int GPS_TASK_DELAY_MS = 1000;
const int GPS_INIT_DELAY_MS = 2000;
const int GPS_FIX_TIMEOUT_MS = 10000; // 10 seconds
const int GPS_SERIAL_BAUD_RATE = 115200;
const int GPS_SERIAL_RX_PIN = 17; // GPIO17
const int GPS_SERIAL_TX_PIN = 16; // GPIO16
const int GPS_SERIAL_MODE   = 134217756;
const int GPS_UART = 1; // Use UART1 for GPS