# Use Cases

## Primary Use Cases

### UC-1: Flight Data Acquisition
**Actor**: Paraglider pilot
**Preconditions**: Device powered on, sensors initialized
**Main Flow**:
1. Pilot powers on M5Core2 device
2. System initializes sensors and BLE
3. Device continuously measures pressure and temperature
4. System calculates altitude and vertical speed
5. LCD displays real-time flight data
6. BLE transmits data to XCTrack app
**Postconditions**: Flight data available in XCTrack
**Alternative Flows**:
- A1: Sensor failure - System logs error and continues with last known values

### UC-2: Variometer Audio Feedback
**Actor**: Paraglider pilot
**Preconditions**: Audio enabled, device in flight
**Main Flow**:
1. Pilot enables audio variometer (if disabled)
2. System detects vertical speed changes
3. For climbing (>0.5 m/s): Plays rising tone (frequency increases with rate)
4. For sinking (<-0.5 m/s): Plays sinking tone (frequency decreases with rate)
5. For neutral: No audio output
**Postconditions**: Pilot receives audible climb/sink feedback
**Alternative Flows**:
- A1: Audio disabled - No tones played

### UC-3: XCTrack Integration
**Actor**: XCTrack application
**Preconditions**: BLE connection established
**Main Flow**:
1. XCTrack app connects to "M5Core2-Vario" BLE device
2. System transmits LK8EX1 sentences at 10Hz
3. XCTrack receives altitude, climb rate, battery data
4. XCTrack processes data for flight tracking
**Postconditions**: Flight path recorded in XCTrack
**Alternative Flows**:
- A1: Connection lost - BLE advertising restarts automatically

### UC-4: Battery Monitoring
**Actor**: Paraglider pilot
**Preconditions**: Device operational
**Main Flow**:
1. System monitors battery voltage via M5Unified
2. LCD displays current battery level
3. BLE transmits battery voltage in LK8EX1 sentences
4. XCTrack displays battery status
**Postconditions**: Pilot aware of battery status
**Alternative Flows**:
- A1: Low battery - Continue operation (no automatic shutdown)

## Secondary Use Cases

### UC-5: System Startup
**Actor**: Paraglider pilot
**Preconditions**: Device connected to power
**Main Flow**:
1. Pilot presses power button
2. System initializes M5Stack hardware
3. Startup screen displays version and battery info
4. Sensor initialization and validation
5. BLE service setup and advertising
6. Tasks start execution
**Postconditions**: System ready for flight data acquisition
**Alternative Flows**:
- A1: Sensor init failure - System halts with error message

### UC-6: GPS Data Fallback (Future)
**Actor**: System
**Preconditions**: GPS configured but no fix available
**Main Flow**:
1. GPS task attempts to get valid fix
2. Timeout after 10 seconds without fix
3. System switches to test data mode
4. Uses predefined coordinates (Bern, Switzerland)
5. Continues normal operation with test data
**Postconditions**: System operational with fallback data
**Alternative Flows**:
- A1: GPS fix acquired - Uses real GPS data

### UC-7: Real-time Data Display
**Actor**: Paraglider pilot
**Preconditions**: System operational
**Main Flow**:
1. LCD updates every 2 seconds
2. Displays pressure (hPa), temperature (°C)
3. Shows calculated altitude (m), vertical speed (m/s)
4. Indicates battery voltage (V)
5. Pilot monitors data during flight
**Postconditions**: Pilot has visual feedback of flight parameters
**Alternative Flows**:
- A1: Display failure - Continue data acquisition and BLE transmission

## Error Scenarios

### UC-8: Mutex Timeout
**Actor**: System
**Preconditions**: High system load
**Main Flow**:
1. Task attempts to acquire mutex
2. Mutex unavailable within timeout (10ms)
3. System logs error message
4. Task continues without updating shared data
**Postconditions**: System continues operation with stale data
**Alternative Flows**:
- A1: Mutex acquired - Normal data update

### UC-9: BLE Transmission Failure
**Actor**: System
**Preconditions**: BLE connection issues
**Main Flow**:
1. BLE notify() fails
2. System continues attempting transmission
3. No data loss (continues calculating)
4. Logs transmission errors
**Postconditions**: Data calculation continues, transmission may resume
**Alternative Flows**:
- A1: Connection restored - Transmission resumes normally

## Performance Requirements
- **Data Latency**: BLE transmission within 100ms of calculation
- **Update Frequency**: Maintain specified rates under normal conditions
- **Resource Usage**: CPU cores utilized efficiently (PRO/APP separation)
- **Memory Usage**: No memory leaks during extended operation