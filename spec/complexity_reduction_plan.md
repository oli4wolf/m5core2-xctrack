# Complexity Reduction Plan

## Executive Summary

This document outlines a phased approach to reduce code complexity in the M5Core2-XCTrack variometer project. The plan addresses:
- Global variable coupling across modules
- Scattered mutex management
- Monolithic task implementations
- Configuration sprawl

**Estimated Total Effort**: 8-12 hours  
**Risk Level**: Low-Medium (changes are incremental and testable)

---

## Phase 1: Data Access Layer (Priority: Critical)

### Goal
Replace scattered global variables and mutexes with a centralized `DataStore` singleton.

### Current State

```
main.cpp        ──extern──▶  globalPressure, globalTemperature
sensor_task.cpp ──extern──▶  globalPressure, globalTemperature, xSensorMutex
variometer_task.cpp ──extern──▶  globalPressure, globalTemperature, xSensorMutex
                    ──extern──▶  globalAltitude_m, globalVerticalSpeed_mps
ble_uart.cpp    ──extern──▶  globalAltitude_m, globalVerticalSpeed_mps, xVariometerMutex
gui.cpp         ──access via params (OK)
```

**Problems**:
- 6+ `extern` declarations spread across files
- 2 separate mutexes manually managed
- Easy to forget mutex protection

### Target State

```
┌─────────────────────────────────────────────────────┐
│                    DataStore                        │
│  ┌─────────────────────────────────────────────┐   │
│  │  SensorData { pressure, temperature }       │   │
│  │  VariometerData { altitude, verticalSpeed } │   │
│  │  BatteryData { level, voltage }             │   │
│  └─────────────────────────────────────────────┘   │
│  Mutex: Single internal mutex                       │
│  Methods: setSensor(), setVario(), getSnapshot()    │
└─────────────────────────────────────────────────────┘
         ▲           ▲           ▲           ▲
         │           │           │           │
    sensor_task  vario_task  ble_task    main.cpp
```

### Implementation Steps

#### Step 1.1: Create DataStore Header
Create `src/data_store.h`:

```cpp
#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct SensorData {
    float pressure;      // hPa
    float temperature;   // °C
};

struct VariometerData {
    float altitude_m;
    float verticalSpeed_mps;
};

struct FlightSnapshot {
    SensorData sensor;
    VariometerData vario;
    int32_t batteryLevel;  // Percentage
};

class DataStore {
public:
    static DataStore& getInstance();
    
    // Thread-safe setters (called from tasks)
    void setSensorData(float pressure, float temperature);
    void setVariometerData(float altitude, float vspeed);
    void setBatteryLevel(int32_t level);
    
    // Thread-safe getters (return copies)
    SensorData getSensorData();
    VariometerData getVariometerData();
    FlightSnapshot getSnapshot();  // Atomic read of all data
    
private:
    DataStore();
    ~DataStore() = default;
    DataStore(const DataStore&) = delete;
    DataStore& operator=(const DataStore&) = delete;
    
    SensorData sensorData_;
    VariometerData varioData_;
    int32_t batteryLevel_;
    SemaphoreHandle_t mutex_;
};
```

#### Step 1.2: Create DataStore Implementation
Create `src/data_store.cpp`:

