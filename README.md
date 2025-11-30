# M5Core2 XCTrack Variometer

[![PlatformIO CI](https://github.com/oli4wolf/m5core2-xctrack/workflows/PlatformIO%20CI/badge.svg)](https://github.com/oli4wolf/m5core2-xctrack/actions)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.1-orange.svg)](https://github.com/oli4wolf/m5core2-xctrack)

A portable, Bluetooth-enabled variometer for paragliding built on M5Stack Core2 hardware. This device integrates barometric pressure sensors with BLE transmission to provide real-time altitude, vertical speed, and flight data compatible with [XCTrack](https://xctrack.org) flight tracking software.

> ⚠️ **Note**: This is a non-commercial hobby project for educational and personal use.

## Features

- 📊 **Barometric Altitude Calculation** - Accurate altitude measurement using MS5637 sensor with Kalman filtering
- 📈 **Vertical Speed Detection** - Real-time climb/sink rate calculation with moving average smoothing
- 📡 **BLE UART Transmission** - 10Hz data transmission to XCTrack using LK8EX1 protocol
- 🖥️ **LCD Display** - Real-time visualization of pressure, temperature, altitude, and vertical speed
- 🔊 **Audio Variometer** - Configurable tone feedback for climb/sink indication (currently disabled by default)
- 🔋 **Battery Monitoring** - Real-time battery voltage display and transmission

## Hardware Requirements

### Main Components
- **M5Stack Core2** - ESP32-based development board with LCD, speaker, and battery
- **MS5637 Barometric Sensor** - High-precision pressure and temperature sensor
- **I2C Connection** - External I2C pins (SDA: GPIO21, SCL: GPIO22)

### Specifications
- **Microcontroller**: ESP32 dual-core @ 240MHz
- **Display**: 320x240 TFT LCD
- **Communication**: BLE 4.2 / 5.0
- **Sensor Accuracy**: ±0.5°C temperature, ±20cm altitude
- **Power**: Built-in battery with USB-C charging

## Software Architecture

The system implements a FreeRTOS multi-task architecture with three main concurrent tasks:

```
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│ Sensor Task │────▶│ Variometer   │────▶│  BLE Task   │
│   (5Hz)     │     │   Task (2Hz) │     │   (10Hz)    │
└─────────────┘     └──────────────┘     └─────────────┘
      │                    │                     │
      │                    ▼                     ▼
      │             ┌─────────────┐       ┌──────────┐
      └────────────▶│ LCD Display │       │ XCTrack  │
                    │   (0.5Hz)   │       │   App    │
                    └─────────────┘       └──────────┘
```

### Data Flow
1. **Sensor Task** reads pressure/temperature from MS5637 @ 5Hz via I2C
2. **Variometer Task** applies Kalman filter and calculates vertical speed @ 2Hz
3. **BLE Task** transmits LK8EX1 sentences @ 10Hz to XCTrack
4. **Main Loop** updates LCD display @ 0.5Hz with current flight data

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed architecture documentation.

## Quick Start

### Prerequisites
- [PlatformIO](https://platformio.org/) installed
- M5Stack Core2 device
- MS5637 sensor connected to external I2C pins
- USB-C cable for programming

## Usage

### Connecting to XCTrack

1. Power on the M5Core2 device
2. Wait for the startup screen (displays version and battery info)
3. Open XCTrack app on your mobile device
4. Go to XCTrack Settings → Bluetooth → External Devices
5. Select "M5Core2-Vario" from available devices
6. Configure data source as LK8EX1 protocol
7. Flight data will now be received and logged

### LCD Display Information

The display shows real-time flight data:
- **Pressure**: Current atmospheric pressure (hPa)
- **Temperature**: Ambient temperature (°C)
- **Altitude**: Calculated altitude above sea level (m)
- **V-Speed**: Vertical speed/climb rate (m/s)
- **Battery**: Current battery voltage (V)

### Audio Feedback (Optional)

Audio variometer is disabled by default. To enable:
1. Set `globalSoundEnabled = true` in [`src/main.cpp`](src/main.cpp:25)
2. Rebuild and upload firmware

Audio behavior:
- **Rising tones**: Climbing >0.5 m/s (frequency increases with rate)
- **Sinking tones**: Descending <-0.5 m/s (frequency decreases with rate)
- **Silent**: Neutral flight ±0.5 m/s

## Configuration

All system parameters are defined in [`src/config.h`](src/config.h) for easy customization:

```cpp
// Variometer update rate
const unsigned long VARIOMETER_UPDATE_INTERVAL_MS = 500; // 2Hz

// Altitude change threshold for audio feedback
const float ALTITUDE_CHANGE_THRESHOLD_MPS = 0.5; // m/s

// Kalman filter tuning
const float KALMAN_PROCESS_NOISE = 0.01f;
const float KALMAN_MEASUREMENT_NOISE = 0.04f;

// Moving average window for vertical speed smoothing
const int MOVING_AVERAGE_WINDOW_SIZE = 10;
```

## BLE Protocol

The device transmits data using the LK8EX1 NMEA sentence format:

```
$LK8EX1,<pressure>,<altitude>,<vario>,<temperature>,<battery>*<checksum>
```

Example:
```
$LK8EX1,999999,1234,150,99,4.2*3A
```

- **Pressure**: Reserved (999999)
- **Altitude**: Meters above sea level (integer)
- **Vario**: Vertical speed in cm/s (integer)
- **Temperature**: Reserved (99)
- **Battery**: Voltage in volts (float)
- **Checksum**: XOR checksum in hexadecimal

## Development

### Project Structure

```
m5core2-xctrack/
├── src/                    # Source files
│   ├── main.cpp           # Main program and initialization
│   ├── sensor_task.cpp    # MS5637 sensor reading task
│   ├── variometer_task.cpp # Altitude/vario calculation task
│   ├── ble_uart.cpp       # BLE UART communication
│   ├── kalman_filter.cpp  # Kalman filter implementation
│   ├── sound.cpp          # Audio feedback module
│   └── config.h           # System configuration constants
├── spec/                  # Project specifications
│   ├── requirements.md    # Functional & non-functional requirements
│   ├── architecture.md    # System architecture documentation
│   ├── component_specs.md # Component specifications
│   ├── testing.md         # Test specifications
│   └── use_cases.md       # Use case scenarios
├── test/                  # Unit and integration tests
├── docs/                  # Additional documentation
└── platformio.ini         # PlatformIO configuration
```

### Building and Testing

```bash
# Clean build
pio run --target clean

# Build with verbose output
pio run -v

# Run tests
pio test

# Upload to specific port
pio run --target upload --upload-port /dev/ttyUSB0
```

### Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on:
- Code style and conventions
- Development workflow
- Testing requirements
- Pull request process

## Troubleshooting

### Sensor Not Detected
- Verify I2C connections (SDA: GPIO21, SCL: GPIO22)
- Check sensor power supply
- Ensure external I2C is properly initialized
- Monitor serial output for error messages

### BLE Connection Issues
- Ensure XCTrack has Bluetooth permissions
- Check device is advertising ("M5Core2-Vario")
- Try forgetting and re-pairing the device
- Verify BLE is enabled in XCTrack settings

### Inaccurate Altitude Readings
- Allow sensor to stabilize after power-on
- Check for rapid temperature changes
- Verify sea level pressure setting (default: 1013.25 hPa)
- Review Kalman filter tuning parameters

For more troubleshooting tips, see [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md).

## Performance Characteristics

- **BLE Transmission Latency**: <100ms
- **Sensor Reading Jitter**: <10ms
- **CPU Utilization**: <80% per core
- **Average Current Draw**: ~150mA
- **Altitude Accuracy**: ±20cm
- **Vertical Speed Accuracy**: ±0.1 m/s
- **Continuous Operation**: 4+ hours on battery

## Known Issues

- Audio variometer disabled by default (requires manual enablement)
- No automatic error recovery for BLE disconnection beyond advertising restart
- Limited field testing in actual flight conditions
- GPS integration not yet implemented

See [GitHub Issues](https://github.com/oli4wolf/m5core2-xctrack/issues) for current bug reports and feature requests.

## Future Enhancements

- [ ] GPS integration for position tracking
- [ ] Flight logging to SD card
- [ ] Configuration menu via touchscreen interface
- [ ] Battery level warnings and auto-shutdown
- [ ] WiFi connectivity for firmware updates
- [ ] Wind speed estimation
- [ ] Thermal detection and mapping

## References

- [XCTrack External Devices](https://xctrack.org/External_Devices.html)
- [LK8EX1 Protocol Specification](https://xctrack.org/External_Devices.html)
- [M5Stack Core2 Documentation](https://docs.m5stack.com/en/core/core2)
- [MS5637 Datasheet](https://www.te.com/usa-en/product-CAT-BLPS0037.html)
- [ESP32 BLE Audio Vario Reference](https://github.com/har-in-air/ESP32C3_BLUETOOTH_AUDIO_VARIO)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

**@oli4wolf** on GitHub

This is a non-commercial hobby project created for the paragliding community. Feel free to use, modify, and share!

## Acknowledgments

- M5Stack for excellent ESP32-based hardware platform
- SparkFun for MS5637 Arduino library
- XCTrack team for comprehensive external device support
- Paragliding community for inspiration and feedback

---

**Safety Disclaimer**: This device is for recreational use only. Always carry certified flight instruments when paragliding. Never rely solely on experimental or hobby devices for safety-critical decisions.
