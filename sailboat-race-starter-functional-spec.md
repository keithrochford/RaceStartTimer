# Functional Specification: Automatic Sailboat Race Starting System

**Project:** ESP32-Based Race Start Timer with Web Interface  
**Version:** 1.0  
**Date:** February 13, 2026  
**Platform:** ESP32 (e.g., TTGO ESP32 with integrated OLED)

---

## 1. Overview

### 1.1 Purpose
This document specifies the functional requirements for an automatic sailboat race starting system designed to manage race countdown sequences, provide audible and visual signals, and offer remote monitoring capabilities via an embedded web server.

### 1.2 System Description
The system is an ESP32-based race starter that controls audible horn signals, visual light indicators, and provides operator feedback through an OLED display and web interface. It replaces traditional manual flag and horn operations with an automated, reliable timing system.

### 1.3 Key Features
- Multiple countdown sequence modes (5-minute, 6-minute, 5-minute repeating)
- OLED display for operator guidance
- Web-based remote monitoring and control
- Relay-controlled horn activation
- Relay-controlled flag light indicators
- Operator alert buzzer for flag changes
- Physical control panel with buttons and indicators
- Real-time countdown display accessible via HTTP

---

## 2. System Architecture

### 2.1 Hardware Components

#### 2.1.1 Core Processing
- **Microcontroller:** ESP32 development board (e.g., TTGO ESP32 with integrated OLED)
- **Display:** 0.96" OLED (128x64 pixels, I2C interface)
- **Real-Time Clock:** ESP32 internal timer/RTC for accurate timekeeping

#### 2.1.2 Input Devices
- **Start/Reset Button:** Momentary push button for starting sequences and resetting
- **Mode Selection Button:** Momentary push button for cycling through countdown modes
- **Horn Test Button:** Momentary push button for manual horn activation

#### 2.1.3 Output Devices
- **Relay Board:** Minimum 4-channel relay module for controlling:
  - Channel 1: Main horn/sound signal
  - Channel 2: Class flag light indicator
  - Channel 3: Preparatory flag light indicator
  - Channel 4: Reserved/Auxiliary
- **Operator Buzzer:** 5V active buzzer for operator alerts
- **Physical Indicators:**
  - Class Flag LED: Indicator showing when class flag should be flown
  - Preparatory Flag LED: Indicator showing when P flag should be flown
  - Heartbeat LED: Blink indicator showing system is operational

#### 2.1.4 Network
- **WiFi Module:** ESP32 integrated WiFi for web server and client connectivity

### 2.2 Software Components
- **Timing Engine:** Core countdown logic with millisecond precision
- **Sequence Manager:** State machine for managing race start sequences
- **Web Server:** HTTP server for remote monitoring and control
- **Display Manager:** OLED rendering and operator message system
- **I/O Controller:** Debounced button input and relay output management
- **Audio Alert Manager:** Buzzer pattern generation for operator alerts

---

## 3. Starting Sequence Modes

### 3.1 Mode 1: 6-Minute Countdown
Standard full sequence starting with AP (Answering Pennant) down.

| Time | Event | Horn Signal | Flags | Display Message |
|------|-------|-------------|-------|-----------------|
| T-6:00 | Sequence Start | 1 long | AP Down | "LOWER AP FLAG" |
| T-5:00 | Class Flag Up | 1 long | Class Flag Up | "RAISE CLASS FLAG" |
| T-4:00 | Preparatory Flag Up | 1 long | P Flag Up | "RAISE PREP FLAG" |
| T-1:00 | Preparatory Flag Down | 1 long | P Flag Down | "LOWER PREP FLAG" |
| T-0:00 | Start | 1 long | Class Flag Down | "START - LOWER CLASS FLAG" |
| T+0:00+ | Post-Start Count Up | - | - | "TIME SINCE START: +MM:SS" |

### 3.2 Mode 2: 5-Minute Countdown
Standard sequence without AP down signal.

| Time | Event | Horn Signal | Flags | Display Message |
|------|-------|-------------|-------|-----------------|
| T-5:00 | Sequence Start | 1 long | Class Flag Up | "RAISE CLASS FLAG" |
| T-4:00 | Preparatory Flag Up | 1 long | P Flag Up | "RAISE PREP FLAG" |
| T-1:00 | Preparatory Flag Down | 1 long | P Flag Down | "LOWER PREP FLAG" |
| T-0:00 | Start | 1 long | Class Flag Down | "START - LOWER CLASS FLAG" |
| T+0:00+ | Post-Start Count Up | - | - | "TIME SINCE START: +MM:SS" |

### 3.3 Mode 3: 5-Minute Repeating
Continuous 5-minute sequence that automatically restarts for multiple classes.

| Time | Event | Horn Signal | Flags | Display Message |
|------|-------|-------------|-------|-----------------|
| T-5:00 | Sequence Start | 1 long | Class Flag Up | "RAISE CLASS FLAG" |
| T-4:00 | Preparatory Flag Up | 1 long | P Flag Up | "RAISE PREP FLAG" |
| T-1:00 | Preparatory Flag Down | 1 long | P Flag Down | "LOWER PREP FLAG" |
| T-0:00 | Start / Auto-Reset | 1 long | Class Flag Down | "START - LOWER CLASS FLAG" |
| (Immediate) | Next Sequence Begins | - | - | "PREPARE NEXT CLASS" |

**Note:** Mode 3 continues cycling until manually reset by operator.

---

## 4. Operator Alert System

### 4.1 Buzzer Alert Patterns
The operator buzzer provides countdown alerts before each flag change:

| Time to Flag | Buzzer Pattern | Description |
|--------------|----------------|-------------|
| 50 seconds | 1 long beep (1000ms) | Initial warning |
| 40 seconds | 1 long beep (1000ms) | Continued warning |
| 30 seconds | 1 long beep (1000ms) | Heightened alert |
| 20 seconds | 1 long beep (1000ms) | Final long warning |
| 15-11 seconds | 1 short beep (200ms) | Single beep countdown |
| 10-6 seconds | 2 short beeps (200ms each, 100ms gap) | Double beep countdown |
| 5-1 seconds | 3 short beeps (200ms each, 100ms gap) | Triple beep countdown |
| 0 seconds | Horn activation | Main horn relay activated |

