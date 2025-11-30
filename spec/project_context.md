# M5Core2 XCTrack Project Context

## Purpose
Portable variometer device for paragliding using M5Stack Core2 hardware, providing real-time altitude, vertical speed, and flight data compatible with XCTrack via BLE.

## Project Info
- **Type**: Non-commercial hobby project
- **Version**: 0.1
- **Author**: @oli4wolf
- **Repository**: https://github.com/oli4wolf/m5core2-xctrack
- **Platform**: PlatformIO with ESP32 Arduino framework

## Core Technology
- **Hardware**: M5Stack Core2 (ESP32) + MS5637 barometric sensor
- **Software**: FreeRTOS multi-task system with Arduino framework
- **Communication**: BLE UART (LK8EX1 protocol)

## Dependencies
- M5Unified: https://github.com/M5Stack/M5Unified.git
- M5GFX: https://github.com/M5Stack/M5GFX.git
- SparkFun MS5637: sparkfun/SparkFun MS5637 Barometric Pressure Library@^1.0.2

## Known Issues
- Audio variometer disabled by default
- No BLE reconnection error recovery
- Limited flight testing

## Future Enhancements
- Enable audio variometer
- SD card flight logging
- Touchscreen configuration menu
- Battery warnings