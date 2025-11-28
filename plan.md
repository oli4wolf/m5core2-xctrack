# M5Core2 XCTrack Development Plan

## Current Status Assessment

Based on code analysis, the following components are implemented:
- ✅ Sensor data acquisition (MS5637 pressure/temperature at 5Hz)
- ✅ BLE UART transmission (LK8EX1 protocol at 10Hz)
- ✅ LCD display (real-time data at 0.5Hz)
- ✅ Basic battery monitoring
- ✅ Task structure with FreeRTOS
- ✅ Kalman filter for altitude smoothing
- ✅ Variometer task with altitude calculation and vertical speed moving average

## Missing/Incomplete Components

### Critical Issues
1. **Variometer Task Implementation**: The `variometerTask` function is missing from `variometer_task.cpp`. Only initialization exists.
2. **Altitude Calculation**: No implementation of pressure-to-altitude conversion using the standard atmosphere formula.
3. **Vertical Speed Calculation**: No moving average filter implementation for vertical speed calculation.
4. **Mutex Protection**: Inconsistent mutex usage across global variables.

### Functional Gaps
5. **Audio Variometer**: Currently disabled, needs enable/disable logic and tone generation.
6. **GPS Integration**: TinyGPSPlus library included but GPS task not implemented.
7. **Configuration Menu**: No touchscreen-based configuration interface.
8. **SD Card Logging**: No flight data logging to SD card.
9. **Error Recovery**: Limited BLE reconnection handling.

## Next Steps for Coding Agent

### Phase 1: Core Functionality Completion (Priority: High)

#### 1.1 Complete Variometer Task Implementation ✅ COMPLETED
- **File**: `src/variometer_task.cpp`
- **Task**: Implement `variometerTask()` function with:
  - Altitude calculation using formula: `h = 44330 * (1 - (P/P0)^(1/5.255))`
  - Moving average filter for vertical speed (10 samples, 2Hz update)
  - Kalman filter integration for altitude smoothing
  - Proper mutex protection for global variables
- **Acceptance Criteria**:
  - Altitude calculation with 0.1m resolution
  - Vertical speed with 0.01m/s resolution and ±0.5 m/s threshold detection
  - Thread-safe access to global variables

#### 1.2 Fix Mutex Usage
- **Files**: `src/main.cpp`, `src/sensor_task.cpp`, `src/variometer_task.cpp`, `src/ble_uart.cpp`
- **Task**: Standardize mutex protection for all global variables:
  - `globalPressure`, `globalTemperature` (sensor mutex)
  - `globalAltitude_m`, `globalVerticalSpeed_mps` (variometer mutex)
- **Acceptance Criteria**:
  - All global variable access protected by appropriate mutexes
  - No race conditions in multi-task environment

#### 1.3 Update Configuration Constants
- **File**: `src/config.h`
- **Task**: Add missing constants and clean up unused ones:
  - Kalman filter constants (process noise, measurement noise, etc.)
  - Moving average window size (10 samples)
  - Update intervals for consistency
- **Acceptance Criteria**:
  - All magic numbers moved to config.h
  - Clear documentation for each constant

### Phase 2: Enhanced Features (Priority: Medium)

#### 2.1 Audio Variometer Implementation ✅ COMPLETED
- **Files**: `src/sound.cpp`, `src/sound.h`, `src/config.h`
- **Task**:
  - Implement tone generation logic (rising: 1000Hz + 50Hz/m/s, sinking: 500Hz - 50Hz/m/s)
  - Add enable/disable runtime control
  - Integrate with variometer task
- **Acceptance Criteria**:
  - Tones within 100-2000Hz range
  - 50ms tone duration
  - Configurable enable/disable

#### 2.2 GPS Task Implementation (Future)
- **Files**: `src/gps_task.cpp`, `src/gps_task.h`
- **Task**: Implement GPS position acquisition using TinyGPSPlus
- **Note**: Currently disabled, implement when GPS hardware available

### Phase 3: User Interface Improvements (Priority: Low)

#### 3.1 Touchscreen Configuration Menu
- **Task**: Implement basic touchscreen menu for:
  - Audio variometer enable/disable
  - Display brightness adjustment
  - System status display
- **Acceptance Criteria**:
  - Intuitive touch interface
  - Non-blocking operation

#### 3.2 SD Card Logging
- **Task**: Implement flight data logging to SD card
- **Acceptance Criteria**:
  - CSV format with timestamp, altitude, vertical speed, GPS data
  - Automatic file rotation
  - Error handling for SD card failures

### Phase 4: Testing and Validation (Priority: High)

#### 4.1 Unit Testing
- **Task**: Create test cases for:
  - Altitude calculation accuracy
  - Vertical speed filtering
  - BLE protocol compliance
  - Mutex protection
- **Tools**: Use PlatformIO test framework

#### 4.2 Integration Testing
- **Task**: Test complete system with:
  - XCTrack app connectivity
  - Real flight conditions simulation
  - Battery life testing
  - Environmental testing (-10°C to +40°C)

### Phase 5: Code Quality and Maintenance

#### 5.1 Code Cleanup
- **Task**:
  - Remove duplicate functions in `ble_uart.cpp`
  - Standardize logging levels
  - Add comprehensive comments
  - Fix compilation warnings

#### 5.2 Documentation Updates
- **Task**: Update README.md and spec files with:
  - Build instructions
  - Configuration options
  - Troubleshooting guide

## Implementation Order

1. **Immediate (Blockers)**: Complete variometer task implementation
2. **Short-term**: Fix mutex usage and configuration
3. **Medium-term**: Audio variometer and basic testing
4. **Long-term**: GPS, touchscreen menu, SD logging

## Risk Assessment

- **High Risk**: Incomplete variometer calculations may cause incorrect altitude/vertical speed readings
- **Medium Risk**: Mutex issues could cause data corruption in multi-task environment
- **Low Risk**: Audio and UI features are optional enhancements

## Success Criteria

- All FR-1 through FR-7 functional requirements implemented
- System meets NFR-1 through NFR-4 non-functional requirements
- Successful BLE connection and data transmission to XCTrack
- Stable operation during simulated flight conditions
- Code maintainability with clear separation of concerns