### 4.2 Buzzer Volume
- Audible in typical race committee boat environment
- Not so loud as to be confused with main horn
- Approximately 85-90 dB at 30cm distance

---

## 5. OLED Display Interface

### 5.1 Display Layout
The OLED screen provides real-time information to the operator:

```
┌──────────────────────────┐
│  MODE: 5-MIN COUNTDOWN   │  Line 1: Current mode
├──────────────────────────┤
│                          │
│      TIME: 04:23         │  Line 3-4: Large countdown timer
│                          │
├──────────────────────────┤
│  NEXT: LOWER PREP FLAG   │  Line 6: Next action required
├──────────────────────────┤
│  ● CLASS  ● PREP  ♥ LIVE │  Line 8: Status indicators
└──────────────────────────┘
```

### 5.2 Display States

#### 5.2.1 Idle State
```
    RACE START TIMER
    ================
    
    Mode: [6-MIN / 5-MIN / REPEAT]
    
    Press START to begin
    Mode button to change
```

#### 5.2.2 Active Countdown
```
    5-MINUTE COUNTDOWN
    ------------------
    
        04:23
    
    NEXT: LOWER PREP FLAG
    ● CLASS  ● PREP  ♥
```

#### 5.2.3 Flag Change Prompt (Last 10 seconds)
```
    !!! ACTION REQUIRED !!!
    =======================
    
        00:08
    
    >>> LOWER PREP FLAG <<<
    ● CLASS  ○ PREP  ♥
```

#### 5.2.4 Post-Start Count Up
```
    START +00:45
    ====================
    
    Time since start:
        00:45
    
    Press RESET for next race
    ○ CLASS  ○ PREP  ♥
```

### 5.3 Display Update Rate
- Countdown timer: Update every 100ms
- Flag indicators: Update on state change
- Heartbeat indicator: Blink every 1 second (500ms on, 500ms off)

---

## 6. Physical Operator Panel

### 6.1 Button Functions

#### 6.1.1 Start/Reset Button
- **Idle State:** Press to start the currently selected countdown sequence
- **Active State:** Press and hold for 3 seconds to abort and reset countdown
- **Post-Start State:** Press to reset for next race
- **LED Indicator:** Green LED, solid when ready, blinking during countdown

#### 6.1.2 Mode Selection Button
- **Function:** Cycle through available countdown modes
- **Sequence:** 6-MIN → 5-MIN → REPEAT → 6-MIN (loop)
- **Availability:** Only functional when system is in idle state
- **LED Indicator:** Blue LED, illuminates on button press

#### 6.1.3 Horn Test Button
- **Function:** Immediately activate horn relay for testing
- **Duration:** Active while button is pressed (maximum 5 seconds)
- **Availability:** Available in all states except during actual horn signal
- **Safety:** Cannot interrupt automated horn signals
- **LED Indicator:** Red LED while pressed

### 6.2 Visual Indicators

#### 6.2.1 Class Flag Indicator
- **Type:** Red LED (high brightness, >1000 mcd)
- **Function:** Illuminated when class flag should be raised
- **States:**
  - OFF: Class flag should not be displayed
  - ON (Solid): Class flag should be flying
  - BLINK (Fast, 5 Hz): Class flag about to go up/down (last 10 seconds)

#### 6.2.2 Preparatory Flag Indicator
- **Type:** Blue LED (high brightness, >1000 mcd)
- **Function:** Illuminated when preparatory flag (P flag) should be raised
- **States:**
  - OFF: P flag should not be displayed
  - ON (Solid): P flag should be flying
  - BLINK (Fast, 5 Hz): P flag about to go up/down (last 10 seconds)

#### 6.2.3 Heartbeat Indicator
- **Type:** Green LED
- **Function:** System status indicator
- **Pattern:** Blink at 1 Hz (500ms on, 500ms off)
- **Purpose:** Confirms system is powered and running correctly

---

## 7. Relay Output Control

### 7.1 Relay Specifications
- **Type:** Optically isolated relay module (5V logic, 10A@250VAC/30VDC rating)
- **Channels Required:** 4
- **Activation:** Active HIGH from ESP32 GPIO pins
- **Response Time:** <10ms switching time

### 7.2 Relay Channel Assignments

#### 7.2.1 Channel 1: Main Horn
- **Purpose:** Activate primary sound signal (horn)
- **Activation Duration:** 1 second for standard signals
- **Current Rating:** 10A minimum (for horn relay coil)
- **Safety:** Includes software timeout to prevent horn from staying on indefinitely

#### 7.2.2 Channel 2: Class Flag Light
- **Purpose:** Control visual light indicator representing class flag
- **States:** ON when class flag should be displayed, OFF otherwise
- **Light Type:** High-visibility LED or incandescent (50-100W equivalent)
- **Color:** Red (to represent class flag)

#### 7.2.3 Channel 3: Preparatory Flag Light
- **Purpose:** Control visual light indicator representing P flag
- **States:** ON when P flag should be displayed, OFF otherwise
- **Light Type:** High-visibility LED or incandescent (50-100W equivalent)
- **Color:** Blue (to represent P flag)

#### 7.2.4 Channel 4: Auxiliary
- **Purpose:** Reserved for future use
- **Potential Uses:**
  - AP (Answering Pennant) light
  - Additional horn/bell
  - Recording device trigger
  - Starting gate motor control

### 7.3 Horn Activation Timing
- **Signal Duration:** 1.0 second ±50ms
- **Inter-Signal Gap:** Minimum 2 seconds between horn activations
- **Override Protection:** Horn test button cannot activate during automated signals

---

## 8. Web Server Interface

### 8.1 Network Configuration

#### 8.1.1 WiFi Mode
- **Primary Mode:** Access Point (AP) mode for standalone operation
  - SSID: "RaceStartTimer-[MAC]"
  - Password: Configurable (default: "racestart2026")
  - IP Address: 192.168.4.1
  - DHCP Server: Enabled (192.168.4.2 - 192.168.4.20)