```cpp
#include "data_store.h"
#include "esp_log.h"

DataStore& DataStore::getInstance() {
    static DataStore instance;
    return instance;
}

DataStore::DataStore() 
    : sensorData_{0.0f, 0.0f}
    , varioData_{0.0f, 0.0f}
    , batteryLevel_(0) {
    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_) {
        ESP_LOGE("DataStore", "Failed to create mutex");
    }
}

void DataStore::setSensorData(float pressure, float temperature) {
    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        sensorData_.pressure = pressure;
        sensorData_.temperature = temperature;
        xSemaphoreGive(mutex_);
    }
}

void DataStore::setVariometerData(float altitude, float vspeed) {
    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        varioData_.altitude_m = altitude;
        varioData_.verticalSpeed_mps = vspeed;
        xSemaphoreGive(mutex_);
    }
}

void DataStore::setBatteryLevel(int32_t level) {
    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        batteryLevel_ = level;
        xSemaphoreGive(mutex_);
    }
}

SensorData DataStore::getSensorData() {
    SensorData copy;
    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        copy = sensorData_;
        xSemaphoreGive(mutex_);
    }
    return copy;
}

VariometerData DataStore::getVariometerData() {
    VariometerData copy;
    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        copy = varioData_;
        xSemaphoreGive(mutex_);
    }
    return copy;
}

FlightSnapshot DataStore::getSnapshot() {
    FlightSnapshot snap;
    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        snap.sensor = sensorData_;
        snap.vario = varioData_;
        snap.batteryLevel = batteryLevel_;
        xSemaphoreGive(mutex_);
    }
    return snap;
}
```

#### Step 1.3: Migrate sensor_task.cpp
**Before**:
```cpp
extern float globalPressure;
extern float globalTemperature;
extern SemaphoreHandle_t xSensorMutex;
// ...
if (xSemaphoreTake(xSensorMutex, (TickType_t)10) == pdTRUE) {
    globalPressure = pressure;
    globalTemperature = temperature;
    xSemaphoreGive(xSensorMutex);
}
```

**After**:
```cpp
#include "data_store.h"
// ...
DataStore::getInstance().setSensorData(pressure, temperature);
```

#### Step 1.4: Migrate variometer_task.cpp
**Before**:
```cpp
extern float globalPressure;
extern SemaphoreHandle_t xSensorMutex;
// ...
if (xSemaphoreTake(xSensorMutex, portMAX_DELAY) == pdTRUE) {
    currentPressure = globalPressure;
    xSemaphoreGive(xSensorMutex);
}
```

**After**:
```cpp
#include "data_store.h"
// ...
SensorData sensor = DataStore::getInstance().getSensorData();
float currentPressure = sensor.pressure;
```

#### Step 1.5: Migrate ble_uart.cpp
**Before**:
```cpp
extern float globalAltitude_m;
extern float globalVerticalSpeed_mps;
extern SemaphoreHandle_t xVariometerMutex;
// ...
if (xSemaphoreTake(xVariometerMutex, (TickType_t)10) == pdTRUE) {
    altitudeM = static_cast<int32_t>(globalAltitude_m);
    climbrateCps = static_cast<int32_t>(globalVerticalSpeed_mps * 100);
    xSemaphoreGive(xVariometerMutex);
}
```

**After**:
```cpp
#include "data_store.h"
// ...
VariometerData vario = DataStore::getInstance().getVariometerData();
altitudeM = static_cast<int32_t>(vario.altitude_m);
climbrateCps = static_cast<int32_t>(vario.verticalSpeed_mps * 100);
```

#### Step 1.6: Migrate main.cpp
**Before**:
```cpp
extern float globalAltitude_m;
extern float globalVerticalSpeed_mps;
float globalPressure = 0.0f;
float globalTemperature = 0.0f;
SemaphoreHandle_t xSensorMutex;
```

**After**:
```cpp
#include "data_store.h"
// Remove all extern declarations and global definitions for flight data
// In loop():
FlightSnapshot data = DataStore::getInstance().getSnapshot();
drawHeader(data.batteryLevel, data.vario.altitude_m, bleState);
drawMainDisplay(data.vario.verticalSpeed_mps);
```

#### Step 1.7: Remove globalSoundEnabled from main.cpp
Move to `sound.h`/`sound.cpp` as proper encapsulation:
```cpp
// sound.h
bool isSoundEnabled();
void setSoundEnabled(bool enabled);
void toggleSound();
```

### Verification Checklist for Phase 1
- [ ] Compiles without errors
- [ ] No remaining `extern float global*` in any file
- [ ] No remaining `xSensorMutex` or `xVariometerMutex` outside DataStore
- [ ] BLE transmission still works (test with XCTrack)
- [ ] Display still updates correctly
- [ ] Sound toggle still works

---

