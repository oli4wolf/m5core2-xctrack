# Complexity Reduction Plan - Implementation Variants

This document outlines alternative technical approaches for each phase in the [`complexity_reduction_plan.md`](../spec/complexity_reduction_plan.md).

---

## Phase 1: Data Access Layer - Variants

### Variant A: DataStore Singleton (Current Plan)
A centralized singleton class with internal mutex protection.

**Pros:**
- Simple API - just call `DataStore::getInstance().setSensorData()`
- All mutex logic hidden from consumers
- Easy to reason about ownership

**Cons:**
- Singleton pattern can complicate unit testing
- Single point of contention for all data access
- Global state still exists, just encapsulated

### Variant B: FreeRTOS Message Queues

Replace shared variables with dedicated queues for each data type.

```cpp
// In main.cpp or data_queues.h
QueueHandle_t xSensorQueue;      // SensorData items
QueueHandle_t xVariometerQueue;  // VariometerData items

// In sensor_task.cpp
SensorData data = {pressure, temperature};
xQueueOverwrite(xSensorQueue, &data);  // Always keep latest

// In variometer_task.cpp
SensorData sensor;
xQueuePeek(xSensorQueue, &sensor, portMAX_DELAY);
```

**Pros:**
- Native FreeRTOS primitive - well understood by embedded developers
- No mutex needed - queue handles synchronization
- `xQueueOverwrite` perfect for latest-value-wins semantics
- Can add multi-consumer support easily

**Cons:**
- Multiple queues to manage
- Slightly more verbose API
- Need to initialize queues before tasks start

### Variant C: Atomic Variables with Lock-Free Reads

Use atomic types for simple float values where platform supports it.

```cpp
#include <atomic>

// For 32-bit floats on ESP32, atomic operations are available
struct AtomicFlightData {
    std::atomic<float> pressure{0.0f};
    std::atomic<float> temperature{0.0f};
    std::atomic<float> altitude{0.0f};
    std::atomic<float> verticalSpeed{0.0f};
    std::atomic<int32_t> batteryLevel{0};
};

extern AtomicFlightData flightData;

// In sensor_task.cpp
flightData.pressure.store(pressure, std::memory_order_relaxed);
flightData.temperature.store(temperature, std::memory_order_relaxed);

// In variometer_task.cpp - no mutex needed!
float currentPressure = flightData.pressure.load(std::memory_order_relaxed);
```

**Pros:**
- Zero mutex overhead for reads/writes
- Very fast - ideal for high-frequency sensor data
- Simple mental model - just read/write

**Cons:**
- No atomic snapshot of multiple values together
- Requires careful ordering if values depend on each other
- May need memory barriers for strict consistency

### Variant D: Event Group with Data Payload

Use FreeRTOS Event Groups combined with a shared data structure.

```cpp
// Event bits for data freshness
#define SENSOR_DATA_READY    (1 << 0)
#define VARIO_DATA_READY     (1 << 1)
#define BATTERY_DATA_READY   (1 << 2)

EventGroupHandle_t xDataEvents;

// Producer sets bits after updating
xEventGroupSetBits(xDataEvents, SENSOR_DATA_READY);

// Consumer waits for specific data
EventBits_t bits = xEventGroupWaitBits(xDataEvents, 
    SENSOR_DATA_READY | VARIO_DATA_READY, 
    pdTRUE,   // Clear on read
    pdFALSE,  // Any bit
    portMAX_DELAY);
```

**Pros:**
- Tasks can wait for data freshness
- Supports complex synchronization patterns
- Good for event-driven architectures

**Cons:**
- Still needs mutex for actual data access
- More complex than needed for simple data sharing
- Overkill for current requirements

### Recommendation for Phase 1

| Criterion | Singleton | Queues | Atomic | Events |
|-----------|-----------|--------|--------|--------|
| Simplicity | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐ |
| Performance | ⭐⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| Snapshot support | ⭐⭐⭐ | ⭐ | ⭐ | ⭐⭐ |
| Testability | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| FreeRTOS idiomatic | ⭐⭐ | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ |

**Best choice:** Variant A - Singleton for this project because:
1. The `getSnapshot()` feature is valuable for atomic display updates
2. Lower complexity for a small team/solo project
3. Current codebase already uses mutex pattern