- **Secondary Mode:** Station (STA) mode for existing network
  - Configurable SSID and password via web interface
  - DHCP Client or static IP configuration

#### 8.1.2 mDNS Support
- **Hostname:** racestart.local
- **Purpose:** Easy discovery and access without remembering IP address

### 8.2 Web Interface Pages

#### 8.2.1 Main Dashboard (/)
Real-time monitoring page with:

**Display Elements:**
- Current countdown mode (6-MIN / 5-MIN / REPEAT)
- Large countdown timer (MM:SS format, auto-updating every 500ms)
- Current sequence state (e.g., "Waiting for Prep Flag Down")
- Next flag action with countdown
- Status indicators:
  - Class Flag: ON/OFF visual indicator (red)
  - Prep Flag: ON/OFF visual indicator (blue)
  - System Status: "READY" / "ACTIVE" / "COUNTING UP"
- Last horn signal timestamp

**Interactive Elements:**
- START button: Initiate countdown
- RESET button: Stop and reset countdown
- Mode selector: Change countdown mode (disabled during active sequence)
- Manual horn trigger: "Test Horn" button

**Example HTML Layout:**
```
┌─────────────────────────────────────────┐
│  RACE START TIMER CONTROL               │
├─────────────────────────────────────────┤
│  Mode: [5-Minute ▼]  Status: ACTIVE     │
│                                         │
│        ┌──────────────┐                │
│        │    04:23     │  ← Large Timer │
│        └──────────────┘                │
│                                         │
│  Next Action: LOWER PREP FLAG           │
│  Time to Action: 00:23                  │
│                                         │
│  Flags:  🔴 CLASS    🔵 PREP           │
│                                         │
│  [START] [RESET] [TEST HORN] [MODE]    │
│                                         │
│  Horn Log:                              │
│  • 14:23:15 - Class Flag Up             │
│  • 14:24:15 - Prep Flag Up              │
│  • 14:27:15 - Prep Flag Down            │
└─────────────────────────────────────────┘
```

#### 8.2.2 Configuration Page (/config)
Settings and configuration:

**Network Settings:**
- WiFi Mode: AP / Station
- SSID configuration (Station mode)
- Password configuration
- Static IP settings

**Timing Settings:**
- Horn duration (500ms - 2000ms)
- Buzzer volume level
- Alert timing adjustments

**System Settings:**
- Display brightness
- Heartbeat LED enable/disable
- Time zone offset (for log timestamps)
- Device name/identifier

**Calibration:**
- Button test function
- Relay test function (individual channel control)
- Display test pattern

#### 8.2.3 Log Page (/logs)
Historical event log:

- Start/Stop events with timestamps
- All horn activations
- Button presses
- Mode changes
- System errors/warnings
- Downloadable as CSV

#### 8.2.4 API Endpoints (JSON)

**GET /api/status**
```json
{
  "mode": "5-minute",
  "state": "active",
  "timeRemaining": 263,
  "timeElapsed": 37,
  "nextEvent": "Prep Flag Down",
  "classFlag": true,
  "prepFlag": true,
  "hornActive": false,
  "systemTime": "2026-02-13T14:25:37Z"
}
```

**POST /api/control**
Commands:
```json
{
  "action": "start" | "reset" | "mode" | "horn"
}
```

**GET /api/config**
Retrieve current configuration

**POST /api/config**
Update configuration settings

### 8.3 Web Interface Technology
- **Backend:** ESP32 AsyncWebServer library
- **Frontend:** Responsive HTML5/CSS3/JavaScript
- **Real-time Updates:** AJAX polling (500ms interval) or WebSocket for live countdown
- **Styling:** Mobile-friendly, works on phones, tablets, and computers
- **Browser Compatibility:** Chrome, Firefox, Safari, Edge (latest versions)

---

## 9. Timing and Synchronization

### 9.1 Timing Requirements
- **Resolution:** 100ms display updates, 1ms internal timekeeping
- **Accuracy:** ±50ms over 6-minute sequence
- **Clock Source:** ESP32 internal timer with NTP sync capability

### 9.2 Timing State Machine

```
                    ┌─────────┐
                    │  IDLE   │
                    └────┬────┘
                         │ START pressed
                         ▼
        ┌───────────────────────────────┐
        │                               │
   ┌────▼────┐  Next Event  ┌───────────▼────┐
   │ WAITING │◄─────────────┤ EVENT_TRIGGERED│
   │         │              │   (Horn On)    │
   └────┬────┘              └───────────┬────┘
        │                               │
        │         Timer Loop            │
        └───────────────────────────────┘
                         │
                         │ Sequence Complete
                         ▼
                  ┌──────────┐
                  │ COUNT_UP │
                  └─────┬────┘
                        │ RESET pressed
                        ▼
                   ┌─────────┐
                   │  IDLE   │
                   └─────────┘
```

### 9.3 Event Scheduling
Each sequence mode has a predefined event table:

```cpp
struct SequenceEvent {
    int32_t time;         // Time in seconds (negative = before start, positive = after)
    String action;        // "CLASS_UP", "PREP_UP", "PREP_DOWN", "CLASS_DOWN"
    String displayMsg;    // Message for OLED
    bool hornSignal;      // Trigger horn?
    bool classFlag;       // Class flag state
    bool prepFlag;        // Prep flag state
};
```

---

## 10. Safety and Reliability Features

### 10.1 Watchdog Timer
- **Function:** ESP32 hardware watchdog (TWDT)
- **Timeout:** 5 seconds
- **Purpose:** Reset system if main loop hangs

### 10.2 Horn Safety
- **Maximum Duration:** 5-second timeout on horn activation
- **Cooldown:** Minimum 2-second gap between activations
- **Override Protection:** Automated signals cannot be interrupted by test button

### 10.3 Power Management
- **Low Power Mode:** Display dims after 30 minutes of inactivity (idle state)
- **Power-On State:** System boots to IDLE with last used mode selected
- **Battery Backup:** Optional RTC battery for timekeeping during power loss

