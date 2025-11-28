# Testing Specifications

## Unit Testing

### Sensor Task Testing
**Test Case: UT-1.1 - Sensor Initialization**
- **Input**: Power on, valid I2C bus
- **Expected Output**: MS5637.begin() returns true
- **Test Method**: Mock I2C responses, verify initialization sequence
- **Pass Criteria**: Sensor initialized without errors

**Test Case: UT-1.2 - Pressure Reading**
- **Input**: Valid sensor data
- **Expected Output**: Pressure value in hPa range (800-1200)
- **Test Method**: Mock sensor responses, verify data parsing
- **Pass Criteria**: Pressure values within valid range

**Test Case: UT-1.3 - Mutex Protection**
- **Input**: Concurrent access attempts
- **Expected Output**: Mutex acquired/released correctly
- **Test Method**: Multi-threaded test with timing
- **Pass Criteria**: No data corruption, proper synchronization

### Variometer Task Testing
**Test Case: UT-2.1 - Altitude Calculation**
- **Input**: Pressure = 1013.25 hPa (sea level)
- **Expected Output**: Altitude = 0.0 m
- **Test Method**: Unit test pressureToAltitude function
- **Pass Criteria**: Altitude calculation matches formula

**Test Case: UT-2.2 - Moving Average Filter**
- **Input**: Sequence of altitude values
- **Expected Output**: Smoothed altitude output
- **Test Method**: Input known sequence, verify filter output
- **Pass Criteria**: Filter response matches expected smoothing

**Test Case: UT-2.3 - Vertical Speed Calculation**
- **Input**: Altitude change over time
- **Expected Output**: Correct velocity in m/s
- **Test Method**: Simulate altitude changes, verify V-speed
- **Pass Criteria**: Velocity calculation accurate to 0.1 m/s

### BLE UART Testing
**Test Case: UT-3.1 - LK8EX1 Checksum**
- **Input**: Sample LK8EX1 sentence
- **Expected Output**: Correct XOR checksum
- **Test Method**: Test checksum calculation function
- **Pass Criteria**: Checksum matches specification

**Test Case: UT-3.2 - BLE Transmission**
- **Input**: Altitude, climb rate, battery data
- **Expected Output**: Properly formatted NMEA sentence
- **Test Method**: Mock BLE interface, verify transmission
- **Pass Criteria**: Sentence format and data correct

## Integration Testing

### System Integration Testing
**Test Case: IT-1.1 - End-to-End Data Flow**
- **Input**: Real sensor data
- **Expected Output**: BLE transmission to XCTrack
- **Test Method**: Connect to XCTrack app, verify data reception
- **Pass Criteria**: XCTrack receives valid flight data

**Test Case: IT-1.2 - Task Synchronization**
- **Input**: All tasks running
- **Expected Output**: No mutex deadlocks, proper data sharing
- **Test Method**: Run system for extended period, monitor logs
- **Pass Criteria**: No synchronization errors in logs

**Test Case: IT-1.3 - LCD Display Update**
- **Input**: Changing sensor values
- **Expected Output**: LCD updates with current data
- **Test Method**: Observe display during operation
- **Pass Criteria**: Display shows accurate, updating data

### Hardware Integration Testing
**Test Case: IT-2.1 - MS5637 Sensor Integration**
- **Input**: Physical sensor connected
- **Expected Output**: Valid pressure/temperature readings
- **Test Method**: Compare with reference barometer
- **Pass Criteria**: Readings within ±0.5°C, ±20cm altitude

**Test Case: IT-2.2 - BLE Connectivity**
- **Input**: XCTrack app in range
- **Expected Output**: Successful connection and data transfer
- **Test Method**: Pair with XCTrack, monitor connection
- **Pass Criteria**: Stable BLE connection, data transmission

**Test Case: IT-2.3 - Power Consumption**
- **Input**: System operating normally
- **Expected Output**: Current draw < 200mA average
- **Test Method**: Measure current consumption
- **Pass Criteria**: Within power budget

## System Testing

### Functional Testing
**Test Case: ST-1.1 - Startup Sequence**
- **Input**: Power button press
- **Expected Output**: Successful initialization, startup screen
- **Test Method**: Power cycle device, observe sequence
- **Pass Criteria**: All components initialize correctly

**Test Case: ST-1.2 - Flight Simulation**
- **Input**: Simulated altitude changes
- **Expected Output**: Correct variometer response
- **Test Method**: Manual altitude simulation, verify audio/visual feedback
- **Pass Criteria**: System responds appropriately to climb/sink

**Test Case: ST-1.3 - XCTrack Integration**
- **Input**: Connected to XCTrack
- **Expected Output**: Flight data recorded
- **Test Method**: Complete flight simulation, check XCTrack logs
- **Pass Criteria**: Complete flight track recorded

### Performance Testing
**Test Case: ST-2.1 - Timing Accuracy**
- **Input**: System running for 1 hour
- **Expected Output**: Update rates within ±10%
- **Test Method**: Timestamp transmissions, analyze intervals
- **Pass Criteria**: All timing requirements met

**Test Case: ST-2.2 - Memory Usage**
- **Input**: Extended operation
- **Expected Output**: No memory leaks
- **Test Method**: Monitor heap usage over time
- **Pass Criteria**: Stable memory usage, no leaks

**Test Case: ST-2.3 - CPU Utilization**
- **Input**: All tasks active
- **Expected Output**: CPU usage < 80% per core
- **Test Method**: Profile system during operation
- **Pass Criteria**: Within performance limits

### Reliability Testing
**Test Case: ST-3.1 - Long Duration Operation**
- **Input**: Continuous operation for 4 hours
- **Expected Output**: No crashes or failures
- **Test Method**: Run system unattended, monitor logs
- **Pass Criteria**: System operates without intervention

**Test Case: ST-3.2 - Environmental Stress**
- **Input**: Temperature variations (-10°C to +40°C)
- **Expected Output**: Reliable operation
- **Test Method**: Temperature chamber testing
- **Pass Criteria**: Performance within specifications

**Test Case: ST-3.3 - BLE Robustness**
- **Input**: Intermittent BLE connection
- **Expected Output**: Automatic reconnection
- **Test Method**: Move device in/out of range
- **Pass Criteria**: Seamless connection recovery

## Test Environment

### Hardware Test Setup
- M5Stack Core2 with MS5637 sensor
- BLE-capable device running XCTrack
- Power supply with current monitoring
- Temperature chamber for environmental testing

### Software Test Tools
- PlatformIO for building and uploading
- Serial monitor for log analysis
- BLE scanner for connection verification
- XCTrack app for integration testing
- Custom test scripts for automated testing

### Test Data
- **Pressure Test Data**: Sea level (1013.25 hPa), altitude simulation
- **Temperature Test Data**: Standard range (-10°C to +40°C)
- **BLE Test Data**: Valid LK8EX1 sentences, checksum verification
- **Timing Test Data**: Precise timestamps for interval verification

## Test Reporting
- **Test Results**: Pass/Fail status for each test case
- **Coverage Metrics**: Percentage of code/functions tested
- **Defect Reports**: Detailed bug reports with reproduction steps
- **Performance Metrics**: Timing, memory, CPU usage statistics
- **Regression Testing**: Automated re-testing after fixes

## Continuous Integration
- **Build Verification**: Automated PlatformIO builds
- **Unit Test Execution**: Run unit tests on each commit
- **Static Analysis**: Code quality checks
- **Firmware Deployment**: Automated testing on hardware