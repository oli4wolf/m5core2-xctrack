# Component Specifications

## Sensor Component (MS5637 Barometric Sensor)
- **Purpose**: Measures atmospheric pressure and temperature for altitude calculation
- **Interface**: I2C (400kHz), pins SDA: GPIO21, SCL: GPIO22 (M5Stack Core2 external I2C)
- **Library**: SparkFun MS5637 Barometric Pressure Library v1.0.2
- **Update Rate**: 5Hz (200ms intervals)
- **Data Types**: Pressure (hPa), Temperature (°C)
- **Accuracy**: ±0.5°C temperature, ±20cm altitude
- **Initialization**: Releases internal I2C, initializes external I2C, checks sensor response
- **Constraints**: Must initialize successfully on startup, data protected by mutex (xSensorMutex)

## Variometer Component
- **Purpose**: Calculates vertical speed from pressure changes using barometric altitude
- **Algorithm**: Kalman filter for altitude smoothing, moving average filter (10 samples) on vertical speed, pressure to altitude conversion using standard atmosphere model
- **Update Rate**: 500ms (configurable via VARIOMETER_UPDATE_INTERVAL_MS)
- **Audio Output**: Tone generation based on vertical speed (enabled by default via globalSoundEnabled=true)
- **Tone Logic**: Rising: 1000Hz base + 50Hz/m/s, Sinking: 500Hz base - 50Hz/m/s, min 100Hz, max 2000Hz, duration 50ms
- **Constraints**: Altitude change threshold of 0.5 m/s, frequency limits (100-2000Hz), data protected by mutex (xVariometerMutex)

## BLE Component (LK8EX1 Protocol)
- **Purpose**: Transmits flight data to XCTrack via BLE UART
- **Protocol**: NMEA-style LK8EX1 sentences with XOR checksum: $LK8EX1,999999,<alt_m>,<climb_cm/s>,99,<bat_V>*<checksum>
- **Update Rate**: 10Hz (100ms intervals)
- **Data Format**: Altitude (int32_t m), Climb rate (int32_t cm/s), Battery voltage (float V)
- **BLE Config**: Device name "M5Core2-Vario", MTU 46 bytes, power level ESP_PWR_LVL_N0, service UUID 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
- **Constraints**: BLE2902 descriptor for notifications, advertising restarts on disconnect

## Display Component (M5Stack LCD)
- **Purpose**: Real-time visualization of sensor data
- **Interface**: Internal LCD (320x240 resolution)
- **Update Rate**: 2Hz (2000ms intervals in loop)
- **Data Displayed**: Pressure (hPa), Temperature (°C), Altitude (m), Vertical Speed (m/s), Battery (V)
- **Library**: M5Unified, M5GFX
- **Constraints**: TFT_BLACK background, TFT_WHITE text, text size 2

## Audio Component (M5Stack Speaker)
- **Purpose**: Provides audible variometer feedback (currently disabled)
- **Interface**: Internal speaker
- **Volume**: Default 64/255 (configurable)
- **Constraints**: Initialized in variometer task, controlled by globalSoundEnabled flag