### 10.4 Error Handling
- **WiFi Failure:** System continues operation in standalone mode
- **Display Failure:** Audible alerts continue, web interface remains available
- **Button Debounce:** 50ms debounce on all physical buttons
- **Relay Fault Detection:** Optional current sensing for horn relay

### 10.5 Fail-Safe Modes
- **Emergency Stop:** Hold all three buttons simultaneously for 2 seconds to force reset
- **Factory Reset:** Hold Mode button during power-on to clear all settings

---

## 11. Configuration and Setup

### 11.1 Initial Setup Procedure
1. Power on the device
2. Connect to "RaceStartTimer-XXXX" WiFi network (password: default or as configured)
3. Navigate to http://192.168.4.1 or http://racestart.local
4. Configure network settings (if connecting to existing WiFi)
5. Test all functions:
   - Button presses
   - Horn relay activation
   - Flag light relays
   - Buzzer alerts
6. Verify countdown accuracy

### 11.2 Pre-Race Checklist
- [ ] System powers on and heartbeat LED is blinking
- [ ] OLED display shows idle screen
- [ ] All buttons respond correctly
- [ ] Horn test produces sound signal
- [ ] Class flag light operates
- [ ] Prep flag light operates
- [ ] Web interface accessible from mobile device/tablet
- [ ] Correct countdown mode selected
- [ ] Backup manual horn button operational

### 11.3 Operator Training Requirements
- Understanding of sailboat race starting procedures
- Familiarity with countdown modes and their differences
- Ability to interpret OLED display messages
- Knowledge of manual override procedures
- Basic troubleshooting skills

---

## 12. System Specifications Summary

### 12.1 Physical Specifications
- **Enclosure:** Weather-resistant (IP54 minimum)
- **Dimensions:** Approximately 200mm x 150mm x 80mm
- **Weight:** <1kg including batteries (if portable)
- **Operating Temperature:** 0°C to 50°C
- **Storage Temperature:** -20°C to 70°C
- **Humidity:** 20% to 90% non-condensing

### 12.2 Electrical Specifications
- **Input Voltage:** 5V DC via USB-C or 12V DC with regulator
- **Power Consumption:**
  - Idle: <500mA @ 5V
  - Active (display + WiFi): <800mA @ 5V
  - Peak (horn relay active): <1.5A @ 5V
- **Relay Outputs:** 4x 10A @ 250VAC / 30VDC
- **Button Inputs:** 3x momentary push button (NO, pull-down)
- **LED Outputs:** 3x indicator LEDs + 1x heartbeat

### 12.3 Software Specifications
- **Platform:** ESP32 (Arduino framework or ESP-IDF)
- **Memory Requirements:**
  - Program Flash: ~1MB
  - Runtime RAM: ~200KB
  - SPIFFS/LittleFS: 1MB for web files
- **Libraries Required:**
  - WiFi / WebServer (AsyncWebServer)
  - OLED Display (Adafruit SSD1306 or U8g2)
  - NTP Client (optional)
  - ArduinoJson (for API)
- **Update Mechanism:** OTA (Over-The-Air) firmware updates via web interface

---

## 13. Testing and Validation

### 13.1 Functional Tests
1. **Timing Accuracy Test**
   - Run full 6-minute sequence
   - Verify each event occurs within ±100ms of expected time
   - Check horn duration is 1.0s ±50ms

2. **Mode Switching Test**
   - Verify all three modes operate correctly
   - Confirm mode selection only works in idle state
   - Test repeating mode cycles correctly

3. **Buzzer Alert Test**
   - Verify buzzer patterns match specification
   - Confirm alerts occur at correct intervals (50, 40, 30, 20, 15-1 seconds)
   - Test buzzer volume is appropriate

4. **Display Test**
   - Verify all display states render correctly
   - Confirm countdown updates smoothly
   - Check flag indicators update correctly

5. **Web Interface Test**
   - Test from multiple devices simultaneously
   - Verify real-time updates work correctly
   - Confirm all API endpoints function properly
   - Test control commands (start/reset/mode/horn)

6. **Button Test**
   - Verify debouncing works (no double-triggers)
   - Test emergency stop function
   - Confirm horn test button operates correctly

7. **Relay Test**
   - Verify all relay channels activate correctly
   - Test horn safety timeout (5-second maximum)
   - Confirm relay state matches flag indicators

### 13.2 Environmental Tests
- Temperature cycling test (0°C to 50°C)
- Vibration test (simulated boat environment)
- Moisture resistance test
- WiFi range test (typical race committee boat scenario)

### 13.3 Safety Tests
- Watchdog timer functionality
- Horn timeout protection
- Emergency stop procedure
- Power-loss recovery

### 13.4 Acceptance Criteria
- ✅ All timing events accurate within ±100ms
- ✅ System operates continuously for 8 hours without failure
- ✅ Web interface accessible from 50 meters distance
- ✅ All audible alerts clearly distinguishable
- ✅ OLED readable in bright sunlight
- ✅ Horn relay never fails to activate or deactivate

---

## 14. Future Enhancements

### 14.1 Potential Features
- **Bluetooth Audio:** Stream buzzer alerts to wireless speaker
- **GPS Time Sync:** Automatic time synchronization via GPS module
- **Finish Line Timer:** Record finish times with wireless remote trigger
- **Voice Alerts:** Text-to-speech announcements for flag changes
- **Mobile App:** Dedicated smartphone/tablet application
- **Multi-Start Management:** Support for multiple simultaneous starts
- **Wind Data Integration:** Display wind speed/direction from anemometer
- **Data Logging:** SD card storage for historical race data
- **Integration with Scoring Systems:** Export timing data in standard formats

### 14.2 Optional Hardware Modules
- **RTC Battery Backup:** CR2032 holder for time persistence
- **GPS Module:** For precise timekeeping and location logging
- **Wireless Remote:** Handheld trigger for finish line recording
- **External Display:** Large LED digits visible from distance
- **Additional Relays:** For more complex flag/light configurations

---

## 15. Appendices

