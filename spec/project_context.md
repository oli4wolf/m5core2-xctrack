# M5Core2 XCTrack Project Context

## Purpose
This is a non-commercial hobby project to create a portable variometer device for paragliding using M5Stack Core2 hardware. The device integrates barometric pressure sensors, GPS (not implemented yet), and BLE transmission to provide real-time altitude, vertical speed, and location data compatible with XCTrack flight tracking software.

## Architecture
- **Hardware**: M5Stack Core2 (ESP32-based) with external MS5637 barometric sensor.
- **Software**: FreeRTOS-based multi-task system with Arduino framework
- **Communication**: BLE UART for XCTrack data transmission (LK8EX1 protocol)
- **Display**: LCD screen for real-time sensor data visualization
- **Audio**: Feedback in case the Mobile has issues.

## Key Features
- Barometric altitude and vertical speed calculation
- BLE transmission of flight data at 10Hz
- Real-time LCD display of pressure, temperature, altitude, vertical speed
- Audio variometer with configurable tones (currently disabled)
- Battery voltage monitoring

## Development Status
- Version: 0.1
- Author: @oli4wolf on github
- Platform: PlatformIO with ESP32 Arduino framework
- Build Type: Debug with logging level 3
- Repository: https://github.com/oli4wolf/m5core2-xctrack

## Dependencies
- **M5Unified**: https://github.com/M5Stack/M5Unified.git
- **M5GFX**: https://github.com/M5Stack/M5GFX.git
- **SparkFun MS5637**: sparkfun/SparkFun MS5637 Barometric Pressure Library@^1.0.2
- **TinyGPSPlus**: mikalhart/TinyGPSPlus@^1.1.0

## Build Configuration
- Platform: espressif32
- Board: m5stack-core2
- Framework: arduino
- Monitor Speed: 115200
- Upload Speed: 1500000
- Build Flags: -DCORE_DEBUG_LEVEL=3
- Monitor Filters: esp32_exception_decoder

## Task Structure
- **Main Task**: Initialization, display loop, task coordination
- **Sensor Task**: MS5637 pressure/temperature acquisition (5Hz)
- **Variometer Task**: Altitude/vertical speed calculation (2Hz)
- **BLE Task**: LK8EX1 transmission to XCTrack (10Hz)
- **GPS Task**: Position data acquisition (future, currently disabled)

## Data Flow
MS5637 → Sensor Task → Global Variables → Variometer Task → BLE Task → XCTrack
                                      ↓
                                   LCD Display
                                      ↓
                                 Audio Speaker

## Known Issues
- Audio variometer disabled by default
- No error recovery for BLE disconnection beyond advertising restart
- Limited testing on actual flight conditions

## Future Enhancements
- Enable audio variometer
- Add flight logging to SD card
- Implement configuration menu via touchscreen
- Battery level warnings