## Phase 2: Extract Reusable Filters (Priority: Medium)

### Goal
Create reusable filter classes to eliminate inline averaging code.

### Current State
`variometer_task.cpp` lines 86-96:
```cpp
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
```

### Implementation Steps

#### Step 2.1: Create Filters Header
Create `src/filters.h`:

```cpp
#pragma once
#include <vector>
#include <cstddef>

class MovingAverage {
public:
    explicit MovingAverage(size_t windowSize);
    
    void addSample(float value);
    float getAverage() const;
    void reset();
    size_t getSampleCount() const;
    
private:
    std::vector<float> buffer_;
    size_t maxSize_;
    float sum_;
};
```

#### Step 2.2: Create Filters Implementation
Create `src/filters.cpp`:

```cpp
#include "filters.h"

MovingAverage::MovingAverage(size_t windowSize)
    : maxSize_(windowSize)
    , sum_(0.0f) {
    buffer_.reserve(windowSize);
}

void MovingAverage::addSample(float value) {
    if (buffer_.size() >= maxSize_) {
        sum_ -= buffer_.front();
        buffer_.erase(buffer_.begin());
    }
    buffer_.push_back(value);
    sum_ += value;
}

float MovingAverage::getAverage() const {
    if (buffer_.empty()) return 0.0f;
    return sum_ / buffer_.size();
}

void MovingAverage::reset() {
    buffer_.clear();
    sum_ = 0.0f;
}

size_t MovingAverage::getSampleCount() const {
    return buffer_.size();
}
```

#### Step 2.3: Update variometer_task.cpp
**Before**:
```cpp
static std::vector<float> verticalSpeedHistory;
// ... 10 lines of averaging logic
```

**After**:
```cpp
#include "filters.h"
static MovingAverage vspeedFilter(MOVING_AVERAGE_WINDOW_SIZE);
// ...
vspeedFilter.addSample(rawVerticalSpeed);
float avgVerticalSpeed = vspeedFilter.getAverage();
```

### Verification Checklist for Phase 2
- [ ] Compiles without errors
- [ ] Vertical speed values match previous behavior
- [ ] No `std::vector<float>` for averaging in task files

---

## Phase 3: Configuration Organization (Priority: Low)

### Goal
Split the monolithic `config.h` into domain-specific files.

### Current State
`src/config.h`: 76 constants in one file covering:
- Sound/audio settings
- BLE configuration
- Display layout
- Kalman filter parameters
- Task timing
- Variometer thresholds

### Target Structure
```
src/
├── config/
│   ├── config_all.h      // Master include (includes all sub-configs)
│   ├── audio_config.h    // Sound thresholds, frequencies
│   ├── ble_config.h      // UUIDs, retry intervals
│   ├── display_config.h  // Colors, sizes, layout
│   ├── filter_config.h   // Kalman, moving average params
│   └── task_config.h     // Stack sizes, timing intervals
```

### Implementation Steps

#### Step 3.1: Create audio_config.h
```cpp
#pragma once

// Sound defaults
const bool DEFAULT_SOUND_ENABLED = true;

// Tone generation
const int SPEAKER_DEFAULT_VOLUME = 64;
const int TONE_DURATION_MS = 50;
const int MIN_TONE_FREQ_HZ = 100;
const int MAX_TONE_FREQ_HZ = 4000;

// Rising (climb) tones
const int RISING_TONE_BASE_FREQ_HZ = 1000;
const int RISING_TONE_MULTIPLIER_HZ_PER_MPS = 50;

// Sinking tones
const int SINKING_TONE_BASE_FREQ_HZ = 500;
const int SINKING_TONE_MULTIPLIER_HZ_PER_MPS = 50;
```

#### Step 3.2: Create ble_config.h
```cpp
#pragma once
#include <cstdint>

const uint32_t BLE_RETRY_INTERVAL_MS = 10000;
const bool BLE_SHOW_STATUS_ON_DISPLAY = true;
```