### Appendix A: Racing Rules Reference
This system supports starting sequences defined in the Racing Rules of Sailing (RRS):
- **Rule 26:** Starting Races
- **Preparatory Signals:** Class flag, P flag, I flag, Z flag, U flag, black flag
- **Starting Penalty Rules:** OCS (On Course Side), I flag, Z flag, U flag, Black flag

### Appendix B: Bill of Materials (BOM)

| Component | Specification | Quantity | Notes |
|-----------|--------------|----------|-------|
| ESP32 Development Board | ESP32 DevKit V1 (30-pin) | 1 | Espressif or compatible |
| OLED Display | 0.96" 128x64 I2C OLED (SSD1306) | 1 | I2C interface, 3.3V or 5V |
| Relay Module | 8-Channel 5V relay board | 1 | 10A rating minimum, optoisolated |
| Push Buttons | Momentary SPST NO, 12mm | 3 | Start/Reset, Mode, Horn Test |
| LEDs | 5mm high-brightness | 3 | Red, Blue, Green (>1000 mcd) |
| Active Buzzer | 5V piezo buzzer | 1 | ~85dB @ 30cm |
| Resistors (220Ω) | 1/4W carbon film | 6 | For LED current limiting |
| Resistors (10kΩ) | 1/4W carbon film | 3 | For button pull-downs |
| Resistors (4.7kΩ) | 1/4W carbon film | 2 | For I2C pull-ups (if needed) |
| Buck Converter | 5V 3A DC-DC step-down | 1 | Input: 9-24V, Output: 5V 3A |
| Power Supply | 12V 3A DC adapter | 1 | Wall adapter or battery pack |
| Power Jack | DC barrel jack 5.5mm x 2.1mm | 1 | Panel mount |
| Enclosure | ABS plastic, IP54 rated | 1 | 200x150x80mm minimum |
| Breadboard or PCB | Prototyping board | 1 | For permanent assembly |
| Hookup Wire | 22 AWG stranded | 10m | Various colors, pre-stripped kit |
| Jumper Wires | Male-to-female dupont | 40 | For prototyping connections |
| Screw Terminals | 2-position, 5mm pitch | 10 | For relay outputs |
| Header Pins | 2.54mm pitch | 2 strips | For ESP32 mounting |
| Mounting Hardware | M3 screws, standoffs, nuts | 1 set | For securing boards in enclosure |
| Heat Shrink Tubing | Assorted sizes | 1 pack | For wire insulation |
| Cable Glands | PG7 or PG9 | 4-6 | For enclosure cable entry |
| Horn (External) | 12V automotive horn with relay | 1 | Not included, customer provided |
| Flag Lights (External) | 12V LED lights, red and blue | 2 | 50-100W equivalent brightness |

### Appendix C: System Schematic Diagram

#### Complete System Schematic - ESP32 DevKit V1 with 8-Channel Relay Board