**Alternative winner:** Variant B - Queues if you want more FreeRTOS-native approach

---

## Phase 2: Extract Reusable Filters - Variants

### Variant A: MovingAverage Class (Current Plan)
Dedicated class with encapsulated buffer and running sum.

**Pros:**
- Efficient O(1) running sum calculation
- Clear, reusable API
- Easy to unit test

**Cons:**
- Allocates heap memory for vector
- Fixed window size after construction

### Variant B: Template-Based Circular Buffer

```cpp
template<size_t WindowSize>
class MovingAverageFixed {
public:
    void addSample(float value) {
        sum_ -= buffer_[index_];
        buffer_[index_] = value;
        sum_ += value;
        index_ = (index_ + 1) % WindowSize;
        if (count_ < WindowSize) count_++;
    }
    
    float getAverage() const {
        return count_ > 0 ? sum_ / count_ : 0.0f;
    }

private:
    float buffer_[WindowSize] = {0};
    size_t index_ = 0;
    size_t count_ = 0;
    float sum_ = 0.0f;
};

// Usage - no heap allocation!
MovingAverageFixed<10> vspeedFilter;
```

**Pros:**
- Zero heap allocation - stack-based array
- Compile-time window size - no runtime checks
- Slightly faster - no vector overhead

**Cons:**
- Window size fixed at compile time
- Template bloat if many different sizes used

### Variant C: Exponential Moving Average - EMA

```cpp
class ExponentialMovingAverage {
public:
    explicit ExponentialMovingAverage(float alpha) : alpha_(alpha) {}
    
    void addSample(float value) {
        if (!initialized_) {
            ema_ = value;
            initialized_ = true;
        } else {
            ema_ = alpha_ * value + (1.0f - alpha_) * ema_;
        }
    }
    
    float getAverage() const { return ema_; }
    
private:
    float alpha_;      // Smoothing factor 0-1
    float ema_ = 0.0f;
    bool initialized_ = false;
};

// Usage - alpha=0.2 gives ~5-sample equivalent smoothing
ExponentialMovingAverage vspeedFilter(0.2f);
```

**Pros:**
- O(1) memory - only stores single value
- Responds to trends faster than simple MA
- Very computationally efficient

**Cons:**
- Different smoothing behavior than simple moving average
- Alpha tuning needed for desired response
- Harder to reason about equivalent window size

### Variant D: Median Filter

```cpp
#include <algorithm>

class MedianFilter {
public:
    explicit MedianFilter(size_t windowSize);
    
    void addSample(float value);
    float getMedian() const;
    
private:
    std::vector<float> buffer_;
    size_t maxSize_;
};

float MedianFilter::getMedian() const {
    if (buffer_.empty()) return 0.0f;
    
    std::vector<float> sorted = buffer_;
    std::sort(sorted.begin(), sorted.end());
    
    size_t mid = sorted.size() / 2;
    if (sorted.size() % 2 == 0) {
        return (sorted[mid - 1] + sorted[mid]) / 2.0f;
    }
    return sorted[mid];
}
```

**Pros:**
- Excellent for removing outliers/spikes
- Robust to noise
- Better for non-Gaussian noise

**Cons:**
- O(n log n) per sample due to sorting
- More memory than EMA
- May lag behind rapid changes

### Recommendation for Phase 2

| Criterion | MovingAvg | FixedMA | EMA | Median |
|-----------|-----------|---------|-----|--------|
| Memory | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| CPU | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | ⭐ |
| Spike rejection | ⭐⭐ | ⭐⭐ | ⭐ | ⭐⭐⭐ |
| Simplicity | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| Predictability | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ |

**Best choice:** Variant B - Template Fixed MA for this project because:
1. Known fixed window size at compile time
2. No heap allocation on embedded system
3. Identical behavior to current implementation

**Alternative winner:** Variant C - EMA if memory is constrained or response time is critical

---

## Phase 3: Configuration Organization - Variants

### Variant A: Split Header Files (Current Plan)
Separate `.h` files in a `config/` subdirectory.

**Pros:**
- Clear separation of concerns
- Files can include only what they need
- Standard C++ approach