#### Step 3.3: Create display_config.h
```cpp
#pragma once
#include <cstdint>

// Layout dimensions
const int DISPLAY_WIDTH = 320;
const int DISPLAY_HEIGHT = 240;
const int HEADER_HEIGHT = 40;
const int FOOTER_HEIGHT = 30;
const int MAIN_DISPLAY_HEIGHT = 170;

// Update rate
const int DISPLAY_UPDATE_INTERVAL_MS = 500;

// Colors (RGB565)
const uint16_t HEADER_BG_COLOR = 0x1A0E;
const uint16_t FOOTER_BG_COLOR = 0x2A2E;
const uint16_t MUTED_GRAY_COLOR = 0x8410;

// Vertical speed color thresholds
const float VSPEED_CLIMB_THRESHOLD = 0.3f;
const float VSPEED_SINK_THRESHOLD = -0.3f;

// Update thresholds (anti-flicker)
const float ALTITUDE_UPDATE_THRESHOLD = 1.0f;
const float BATTERY_UPDATE_THRESHOLD = 0.1f;
const float VSPEED_UPDATE_THRESHOLD = 0.05f;

// Text sizes
const int HEADER_TEXT_SIZE = 2;
const int MAIN_TEXT_SIZE = 6;
const int FOOTER_TEXT_SIZE = 2;
```

#### Step 3.4: Create filter_config.h
```cpp
#pragma once

// Variometer update rate
const unsigned long VARIOMETER_UPDATE_INTERVAL_MS = 500;

// Kalman filter tuning
const float KALMAN_DT = VARIOMETER_UPDATE_INTERVAL_MS / 1000.0f;
const float KALMAN_PROCESS_NOISE = 0.01f;
const float KALMAN_MEASUREMENT_NOISE = 0.04f;

// Moving average
const int MOVING_AVERAGE_WINDOW_SIZE = 10;

// Altitude calculation constants
const float STANDARD_SEA_LEVEL_PRESSURE_HPA = 1013.25f;
const float ALTITUDE_CONSTANT_A = 44330.0f;
const float ALTITUDE_CONSTANT_B = 5.255f;

// Thresholds
const float ALTITUDE_CHANGE_THRESHOLD_MPS = 0.5f;
```

#### Step 3.5: Create task_config.h
```cpp
#pragma once
#include <cstddef>

// Task stack sizes
const size_t SENSOR_TASK_STACK_SIZE = 8192;
const size_t VARIOMETER_TASK_STACK_SIZE = 4096;
const size_t BUTTON_TASK_STACK_SIZE = 2048;
const size_t TOUCH_TASK_STACK_SIZE = 4096;

// Task timing
const int VARIOMETER_TASK_DELAY_MS = 250;
const int BUTTON_TASK_DELAY_MS = 50;
const int TOUCH_TASK_DELAY_MS = 20;

// Touch/zoom
const int MIN_ZOOM_LEVEL = 1;
const int DEFAULT_MAP_ZOOM_LEVEL = 15;
const int MAX_ZOOM_LEVEL = 19;
const int ZOOM_THRESHOLD = 50;
const int DOUBLE_TAP_THRESHOLD_MS = 300;
```

#### Step 3.6: Create config_all.h
```cpp
#pragma once

// Master config include - use this to get all settings
#include "config/audio_config.h"
#include "config/ble_config.h"
#include "config/display_config.h"
#include "config/filter_config.h"
#include "config/task_config.h"
```

#### Step 3.7: Update existing files
Replace:
```cpp
#include "config.h"
```
With either:
```cpp
#include "config/config_all.h"  // If needs everything
// Or specific:
#include "config/display_config.h"  // gui.cpp only needs display
```

### Verification Checklist for Phase 3
- [ ] All files compile
- [ ] No duplicate constant definitions
- [ ] Each source file includes only needed configs

---

## Phase 4: Initialization Consolidation (Priority: Low)

### Goal
Group initialization calls logically in `main.cpp`.

