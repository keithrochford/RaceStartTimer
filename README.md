# Sailboat Race Starting System - Arduino Sketch

## Overview
Complete implementation of the Sailboat Race Starting System for ESP32 DevKit V1 based on the functional specification.

## Features
- ✅ Three countdown modes (6-minute, 5-minute, 5-minute repeating)
- ✅ OLED display (128x64 I2C) with real-time countdown
- ✅ 8-channel relay control (horn, class light, prep light, auxiliary)
- ✅ Operator alert buzzer with configurable patterns
- ✅ Physical control buttons (Start/Reset, Mode, Horn Test)
- ✅ LED indicators (Class Flag, Prep Flag, Heartbeat)
- ✅ WiFi Access Point mode
- ✅ Web server with dashboard and API
- ✅ Real-time status monitoring via web interface
- ✅ mDNS support (http://racestart.local)

## Hardware Requirements

### Main Components
- **ESP32 DevKit V1** (30-pin)
- **1.3" OLED Display** (128x64, I2C, SSD1306)
- **8-Channel Relay Module** (5V logic, 10A rating)
- **Active Buzzer** (5V piezo)
- **3x Momentary Push Buttons**
- **3x LEDs** (Red, Blue, Green)
- **Resistors** (220Ω for LEDs, 10kΩ for buttons, 4.7kΩ for I2C)
- **Buck Converter** (12V to 5V, 3A)
- **Power Supply** (12V 3A)

### Pin Connections

#### OLED Display (I2C) - 1.3" 128x64 SSD1306
```
GPIO 4  → SDA
GPIO 15 → SCL
3.3V    → VCC
GND     → GND
```

#### Relays
```
GPIO 13 → Channel 1 (Horn)
GPIO 12 → Channel 2 (Class Light)
GPIO 14 → Channel 3 (Prep Light)
GPIO 27 → Channel 4 (Auxiliary)
GPIO 23 → Channel 5 (Reserved)
GPIO 22 → Channel 6 (Reserved)
GPIO 5  → Channel 7 (Reserved)
GPIO 17 → Channel 8 (Reserved)
```

#### Buttons (with 10kΩ pull-down resistors)
```
GPIO 32 → Start/Reset Button
GPIO 33 → Mode Select Button
GPIO 25 → Horn Test Button
```

#### LEDs (with 220Ω current-limiting resistors)
```
GPIO 18 → Class Flag LED (Red)
GPIO 19 → Prep Flag LED (Blue)
GPIO 21 → Heartbeat LED (Green)
```

#### Buzzer
```
GPIO 26 → Buzzer (+)
5V      → VCC
GND     → GND (-)
```

## Software Dependencies

### Arduino Libraries
- **Adafruit SSD1306** (v2.5.13 or higher) - OLED display driver
- **Adafruit GFX Library** (v1.12.0 or higher) - Graphics primitives
- **ArduinoJson** (v7.3.1 or higher) - JSON parsing for API
- **WiFi** (included with ESP32 core)
- **WebServer** (included with ESP32 core)
- **ESPmDNS** (included with ESP32 core)

### ESP32 Board Support
- **ESP32 Core** (v3.3.6 or higher)

## Building and Uploading

### Using Arduino CLI

1. **Install Arduino CLI** (if not already installed):
   ```bash
   # macOS
   brew install arduino-cli
   
   # Linux
   curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
   
   # Windows
   # Download from https://arduino.github.io/arduino-cli/
   ```

2. **Update core index**:
   ```bash
   arduino-cli core update-index
   ```

3. **Install ESP32 core**:
   ```bash
   arduino-cli core install esp32:esp32
   ```

4. **Install required libraries**:
   ```bash
   arduino-cli lib install "Adafruit SSD1306" "Adafruit GFX Library" "ArduinoJson"
   ```

5. **Compile the sketch**:
   ```bash
   cd /path/to/RaceStartTimer
   arduino-cli compile --fqbn esp32:esp32:esp32 RaceStartTimer.ino
   ```

6. **Upload to ESP32** (replace PORT with your device port):
   ```bash
   # Find your port
   arduino-cli board list
   
   # Upload
   arduino-cli upload -p /dev/cu.usbserial-* --fqbn esp32:esp32:esp32 RaceStartTimer.ino
   ```

### Using Arduino IDE

1. **Install Arduino IDE** (v2.0 or higher)
2. **Add ESP32 Board Manager URL**:
   - Open: File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     https://espressif.github.io/arduino-esp32/package_esp32_index.json
     ```
3. **Install ESP32 boards**:
   - Open: Tools → Board → Boards Manager
   - Search for "esp32"
   - Install "esp32 by Espressif Systems"
4. **Install libraries**:
   - Open: Tools → Manage Libraries
   - Install: Adafruit SSD1306, Adafruit GFX Library, ArduinoJson
5. **Configure board**:
   - Board: "ESP32 Dev Module"
   - Upload Speed: 115200
   - CPU Frequency: 240MHz
   - Flash Frequency: 80MHz
   - Flash Mode: QIO
   - Flash Size: 4MB
   - Partition Scheme: Default 4MB with spiffs
6. **Open sketch**: File → Open → RaceStartTimer.ino
7. **Upload**: Sketch → Upload (or Ctrl+U)

## Configuration

### WiFi Settings
Default Access Point configuration (modify in code if needed):
```cpp
const char* AP_SSID = "RaceStartTimer";
const char* AP_PASSWORD = "racestart2026";
```

### Timing Constants
All timing values can be adjusted in the code:
```cpp
#define HORN_DURATION 1000      // Horn signal duration (ms)
#define BUZZER_LONG_BEEP 1000   // Long beep duration (ms)
#define BUZZER_SHORT_BEEP 200   // Short beep duration (ms)
// Alert times array can be customized
```

### Pin Assignments
All GPIO pins are defined at the top of the sketch and can be modified if needed.

## Usage

### Initial Setup
1. Power on the device
2. Connect to WiFi network: **RaceStartTimer** (password: racestart2026)
3. Open browser to: **http://192.168.4.1** or **http://racestart.local**

### Physical Controls

#### Start/Reset Button
- **Idle State**: Press to start the selected countdown sequence
- **Active State**: Press to reset/abort countdown
- **Post-Start**: Press to reset for next race

#### Mode Selection Button
- Press to cycle through modes: 6-MIN → 5-MIN → REPEAT
- Only works when system is idle

#### Horn Test Button
- Press and hold to activate horn manually
- Cannot interrupt automated horn signals

### LED Indicators

#### Class Flag LED (Red)
- **ON**: Class flag should be raised
- **OFF**: Class flag should be down

#### Prep Flag LED (Blue)
- **ON**: Preparatory flag should be raised
- **OFF**: Prep flag should be down

#### Heartbeat LED (Green)
- **Blink**: System is running normally (1 Hz)

### Web Interface

#### Main Dashboard
Access at: http://192.168.4.1 or http://racestart.local

Features:
- Large countdown timer display
- Real-time status updates
- Flag indicators (red/blue)
- Control buttons (Start, Reset, Mode, Test Horn)
- Next action display
- System status

#### API Endpoints

**GET /api/status**
Returns current system status as JSON:
```json
{
  "mode": "5-MINUTE",
  "state": "ACTIVE",
  "time": 263,
  "classFlag": true,
  "prepFlag": true,
  "hornActive": false,
  "nextEvent": "LOWER PREP FLAG"
}
```

**POST /api/control**
Send control commands:
```json
{"action": "start"}   // Start countdown
{"action": "reset"}   // Reset system
{"action": "mode"}    // Change mode
{"action": "horn"}    // Test horn
```

## Countdown Sequences

### 6-Minute Mode
```
T-6:00  AP Down          → Horn + "LOWER AP FLAG"
T-5:00  Class Flag Up    → Horn + "RAISE CLASS FLAG"
T-4:00  Prep Flag Up     → Horn + "RAISE PREP FLAG"
T-1:00  Prep Flag Down   → Horn + "LOWER PREP FLAG"
T-0:00  Start            → Horn + "START - LOWER CLASS"
T+      Count up
```

### 5-Minute Mode
```
T-5:00  Class Flag Up    → Horn + "RAISE CLASS FLAG"
T-4:00  Prep Flag Up     → Horn + "RAISE PREP FLAG"
T-1:00  Prep Flag Down   → Horn + "LOWER PREP FLAG"
T-0:00  Start            → Horn + "START - LOWER CLASS"
T+      Count up
```

### 5-Minute Repeating Mode
Same as 5-minute but automatically restarts after start signal for multiple classes.

## Buzzer Alert Patterns

Operator alerts before each flag change:
- **50s, 40s, 30s, 20s**: 1 long beep (1000ms)
- **15-11 seconds**: 1 short beep (200ms)
- **10-6 seconds**: 2 short beeps
- **5-1 seconds**: 3 short beeps

## Troubleshooting

### Display Not Working
- Check I2C connections (SDA on GPIO 4, SCL on GPIO 15)
- Verify I2C address is 0x3C (default for SSD1306 128x64)
- Check 3.3V power supply to OLED
- Ensure display is 1.3" OLED with 128x64 resolution

### WiFi Not Connecting
- Reset device and wait 30 seconds
- Check for "RaceStartTimer" network in WiFi list
- Verify password: racestart2026

### Relays Not Activating
- Check relay module power (5V and GND)
- Verify GPIO connections
- Test individual relays via web interface
- Check relay module logic level (should support 3.3V input)

### Horn Not Sounding
- Verify relay CH1 activates (LED on relay board)
- Check horn relay external wiring
- Test with Horn Test button
- Check horn power supply (typically 12V)

### Buttons Not Responding
- Check button wiring and pull-down resistors (10kΩ)
- Verify GPIO connections
- Check serial monitor for button press debug messages

### Serial Monitor Output
Connect to serial port at 115200 baud to see debug messages:
```
===================================
Sailboat Race Starting System
Version 1.0.0
===================================

Initializing hardware...
Hardware initialized successfully
Setting up WiFi Access Point...
AP IP address: 192.168.4.1
mDNS responder started: http://racestart.local
WiFi setup complete
Setting up web server...
Web server started on port 80
System ready!
```

## Compilation Statistics

```
Sketch uses 1023027 bytes (78%) of program storage space. Maximum is 1310720 bytes.
Global variables use 49648 bytes (15%) of dynamic memory, leaving 278032 bytes for local variables. Maximum is 327680 bytes.
```

## Safety Features

- **Watchdog Timer**: System automatically resets if main loop hangs
- **Horn Safety Timeout**: Horn automatically deactivates after 5 seconds
- **Button Debouncing**: 50ms debounce prevents false triggers
- **Input Validation**: All web API commands are validated

## License

This code is provided as-is for the Sailboat Race Starting System project.

## Version History

- **v1.0.0** (2026-02-13): Initial release
  - All three countdown modes implemented
  - Web server and API functional
  - OLED display working
  - Relay control operational
  - Buzzer alerts implemented

## Support and Documentation

For full system documentation, refer to the Functional Specification document.

## Contributing

To modify the code:
1. Test thoroughly with hardware
2. Verify timing accuracy
3. Check relay safety features
4. Test web interface on multiple devices
5. Document any changes

## Credits

Developed based on the YachtRaceTimer project and sensorsiot ESP32 examples.