**Cons:**
- Multiple files to maintain
- Need master include for convenience

### Variant B: Namespaced Config in Single File

```cpp
// config.h
#pragma once

namespace config {
    namespace audio {
        constexpr bool DEFAULT_SOUND_ENABLED = true;
        constexpr int SPEAKER_VOLUME = 64;
        constexpr int TONE_DURATION_MS = 50;
    }
    
    namespace ble {
        constexpr uint32_t RETRY_INTERVAL_MS = 10000;
        constexpr bool SHOW_STATUS = true;
    }
    
    namespace display {
        constexpr int WIDTH = 320;
        constexpr int HEIGHT = 240;
        constexpr int HEADER_HEIGHT = 40;
    }
    
    namespace filter {
        constexpr float KALMAN_DT = 0.5f;
        constexpr int MOVING_AVG_WINDOW = 10;
    }
}

// Usage
using namespace config::display;
lcd.fillRect(0, 0, WIDTH, HEADER_HEIGHT, color);
```

**Pros:**
- Single file - simpler project structure
- Namespaces prevent collisions
- `constexpr` enables compile-time evaluation

**Cons:**
- Still one large file
- Full rebuild on any config change

### Variant C: Constexpr Struct Pattern

```cpp
// config.h
#pragma once

struct AudioConfig {
    static constexpr bool DefaultSoundEnabled = true;
    static constexpr int SpeakerVolume = 64;
    static constexpr int ToneDurationMs = 50;
};

struct DisplayConfig {
    static constexpr int Width = 320;
    static constexpr int Height = 240;
    static constexpr int HeaderHeight = 40;
    static constexpr int FooterHeight = 30;
};

struct FilterConfig {
    static constexpr float KalmanDt = 0.5f;
    static constexpr int MovingAvgWindow = 10;
};

// Usage
lcd.fillRect(0, 0, DisplayConfig::Width, DisplayConfig::HeaderHeight, color);
```

**Pros:**
- Groups related values clearly
- Type-safe - can pass `AudioConfig` to functions
- IDE autocomplete works well

**Cons:**
- Verbose syntax for access
- Static members can be awkward

### Variant D: INI/JSON Runtime Configuration

```cpp
// config.json on SPIFFS
{
    "audio": {"enabled": true, "volume": 64},
    "display": {"updateRate": 500},
    "filter": {"kalmanNoise": 0.01}
}

// ConfigManager.h
class ConfigManager {
public:
    static ConfigManager& getInstance();
    
    bool loadFromFile(const char* path);
    
    int getAudioVolume() const;
    int getDisplayUpdateRate() const;
    // ...
    
private:
    JsonDocument config_;
};
```

**Pros:**
- Runtime configurable - no recompile
- User can tune parameters
- Can save user preferences

**Cons:**
- Runtime overhead for parsing
- Need filesystem support
- More complex error handling

### Recommendation for Phase 3

| Criterion | Split Files | Namespaces | Structs | Runtime |
|-----------|-------------|------------|---------|---------|
| Compile time | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | ⭐ |
| Flexibility | ⭐⭐ | ⭐⭐ | ⭐⭐ | ⭐⭐⭐ |
| Simplicity | ⭐⭐ | ⭐⭐⭐ | ⭐⭐ | ⭐ |
| Type safety | ⭐⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐ |
| IDE support | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |

**Best choice:** Variant B - Namespaces for this project because:
1. Single file simplicity
2. Modern C++ - `constexpr` values
3. Logical grouping without file proliferation

**Alternative winner:** Variant A - Split Files if team prefers file-per-domain

---

## Phase 4: Initialization Consolidation - Variants

### Variant A: Grouped Functions (Current Plan)
Separate initialization functions called sequentially from `setup()`.

**Pros:**
- Clear phases visible in setup()
- Easy to add logging per phase
- Simple to understand

**Cons:**
- Still sequential - no parallelism
- Order dependencies not explicit

### Variant B: State Machine Initialization