### Current State (setup function)
```cpp
void setup() {
    Serial.begin(115200);
    // ... battery read ...
    xSensorMutex = xSemaphoreCreateMutex();  // Data init
    initializeM5Stack();                      // Hardware
    startupScreen();                          // UI
    initSound();                              // Audio
    initSensor();                             // Hardware
    initializeSensorTask();                   // Task
    initVariometerTask();                     // Logic init
    initializeVariometerTask();               // Task (confusing name!)
    initializeBLE();                          // Task
    initializeDisplay();                      // UI
}
```

### Target State
```cpp
void setup() {
    initHardware();     // M5Stack, sensor probe
    initDataStore();    // DataStore singleton
    initProcessing();   // Filters (Kalman, moving avg)
    initTasks();        // All FreeRTOS tasks
    initUI();           // Display, sound
}
```

### Implementation Steps

#### Step 4.1: Create initialization groups
Add to `main.cpp`:

```cpp
void initHardware() {
    Serial.begin(115200);
    initializeM5Stack();
    initSensor();
    ESP_LOGI("main", "Hardware initialized");
}

void initDataStore() {
    // DataStore singleton auto-initializes on first access
    // Just trigger it here to ensure early creation
    DataStore::getInstance();
    ESP_LOGI("main", "DataStore initialized");
}

void initProcessing() {
    initVariometerFilters();  // Rename from initVariometerTask()
    ESP_LOGI("main", "Processing initialized");
}

void initTasks() {
    initializeSensorTask();
    initializeVariometerTask();
    initializeBLE();
    ESP_LOGI("main", "Tasks initialized");
}

void initUI() {
    startupScreen();
    initSound();
    initializeDisplay();
    ESP_LOGI("main", "UI initialized");
}
```

#### Step 4.2: Rename confusing functions
- `initVariometerTask()` in `variometer_task.cpp` → `initVariometerFilters()`
- `initializeVariometerTask()` in `main.cpp` → remains (creates FreeRTOS task)

#### Step 4.3: Update setup()
```cpp
void setup() {
    initHardware();
    initDataStore();
    initProcessing();
    initTasks();
    initUI();
}
```

### Verification Checklist for Phase 4
- [ ] Device boots correctly
- [ ] All tasks start
- [ ] Logs show proper initialization order

---

## Implementation Timeline

| Phase | Description | Est. Time | Dependencies |
|-------|-------------|-----------|--------------|
| 1 | DataStore singleton | 3-4 hours | None |
| 2 | MovingAverage class | 1-2 hours | None |
| 3 | Config file split | 1-2 hours | None |
| 4 | Init consolidation | 1-2 hours | Phase 1 |

**Recommended Order**: Phase 1 → Phase 2 → Phase 3 → Phase 4

Phases 1-3 are independent and could be done in parallel by different developers.

---

## Testing Strategy

### Unit Tests to Add
1. `test_data_store.cpp` - Thread safety, snapshot consistency
2. `test_moving_average.cpp` - Edge cases (empty, single sample, overflow)
3. `test_kalman_filter.cpp` - Already exists, verify unchanged behavior

### Integration Tests
1. Full boot sequence with new initialization
2. BLE transmission with DataStore
3. Display updates with DataStore snapshot

### Manual Verification
1. XCTrack pairing and data reception
2. Sound toggle functionality
3. Display refresh rates
4. Battery reading accuracy

---

## Rollback Plan

Each phase creates new files without immediately deleting old code:

1. **Phase 1**: Keep `extern` variables as deprecated aliases initially
2. **Phase 2**: Keep old vector code commented until verified
3. **Phase 3**: Keep original `config.h` as backup
4. **Phase 4**: Original `setup()` preserved in comments

Final cleanup removes deprecated code after full verification.

---

## Success Metrics

| Metric | Before | After |
|--------|--------|-------|
| `extern` declarations | 8+ | 1 (DataStore only) |
| Manual mutex operations | 12+ | 0 (hidden in DataStore) |
| Lines in config.h | 76 | 0 (split into 5 files) |
| Init calls in setup() | 10 | 5 (grouped) |
| Reusable filter classes | 1 | 2 (Kalman + MovingAverage) |
