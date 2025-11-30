# Testing Specifications

## Unit Testing

### Sensor Task
**UT-1: Sensor Initialization**
- **Input**: Power on, valid I2C bus
- **Expected**: MS5637.begin() returns true
- **Pass Criteria**: Sensor initialized without errors

**UT-2: Pressure Reading**
- **Input**: Valid sensor data
- **Expected**: Pressure 800-1200 hPa
- **Pass Criteria**: Values within range, thread-safe access

### Variometer Task
**UT-3: Altitude Calculation**
- **Input**: Pressure = 1013.25 hPa (sea level)
- **Expected**: Altitude = 0.0m
- **Pass Criteria**: Calculation matches formula ±0.1m

**UT-4: Moving Average Filter**
- **Input**: Sequence of altitude values
- **Expected**: Smoothed altitude output
- **Pass Criteria**: Filter response matches expected smoothing

**UT-5: Vertical Speed Calculation**
- **Input**: Altitude change over time
- **Expected**: Correct velocity in m/s
- **Pass Criteria**: Accuracy ±0.1 m/s

### BLE UART
**UT-6: LK8EX1 Checksum**
- **Input**: Sample LK8EX1 sentence
- **Expected**: Correct XOR checksum
- **Pass Criteria**: Checksum matches specification

**UT-7: BLE Transmission**
- **Input**: Altitude, climb rate, battery data
- **Expected**: Properly formatted NMEA sentence
- **Pass Criteria**: Sentence format and data correct

## Integration Testing

### System Integration
**IT-1: End-to-End Data Flow**
- **Input**: Real sensor data
- **Expected**: BLE transmission to XCTrack
- **Pass Criteria**: XCTrack receives valid flight data

**IT-2: Task Synchronization**
- **Input**: All tasks running for 1 hour
- **Expected**: No mutex deadlocks, proper data sharing
- **Pass Criteria**: No synchronization errors in logs

**IT-3: LCD Display Update**
- **Input**: Changing sensor values
- **Expected**: LCD updates with current data
- **Pass Criteria**: Display shows accurate, updating values

### Hardware Integration
**IT-4: MS5637 Sensor**
- **Input**: Physical sensor connected
- **Expected**: Valid pressure/temperature readings
- **Pass Criteria**: Readings within ±0.5°C, ±20cm altitude

**IT-5: BLE Connectivity**
- **Input**: XCTrack app in range
- **Expected**: Successful connection and data transfer
- **Pass Criteria**: Stable BLE connection, continuous transmission

**IT-6: Power Consumption**
- **Input**: System operating normally
- **Expected**: Current draw < 200mA average
- **Pass Criteria**: Within power budget

## System Testing

### Functional Testing
**ST-1: Startup Sequence**
- **Input**: Power button press
- **Expected**: Successful initialization, startup screen
- **Pass Criteria**: All components initialize correctly

**ST-2: Flight Simulation**
- **Input**: Simulated altitude changes
- **Expected**: Correct variometer response
- **Pass Criteria**: System responds to climb/sink appropriately

**ST-3: XCTrack Integration**
- **Input**: Connected to XCTrack during flight simulation
- **Expected**: Flight data recorded
- **Pass Criteria**: Complete flight track recorded in XCTrack

### Performance Testing
**ST-4: Timing Accuracy**
- **Input**: System running for 1 hour
- **Expected**: Update rates within ±10%
- **Pass Criteria**: All timing requirements met

**ST-5: Memory Stability**
- **Input**: Extended operation
- **Expected**: No memory leaks
- **Pass Criteria**: Stable memory usage over time

**ST-6: CPU Utilization**
- **Input**: All tasks active
- **Expected**: CPU usage < 80% per core
- **Pass Criteria**: Within performance limits

### Reliability Testing
**ST-7: Long Duration Operation**
- **Input**: Continuous operation for 4 hours
- **Expected**: No crashes or failures
- **Pass Criteria**: System operates without intervention

**ST-8: Environmental Stress**
- **Input**: Temperature variations (-10°C to +40°C)
- **Expected**: Reliable operation
- **Pass Criteria**: Performance within specifications

**ST-9: BLE Robustness**
- **Input**: Intermittent BLE connection (move in/out of range)
- **Expected**: Automatic reconnection
- **Pass Criteria**: Seamless connection recovery

## Test Tools
- PlatformIO for building and uploading
- Serial monitor for log analysis
- BLE scanner for connection verification
- XCTrack app for integration testing
- Power measurement equipment