```cpp
enum class InitState {
    HARDWARE,
    DATA_STORE,
    TASKS,
    UI,
    COMPLETE,
    ERROR
};

class SystemInitializer {
public:
    bool advance() {
        switch (state_) {
            case InitState::HARDWARE:
                if (initHardware()) state_ = InitState::DATA_STORE;
                else state_ = InitState::ERROR;
                break;
            // ... other states
        }
        return state_ == InitState::COMPLETE;
    }
    
    InitState getState() const { return state_; }
    
private:
    InitState state_ = InitState::HARDWARE;
    bool initHardware();
    bool initDataStore();
    // ...
};

// In setup()
SystemInitializer init;
while (!init.advance()) {
    updateLoadingScreen(init.getState());
}
```

**Pros:**
- Explicit state transitions
- Can show progress on screen
- Easy to retry failed steps

**Cons:**
- More complex structure
- Overkill for simple boot sequence

### Variant C: Dependency Injection Pattern

```cpp
struct SystemComponents {
    DataStore* dataStore;
    SensorTask* sensorTask;
    VariometerTask* varioTask;
    BLETask* bleTask;
    Display* display;
};

SystemComponents initializeSystem() {
    SystemComponents sys;
    
    // Create in dependency order
    sys.dataStore = new DataStore();
    sys.sensorTask = new SensorTask(sys.dataStore);
    sys.varioTask = new VariometerTask(sys.dataStore, sys.sensorTask);
    sys.bleTask = new BLETask(sys.dataStore);
    sys.display = new Display(sys.dataStore);
    
    return sys;
}

// In setup()
auto system = initializeSystem();
system.sensorTask->start();
system.varioTask->start();
// ...
```

**Pros:**
- Dependencies explicit in constructors
- Easy to test with mocks
- Clear ownership model

**Cons:**
- Significant refactoring needed
- Heap allocations for all components
- More object-oriented than current style

### Variant D: Builder Pattern

```cpp
class SystemBuilder {
public:
    SystemBuilder& withHardware() {
        initializeM5Stack();
        initSensor();
        return *this;
    }
    
    SystemBuilder& withDataStore() {
        DataStore::getInstance();
        return *this;
    }
    
    SystemBuilder& withTasks() {
        initializeSensorTask();
        initializeVariometerTask();
        initializeBLE();
        return *this;
    }
    
    SystemBuilder& withUI() {
        initializeDisplay();
        initSound();
        return *this;
    }
    
    void build() {
        ESP_LOGI("System", "Initialization complete");
    }
};

// In setup()
SystemBuilder()
    .withHardware()
    .withDataStore()
    .withTasks()
    .withUI()
    .build();
```

**Pros:**
- Fluent API - readable setup
- Order is explicit in call chain
- Easy to skip optional components

**Cons:**
- Adds class overhead
- May confuse embedded developers
- Doesn't add much value for current size

### Recommendation for Phase 4

| Criterion | Grouped | StateMachine | DI | Builder |
|-----------|---------|--------------|-----|---------|
| Simplicity | ⭐⭐⭐ | ⭐⭐ | ⭐ | ⭐⭐ |
| Testability | ⭐⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| Visibility | ⭐⭐ | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ |
| Flexibility | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| Embedded fit | ⭐⭐⭐ | ⭐⭐ | ⭐ | ⭐⭐ |

**Best choice:** Variant A - Grouped Functions for this project because:
1. Least invasive change
2. Familiar pattern for embedded developers
3. Sufficient for current complexity level

**Alternative winner:** Variant B - State Machine if you want loading screen progress

---

## Summary: Recommended Variant Selection

| Phase | Recommended | Alternative |
|-------|-------------|-------------|
| Phase 1: Data Access | **A: DataStore Singleton** | B: FreeRTOS Queues |
| Phase 2: Filters | **B: Template Fixed MA** | C: Exponential MA |
| Phase 3: Config | **B: Namespaces** | A: Split Files |
| Phase 4: Init | **A: Grouped Functions** | B: State Machine |

---

## Quick Reference: Variant Selection Criteria

Choose based on your priorities:

- **Simplicity first:** A-B-B-A
- **Performance first:** C-B-B-A (atomic + fixed buffer + constexpr)
- **Testability first:** B-A-C-C (queues + heap MA + structs + DI)
- **FreeRTOS idiomatic:** B-B-B-B (queues everywhere)
- **Future flexibility:** D-C-D-C (events + EMA + runtime config + DI)
