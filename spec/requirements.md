# Requirements Specification

## Functional Requirements

### FR-1: Sensor Data Acquisition
**Description**: The system shall acquire barometric pressure and temperature data from MS5637 sensor
**Priority**: High
**Acceptance Criteria**:
- I2C communication at 400kHz
- Pressure readings in range 800-1200 hPa
- Temperature readings in range -40°C to +85°C
- Update rate of 5Hz (200ms intervals)
- Data protected by mutex

### FR-2: Altitude Calculation
**Description**: The system shall calculate altitude from pressure using standard atmosphere model
**Priority**: High
**Acceptance Criteria**:
- Formula: h = 44330 * (1 - (P/P0)^(1/5.255))
- P0 = 1013.25 hPa (sea level pressure)
- Output in meters with 0.1m resolution
- Real-time calculation without blocking

### FR-3: Vertical Speed Calculation
**Description**: The system shall calculate vertical speed from altitude changes
**Priority**: High
**Acceptance Criteria**:
- Moving average filter with 10 samples
- Update rate of 2Hz (500ms intervals)
- Output in m/s with 0.01m/s resolution
- Threshold detection at ±0.5 m/s

### FR-4: BLE Data Transmission
**Description**: The system shall transmit flight data to XCTrack via BLE UART
**Priority**: High
**Acceptance Criteria**:
- LK8EX1 protocol: $LK8EX1,999999,<alt>,<climb>,<temp>,<bat>*
- Transmission rate of 10Hz (100ms intervals)
- XOR checksum calculation
- MTU size 46 bytes
- Automatic reconnection on disconnect

### FR-5: LCD Display
**Description**: The system shall display real-time flight data on LCD screen
**Priority**: Medium
**Acceptance Criteria**:
- Update rate of 0.5Hz (2000ms intervals)
- Display format: Pressure, Temperature, Altitude, V-Speed, Battery
- TFT_BLACK background, TFT_WHITE text
- Text size 2, clear layout

### FR-6: Audio Variometer (Optional)
**Description**: The system may provide audible feedback for vertical speed
**Priority**: Low
**Acceptance Criteria**:
- Rising tones: 1000Hz base + 50Hz/m/s
- Sinking tones: 500Hz base - 50Hz/m/s
- Tone duration 50ms
- Frequency limits 100-2000Hz
- Configurable enable/disable

### FR-7: Battery Monitoring
**Description**: The system shall monitor and report battery voltage
**Priority**: Medium
**Acceptance Criteria**:
- Voltage reading via M5.Power.getBatteryVoltage()
- Display on LCD and BLE transmission
- No automatic shutdown (monitoring only)

## Non-Functional Requirements

### NFR-1: Performance
**Description**: The system shall maintain real-time performance
**Priority**: High
**Acceptance Criteria**:
- BLE transmission latency < 100ms
- Sensor reading jitter < 10ms
- CPU utilization < 80% per core
- Memory usage stable over time

### NFR-2: Reliability
**Description**: The system shall operate reliably in flight conditions
**Priority**: High
**Acceptance Criteria**:
- No crashes during 4-hour continuous operation
- Graceful handling of sensor failures
- Automatic BLE reconnection
- Data integrity with mutex protection

### NFR-3: Power Efficiency
**Description**: The system shall minimize power consumption
**Priority**: Medium
**Acceptance Criteria**:
- Average current draw < 200mA
- Efficient task scheduling
- No unnecessary computations
- Low-power BLE operation

### NFR-4: Accuracy
**Description**: Sensor data shall meet specified accuracy requirements
**Priority**: High
**Acceptance Criteria**:
- Temperature accuracy ±0.5°C
- Altitude accuracy ±20cm
- Vertical speed accuracy ±0.1 m/s
- Timing accuracy ±10ms

### NFR-5: Maintainability
**Description**: Code shall be maintainable and configurable
**Priority**: Medium
**Acceptance Criteria**:
- All constants in config.h
- Clear separation of concerns
- Comprehensive logging (ESP_LOG levels)
- Modular task structure

### NFR-6: Compatibility
**Description**: System shall be compatible with XCTrack and M5Stack
**Priority**: High
**Acceptance Criteria**:
- LK8EX1 protocol compliance
- M5Stack Core2 hardware support
- PlatformIO build system
- Arduino framework compatibility

### NFR-7: Safety
**Description**: System shall not pose safety risks to users
**Priority**: High
**Acceptance Criteria**:
- No interference with flight operations
- Reliable operation in environmental conditions
- Graceful failure modes
- No hazardous audio levels

### NFR-8: Usability
**Description**: System shall be usable by paraglider pilots
**Priority**: Medium
**Acceptance Criteria**:
- Clear LCD display layout
- Intuitive audio feedback
- Minimal configuration required
- Robust in field conditions

## Interface Requirements

### IR-1: Hardware Interfaces
- **MS5637 Sensor**: I2C, SDA=GPIO21, SCL=GPIO22, 400kHz
- **LCD Display**: Internal M5Stack display, 320x240 resolution
- **Speaker**: Internal M5Stack speaker, PWM output
- **Power**: Battery input via M5Stack power management

### IR-2: Software Interfaces
- **BLE UART**: Custom service UUID 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
- **LK8EX1 Protocol**: NMEA-style sentences with XOR checksum
- **XCTrack**: BLE UART receiver, expects 10Hz data stream
- **Libraries**: M5Unified, M5GFX, SparkFun MS5637, TinyGPSPlus

### IR-3: User Interfaces
- **Visual**: LCD screen with real-time data display
- **Auditory**: Optional tone feedback for vertical speed
- **BLE**: Wireless connection to XCTrack app

## Constraints

### CON-1: Hardware Constraints
- ESP32 dual-core processor
- Limited RAM and flash memory
- Battery-powered operation
- Environmental operating range

### CON-2: Software Constraints
- FreeRTOS real-time operating system
- Arduino framework compatibility
- PlatformIO build system
- No dynamic memory allocation in critical paths

### CON-3: Regulatory Constraints
- Non-commercial hobby project
- No certification requirements
- Open source licensing
- Community contribution guidelines

## Assumptions

### ASS-1: Environmental Assumptions
- Operating temperature: -10°C to +40°C
- Altitude range: 0-5000m
- Humidity: Non-condensing
- Vibration: Typical paragliding conditions

### ASS-2: User Assumptions
- Basic technical knowledge for setup
- XCTrack app installed and configured
- BLE device pairing capability
- Visual/audio feedback interpretation

### ASS-3: System Assumptions
- Stable power supply during operation
- Valid sensor calibration
- XCTrack compatibility maintained
- No electromagnetic interference