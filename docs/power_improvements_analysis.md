# Power Module Code Improvements Analysis

## Original Code (lines 71-74)
```cpp
// Update global state with validated reading
globalBatteryLevel = batteryLevel;
globalChargingState = isCharging;      // TRUE only when actively charging
globalUsbPowerState = usbConnected;    // TRUE when USB power is present
```

## Issues Identified

### 1. Thread Safety (CRITICAL)
- **Problem**: Three global variables updated without mutex protection in a multi-threaded FreeRTOS environment
- **Risk**: Other tasks (sensor, variometer, BLE) could read inconsistent state (e.g., old battery level with new charging state)
- **Evidence**: Other globals (`globalPressure`, `globalTemperature`, `globalAltitude_m`) all use mutex protection

### 2. Data Coherency
- **Problem**: Individual assignments can be interrupted, creating temporary inconsistent state
- **Risk**: Display could show battery=10%, charging=false, USB=true (impossible state)

### 3. Code Organization
- **Problem**: Three separate global variables instead of structured data
- **Impact**: Harder to maintain, pass to functions, and ensure consistency

### 4. Missing Validation
- **Problem**: No bounds checking before assignment
- **Risk**: Invalid values could propagate (e.g., batteryLevel > 100 or < 0)

### 5. Change Detection Inefficiency
- **Problem**: Change detection only checks `batteryLevel`, but all three values should be considered
- **Impact**: State changes (charging/USB) might not be logged if battery level unchanged

## Recommended Improvements

### Improvement 1: Thread-Safe Struct-Based Approach (RECOMMENDED)

**Benefits:**
- ✅ Thread-safe with mutex protection
- ✅ Atomic updates of all three values
- ✅ Better code organization
- ✅ Validation and bounds checking
- ✅ Consistent with other global variables in the project

**Implementation:**

```cpp
// In power.h
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct BatteryState {
    int32_t level;        // 0-100%
    bool isCharging;      // TRUE when actively charging
    bool usbConnected;    // TRUE when USB power present
};

extern SemaphoreHandle_t xBatteryMutex;
extern BatteryState globalBatteryState;

// Thread-safe accessors
void setBatteryState(int32_t level, bool charging, bool usbPower);
BatteryState getBatteryState();
```

```cpp
// In power.cpp (lines 71-74 replacement)
// Update global state with validated reading (thread-safe)
if (xSemaphoreTake(xBatteryMutex, portMAX_DELAY) == pdTRUE) {
    // Validate and clamp battery level to valid range
    int32_t validatedLevel = batteryLevel;
    if (validatedLevel < 0) validatedLevel = 0;
    if (validatedLevel > 100) validatedLevel = 100;
    
    // Check for state changes before updating
    bool stateChanged = (globalBatteryState.level != validatedLevel) ||
                       (globalBatteryState.isCharging != isCharging) ||
                       (globalBatteryState.usbConnected != usbConnected);
    
    // Atomic update of all battery state variables
    globalBatteryState.level = validatedLevel;
    globalBatteryState.isCharging = isCharging;      // TRUE only when actively charging
    globalBatteryState.usbConnected = usbConnected;  // TRUE when USB power is present
    
    xSemaphoreGive(xBatteryMutex);
    
    // Log outside mutex to minimize lock time
    if (stateChanged) {
        ESP_LOGI("Battery", "State change: Level=%ld%%, Charging=%d, USB=%d, Voltage=%.0fmV",
                 validatedLevel, isCharging, usbConnected, voltage);
    }
} else {
    ESP_LOGE("Battery", "Failed to acquire battery mutex for state update");
}
```

### Improvement 2: Minimal Change with Validation (ALTERNATIVE)

**Benefits:**
- ✅ Minimal refactoring required
- ✅ Adds validation
- ✅ Improves maintainability

**Drawbacks:**
- ❌ Still not thread-safe
- ❌ Doesn't address core architectural issues

```cpp
// Update global state with validated reading
// Validate and clamp battery level to valid range [0, 100]
globalBatteryLevel = (batteryLevel < 0) ? 0 : (batteryLevel > 100) ? 100 : batteryLevel;
globalChargingState = isCharging;      // TRUE only when actively charging
globalUsbPowerState = usbConnected;    // TRUE when USB power is present

// Assert invariants in debug builds
#ifdef DEBUG
assert(globalBatteryLevel >= 0 && globalBatteryLevel <= 100);
assert(!(globalChargingState && !globalUsbPowerState)); // Charging implies USB connected
#endif
```

### Improvement 3: Atomic Operations (ESP32-SPECIFIC)

**Benefits:**
- ✅ Lock-free performance
- ✅ Suitable for single reader/writer scenarios

**Drawbacks:**
- ❌ More complex
- ❌ Requires careful ordering
- ❌ Only works reliably for aligned int32_t on ESP32

```cpp
#include <atomic>

// Update global state with validated reading (using atomic operations)
// Note: std::atomic not ideal for struct on ESP32, better to use mutex
std::atomic_store(&globalBatteryLevel, batteryLevel);
std::atomic_store(&globalChargingState, isCharging);
std::atomic_store(&globalUsbPowerState, usbConnected);
```

## Performance Analysis

### Memory Impact
- Current: 3 separate globals = 8 bytes (int32_t + 2 * bool)
- Struct approach: 8 bytes + 4 bytes padding = 12 bytes
- Mutex overhead: ~76 bytes (SemaphoreHandle_t + control block)
- **Total increase**: ~80 bytes (negligible on ESP32)

### CPU Impact
- Mutex lock/unlock: ~10-20 CPU cycles worst case
- Called frequency: Once per loop iteration (~50-200ms)
- **Performance impact**: <0.01% (negligible)

### Lock Contention Risk
- Low: Battery updates happen in main loop (lower frequency than sensor tasks)
- Readers: Main loop display update, button handler
- Writers: Only `updateBatteryStatus()`
- **Contention probability**: Very low

## Recommended Implementation Plan

1. **Phase 1**: Create battery state struct and mutex
2. **Phase 2**: Update `power.cpp` to use mutex-protected updates
3. **Phase 3**: Update consumers (`main.cpp`, `button.cpp`, `gui.cpp`) to use accessors
4. **Phase 4**: Remove old global variables

## Edge Cases to Handle

1. **Battery level out of range**: Clamp to [0, 100]
2. **Inconsistent state**: Validate that `isCharging` implies `usbConnected`
3. **Mutex acquisition failure**: Log error and use last known good state
4. **Initial state**: Handle first read when no previous state exists
5. **Invalid sensor readings**: Already handled by upstream validation (lines 36-63)

## Testing Recommendations

1. Verify thread safety with multiple readers in different tasks
2. Test boundary conditions (0%, 100%, transitions)
3. Verify state consistency under rapid changes
4. Measure performance impact with profiling
5. Test mutex timeout scenarios