```
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│                          SAILBOAT RACE STARTING SYSTEM SCHEMATIC                            │
│                         ESP32 DevKit V1 + 8-Channel Relay Board                             │
└─────────────────────────────────────────────────────────────────────────────────────────────┘


POWER SUPPLY
═════════════════════════════════════════════════════════════════════════════════════════════

    ┌──────────────┐
    │   12V DC     │ ← External Power (12V 3A recommended)
    │  Power Input │
    └──────┬───────┘
           │
           ├─────────────────────────────────────────┐
           │                                         │
    ┌──────▼───────┐                         ┌──────▼──────┐
    │   5V 3A      │                         │ 12V Direct  │
    │   Buck       │                         │  (Optional) │
    │  Converter   │                         └──────┬──────┘
    └──────┬───────┘                                │
           │                                        │
           └──────┬─────────────────────────────────┘
                  │
          ┌───────┴──────────────────────────────────┐
          │                                           │
          ▼                                           ▼
    [ESP32 5V]                              [Relay Board VCC]
    [ESP32 GND]◄────────────────────────────[Relay Board GND]
          │                                           │
          └───────────┬───────────────────────────────┘
                      │
                  ┌───▼────┐
                  │  COMMON│
                  │  GROUND│
                  └────────┘


ESP32 DEVKIT V1 PINOUT
═════════════════════════════════════════════════════════════════════════════════════════════

                          ┌─────────────────────────┐
                          │    ESP32 DevKit V1      │
                          │                         │
                    3V3 ──┤ 1                    30 ├── GND
                     EN ──┤ 2                    29 ├── GPIO23 (NC)
                 GPIO36 ──┤ 3 (Input Only)      28 ├── GPIO22 (NC)
                 GPIO39 ──┤ 4 (Input Only)      27 ├── TX0
                 GPIO34 ──┤ 5 (Input Only)      26 ├── RX0
                 GPIO35 ──┤ 6 (Input Only)      25 ├── GPIO21 ──────► LED: Heartbeat
 Button: Start/Reset ◄────┤ 7  GPIO32           24 ├── GPIO19 ──────► LED: Prep Flag
 Button: Mode Select ◄────┤ 8  GPIO33           23 ├── GPIO18 ──────► LED: Class Flag
  Button: Horn Test ◄─────┤ 9  GPIO25           22 ├── GPIO5  (NC)
    Buzzer (+) ►──────────┤10  GPIO26           21 ├── GPIO17 (NC)
   Relay CH4 (Aux) ►──────┤11  GPIO27           20 ├── GPIO16 (NC)
  Relay CH3 (Prep) ►──────┤12  GPIO14           19 ├── GPIO4  ──────► OLED SDA
  Relay CH2 (Class)►──────┤13  GPIO12           18 ├── GPIO2  (NC)
  Relay CH1 (Horn) ►──────┤14  GPIO13           17 ├── GPIO15 ──────► OLED SCL
                    GND ──┤15                   16 ├── GND
                          │                         │
                    VIN ──┤    (Connect to 5V)      │
                          └─────────────────────────┘

Additional Relay Connections (Future Use):
───────────────────────────────────────────
GPIO 23 ──────► Relay CH5 (Reserved - AP Flag Light)
GPIO 22 ──────► Relay CH6 (Reserved - Secondary Horn/Bell)
GPIO 5  ──────► Relay CH7 (Reserved - Recording Device Trigger)
GPIO 17 ──────► Relay CH8 (Reserved - Auxiliary/Gate Control)


OLED DISPLAY CONNECTION (I2C)
═════════════════════════════════════════════════════════════════════════════════════════════

    ESP32                         0.96" OLED Display (128x64)
                                  (I2C Address: 0x3C)
    ┌────────┐                    ┌────────────────┐
    │ GPIO4  ├────────────────────┤ SDA            │
    │ GPIO15 ├────────────────────┤ SCL            │
    │  3.3V  ├────────────────────┤ VCC            │
    │  GND   ├────────────────────┤ GND            │
    └────────┘                    └────────────────┘

    Note: Use 4.7kΩ pull-up resistors on SDA and SCL if not included on OLED module


8-CHANNEL RELAY BOARD CONNECTION
═════════════════════════════════════════════════════════════════════════════════════════════

    ESP32 DevKit V1                     8-Channel Relay Module (Active HIGH)
    ┌──────────────┐                    ┌─────────────────────────────────┐
    │              │                    │ VCC ◄───[5V from Buck Converter]│
    │ GPIO13  ─────┼────────────────────┤ IN1  (Horn)                     │
    │ GPIO12  ─────┼────────────────────┤ IN2  (Class Flag Light)         │
    │ GPIO14  ─────┼────────────────────┤ IN3  (Prep Flag Light)          │
    │ GPIO27  ─────┼────────────────────┤ IN4  (Auxiliary)                │
    │ GPIO23  ─────┼────────────────────┤ IN5  (Reserved - AP Light)      │
    │ GPIO22  ─────┼────────────────────┤ IN6  (Reserved - Bell)          │
    │ GPIO5   ─────┼────────────────────┤ IN7  (Reserved - Trigger)       │
    │ GPIO17  ─────┼────────────────────┤ IN8  (Reserved - Aux)           │
    │ GND     ─────┼────────────────────┤ GND                             │
    └──────────────┘                    └─────────────────────────────────┘

    Relay Outputs (10A @ 250VAC / 30VDC):
    ┌─────────────────────────────────┐
    │  COM  NO  NC  (Channel 1 - Horn)│───► To Horn Relay Coil (+)
    │  COM  NO  NC  (Channel 2)       │───► To Class Flag Light (+)
    │  COM  NO  NC  (Channel 3)       │───► To Prep Flag Light (+)
    │  COM  NO  NC  (Channel 4)       │───► Future Use
    │  COM  NO  NC  (Channel 5)       │───► Future Use
    │  COM  NO  NC  (Channel 6)       │───► Future Use
    │  COM  NO  NC  (Channel 7)       │───► Future Use
    │  COM  NO  NC  (Channel 8)       │───► Future Use
    └─────────────────────────────────┘
    
    Note: COM = Common, NO = Normally Open, NC = Normally Closed
          Connect COM to +12V (or appropriate voltage for load)
          Connect NO to load
          Load negative returns to power supply ground


OPERATOR BUTTONS (with Pull-Down Resistors)
═════════════════════════════════════════════════════════════════════════════════════════════

    Start/Reset Button                 Mode Select Button              Horn Test Button
    ┌──────────────┐                  ┌──────────────┐                ┌──────────────┐
    │     ┌─┐      │                  │     ┌─┐      │                │     ┌─┐      │
    │  ───┤ ├───   │                  │  ───┤ ├───   │                │  ───┤ ├───   │
    │     └─┘      │                  │     └─┘      │                │     └─┘      │
    │   Momentary  │                  │   Momentary  │                │   Momentary  │
    │   NO Switch  │                  │   NO Switch  │                │   NO Switch  │
    └──────┬───────┘                  └──────┬───────┘                └──────┬───────┘
           │                                  │                                │
           ├──────────[3.3V]                  ├──────────[3.3V]                ├──────────[3.3V]
           │                                  │                                │
           ├──────► GPIO32                    ├──────► GPIO33                  ├──────► GPIO25
           │                                  │                                │
           └─[10kΩ]─[GND]                     └─[10kΩ]─[GND]                  └─[10kΩ]─[GND]

    Button Press Detection:
    ─────────────────────────
    • HIGH signal (3.3V) when pressed
    • LOW signal (GND) when released via pull-down resistor
    • Software debounce: 50ms


OPERATOR PANEL LED INDICATORS (Current-Limiting Resistors)
═════════════════════════════════════════════════════════════════════════════════════════════

    Class Flag LED (Red)          Prep Flag LED (Blue)         Heartbeat LED (Green)
    ┌──────────────┐              ┌──────────────┐             ┌──────────────┐
    │   GPIO18 ────┼──[220Ω]──┬───│   GPIO19 ────┼──[220Ω]──┬──│   GPIO21 ────┼──[220Ω]──┬
    │              │          │   │              │          │  │              │          │
    │              │         ┌▼┐  │              │         ┌▼┐ │              │         ┌▼┐
    │              │     (A) └┬┘  │              │     (A) └┬┘ │              │     (A) └┬┘
    │              │         │    │              │         │   │              │         │
    │              │     (K) ┌▼┐  │              │     (K) ┌▼┐ │              │     (K) ┌▼┐
    │              │         └┬┘  │              │         └┬┘ │              │         └┬┘
    │              │          │   │              │          │  │              │          │
    │   GND ───────┼──────────┘   │   GND ───────┼──────────┘  │   GND ───────┼──────────┘
    └──────────────┘              └──────────────┘             └──────────────┘
    
    Red LED (Class)              Blue LED (Prep)              Green LED (Heartbeat)
    • Forward Voltage: ~2.0V     • Forward Voltage: ~3.2V     • Forward Voltage: ~2.1V
    • Forward Current: ~10mA     • Forward Current: ~10mA     • Forward Current: ~10mA
    • 220Ω resistor              • 220Ω resistor              • 220Ω resistor


OPERATOR BUZZER (Active Buzzer)
═════════════════════════════════════════════════════════════════════════════════════════════

    ESP32                           Active Buzzer (5V)
    ┌──────────┐                    ┌────────────┐
    │ GPIO26 ──┼────────────────────┤ (+) Signal │
    │          │                    │            │
    │  5V   ───┼────────────────────┤ VCC        │
    │          │                    │            │
    │  GND  ───┼────────────────────┤ GND (-)    │
    └──────────┘                    └────────────┘

    Alternative (Passive Buzzer with Transistor):
    ┌──────────┐     ┌────────┐      ┌───────────┐
    │ GPIO26 ──┼─────┤ 1kΩ    ├──┬───┤ Base (B)  │
    │          │     └────────┘  │   │  NPN      │
    │  5V   ───┼─────────────────┼───┤ Collector │──[Buzzer +]
    │          │                 │   │  (2N3904) │
    │  GND  ───┼─────────────────┴───┤ Emitter   │──[Buzzer -]
    └──────────┘                     └───────────┘


EXTERNAL LOAD CONNECTIONS (via Relay Board)
═════════════════════════════════════════════════════════════════════════════════════════════

1. HORN CIRCUIT (Relay Channel 1)
   ═══════════════════════════════

        12V Supply                 Relay CH1              External Horn Relay
        ┌────┐                    ┌─────────┐              ┌──────────┐
        │ +  ├────────────────────┤ COM  NO ├──────────────┤ Coil (+) │
        │    │                    └─────────┘              │          │
        │ -  ├─────────────────────────────────────────────┤ Coil (-) │
        └────┘                                             └────┬─────┘
                                                                │
                                                                │
                                                          ┌─────▼─────┐
                                                          │   Horn    │
                                                          │ (Customer │
                                                          │ Provided) │
                                                          └───────────┘

2. CLASS FLAG LIGHT (Relay Channel 2)
   ═══════════════════════════════════

        12V Supply                 Relay CH2              Red Light (50-100W equiv.)
        ┌────┐                    ┌─────────┐              ┌──────────┐
        │ +  ├────────────────────┤ COM  NO ├──────────────┤   (+)    │
        │    │                    └─────────┘              │   RED    │
        │ -  ├─────────────────────────────────────────────┤   (-)    │
        └────┘                                             └──────────┘

3. PREP FLAG LIGHT (Relay Channel 3)
   ══════════════════════════════════

        12V Supply                 Relay CH3              Blue Light (50-100W equiv.)
        ┌────┐                    ┌─────────┐              ┌──────────┐
        │ +  ├────────────────────┤ COM  NO ├──────────────┤   (+)    │
        │    │                    └─────────┘              │   BLUE   │
        │ -  ├─────────────────────────────────────────────┤   (-)    │
        └────┘                                             └──────────┘

4. AUXILIARY OUTPUT (Relay Channel 4)
   ═══════════════════════════════════
   Reserved for future use (AP flag light, secondary signal, etc.)


COMPLETE SYSTEM BLOCK DIAGRAM
═════════════════════════════════════════════════════════════════════════════════════════════

                                    ┌──────────────────────┐
                                    │   12V DC Power       │
                                    │   Supply (3A)        │
                                    └─────┬───────┬────────┘
                                          │       │
                                    ┌─────▼───┐   └──────────┐
                                    │ Buck to │              │
                                    │   5V    │              │
                                    └────┬────┘              │
                         ┌───────────────┼────────────┐      │
                         │               │            │      │
    ┌────────────────────▼──────┐   ┌────▼────┐   ┌──▼──────▼─────────┐
    │   ESP32 DevKit V1         │   │  OLED   │   │  8-Channel Relay   │
    │   ┌──────────────────┐    │   │ Display │   │      Board         │
    │   │  WiFi & Bluetooth│    │   │ 128x64  │   │                    │
    │   │     Built-in     │    │   │         │   │ CH1  CH2  CH3  CH4 │
    │   └──────────────────┘    │   │         │   │ CH5  CH6  CH7  CH8 │
    │                            │   └─────────┘   └─┬────┬────┬────┬──┘
    │   GPIO: 4,15 (I2C) ────────┼────────┘           │    │    │    │
    │   GPIO: 13,12,14,27 ───────┼────────────────────┤    │    │    │
    │   GPIO: 23,22,5,17  ───────┼────────────────────┴────┴────┴────┘
    │   GPIO: 32,33,25 (Inputs)  │                    │    │    │
    │   GPIO: 18,19,21 (LEDs) ───┼───┐                │    │    │
    │   GPIO: 26 (Buzzer) ────────┼─┐ │                │    │    │
    └────────────────────────────┘ │ │                │    │    │
              │                    │ │                │    │    │
              │                    │ │         ┌──────▼────▼────▼────┐
              │                    │ │         │  External Loads:    │
              │                    │ │         │  • Horn Relay       │
              │                    │ │         │  • Class Light      │
              │                    │ │         │  • Prep Light       │
    ┌─────────▼──────────┐         │ │         │  • Aux (Future)     │
    │  Operator Console  │         │ │         └─────────────────────┘
    │  ┌──────────────┐  │         │ │
    │  │ Buttons:     │  │         │ └───────► ┌─────────────────┐
    │  │ • Start      │  │         │           │ LEDs:           │
    │  │ • Mode       │  │         │           │ • Class (Red)   │
    │  │ • Horn Test  │  │         │           │ • Prep (Blue)   │
    │  └──────────────┘  │         │           │ • Heartbeat (G) │
    └────────────────────┘         │           └─────────────────┘
                                   │
                              ┌────▼─────┐
                              │  Buzzer  │
                              │  (5V)    │
                              └──────────┘

    Network Access:
    ───────────────
    WiFi AP: RaceStartTimer-XXXX
    Web Interface: http://192.168.4.1 or http://racestart.local
    Mobile devices connect wirelessly for monitoring and control


GPIO PIN ASSIGNMENT SUMMARY
═════════════════════════════════════════════════════════════════════════════════════════════

| GPIO Pin | Function                  | Type   | Notes                        |
|----------|---------------------------|--------|------------------------------|
| GPIO4    | OLED SDA                  | I2C    | With 4.7kΩ pull-up          |
| GPIO15   | OLED SCL                  | I2C    | With 4.7kΩ pull-up          |
| GPIO13   | Relay CH1 (Horn)          | Output | Active HIGH                  |
| GPIO12   | Relay CH2 (Class Light)   | Output | Active HIGH                  |
| GPIO14   | Relay CH3 (Prep Light)    | Output | Active HIGH                  |
| GPIO27   | Relay CH4 (Auxiliary)     | Output | Active HIGH                  |
| GPIO23   | Relay CH5 (Reserved)      | Output | Future: AP Light             |
| GPIO22   | Relay CH6 (Reserved)      | Output | Future: Bell                 |
| GPIO5    | Relay CH7 (Reserved)      | Output | Future: Trigger              |
| GPIO17   | Relay CH8 (Reserved)      | Output | Future: Aux                  |
| GPIO32   | Button: Start/Reset       | Input  | Pull-down, active HIGH       |
| GPIO33   | Button: Mode Select       | Input  | Pull-down, active HIGH       |
| GPIO25   | Button: Horn Test         | Input  | Pull-down, active HIGH       |
| GPIO18   | LED: Class Flag Indicator | Output | With 220Ω resistor           |
| GPIO19   | LED: Prep Flag Indicator  | Output | With 220Ω resistor           |
| GPIO21   | LED: Heartbeat            | Output | With 220Ω resistor           |
| GPIO26   | Buzzer                    | Output | 5V active buzzer             |


PARTS LIST FOR SCHEMATIC
═════════════════════════════════════════════════════════════════════════════════════════════

| Qty | Part                          | Specification                    |
|-----|-------------------------------|----------------------------------|
|  1  | ESP32 DevKit V1               | 30-pin development board         |
|  1  | 8-Channel Relay Module        | 5V logic, 10A @ 250VAC           |
|  1  | OLED Display                  | 0.96" 128x64 I2C (SSD1306)       |
|  1  | 5V 3A Buck Converter          | Input: 12V DC, Output: 5V 3A     |
|  1  | 12V 3A Power Supply           | Wall adapter or battery          |
|  3  | Momentary Push Buttons        | SPST NO, 12mm, panel mount       |
|  1  | Active Buzzer                 | 5V, piezo, 85dB                  |
|  3  | 5mm LEDs                      | Red, Blue, Green (high bright)   |
|  6  | 220Ω Resistors (1/4W)         | LED current limiting             |
|  3  | 10kΩ Resistors (1/4W)         | Button pull-down                 |
|  2  | 4.7kΩ Resistors (1/4W)        | I2C pull-up (if needed)          |
|  1  | Breadboard or PCB             | For assembly                     |
|  -  | Hookup wire 22AWG             | Various colors                   |
|  -  | Screw terminals               | For relay outputs                |
|  1  | Project Enclosure             | 200x150x80mm, IP54 rated         |


NOTES AND SAFETY WARNINGS
═════════════════════════════════════════════════════════════════════════════════════════════

⚠️  IMPORTANT SAFETY CONSIDERATIONS:

1. **Relay Ratings:** Ensure relay board is rated for the load (horn relay coil, lights)
   Typical horn relay coil: 12V @ 0.5A = 6W (well within 10A relay rating)

2. **Power Supply:** Use adequate power supply (12V 3A minimum)
   - ESP32 + OLED + Buzzer + Relays: ~1A @ 5V
   - External loads (horn, lights): ~2A @ 12V peak

3. **GPIO Protection:** ESP32 GPIOs are 3.3V logic. Relay board should have
   optoisolators and can typically accept 3.3V or 5V logic.

4. **Ground Connection:** Ensure common ground between ESP32, relay board, and power supply.

5. **Horn Relay:** This schematic assumes the horn has its own relay/driver circuit.
   The relay board switches the horn relay coil, not the horn directly.

6. **ESD Protection:** Use proper ESD precautions when assembling.

7. **Enclosure:** Use weather-resistant enclosure (IP54+) for marine environment.

8. **Testing:** Test each subsystem individually before connecting external loads:
   - Test buttons and LEDs first
   - Test relay activation with no load
   - Connect loads only after verifying correct operation

9. **WiFi Antenna:** Ensure ESP32 antenna has clearance from metal enclosure for good signal.

10. **Mounting:** Secure all components to prevent movement in boat environment.

```

### Appendix D: Sample Configuration File
```json
{
  "device": {
    "name": "RaceStartTimer-001",
    "version": "1.0.0"
  },
  "wifi": {
    "mode": "ap",
    "ap_ssid": "RaceStartTimer",
    "ap_password": "racestart2026",
    "sta_ssid": "",
    "sta_password": "",
    "static_ip": false,
    "ip_address": "192.168.4.1"
  },
  "timing": {
    "horn_duration_ms": 1000,
    "horn_timeout_ms": 5000,
    "buzzer_volume": 80,
    "alert_times": [50, 40, 30, 20, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
  },
  "display": {
    "brightness": 255,
    "screensaver_timeout": 1800,
    "update_interval_ms": 100
  },
  "modes": {
    "default_mode": "5-minute",
    "available_modes": ["6-minute", "5-minute", "5-minute-repeat"]
  }
}
```

### Appendix E: Glossary

- **AP (Answering Pennant):** Flag indicating postponement or delay
- **Class Flag:** Flag identifying the starting class
- **ESP32:** Espressif Systems microcontroller with WiFi and Bluetooth
- **OCS:** On Course Side (early start)
- **OLED:** Organic Light Emitting Diode display
- **P Flag:** Preparatory flag (blue with white center)
- **Preparatory Signal:** Signal given 4 minutes before start in 5-minute sequence
- **RRS:** Racing Rules of Sailing
- **RTC:** Real-Time Clock
- **Sound Signal:** Audible signal (horn) indicating flag change
- **Starting Sequence:** Timed series of flag and sound signals preceding race start

---

## Document Control

**Revision History:**

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-02-13 | System Architect | Initial functional specification |

**Approval:**

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Project Manager | | | |
| Lead Developer | | | |
| Race Committee Representative | | | |

**Distribution:**
- Development Team
- Race Committee
- Testing Team
- Hardware Team

---

*End of Functional Specification Document*
