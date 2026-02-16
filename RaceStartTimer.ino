/*
 * Sailboat Race Starting System
 * ESP32 DevKit V1 Implementation
 * 
 * Features:
 * - Multiple countdown modes (6-min, 5-min, 5-min repeating)
 * - OLED display for operator guidance
 * - Web server for remote monitoring
 * - Relay control for horn and flag lights
 * - Buzzer alerts for operators
 * 
 * Author: Race Timer System
 * Date: February 13, 2026
 * Version: 1.0.0
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================

// OLED Display (I2C)
#define OLED_SDA 4
#define OLED_SCL 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Relay Outputs (Active HIGH)
#define RELAY_HORN 13        // CH1 - Main horn
#define RELAY_CLASS_LIGHT 12 // CH2 - Class flag light
#define RELAY_PREP_LIGHT 14  // CH3 - Prep flag light
#define RELAY_AUX 27         // CH4 - Auxiliary

// Reserved for future use
#define RELAY_CH5 23
#define RELAY_CH6 22
#define RELAY_CH7 5
#define RELAY_CH8 17

// Button Inputs (Active HIGH with pull-down)
#define BTN_START 32    // Start/Reset button
#define BTN_MODE 33     // Mode selection button
#define BTN_HORN_TEST 25 // Horn test button

// LED Indicators
#define LED_CLASS 18     // Class flag indicator (Red)
#define LED_PREP 19      // Prep flag indicator (Blue)
#define LED_HEARTBEAT 21 // System heartbeat (Green)

// Buzzer Output
#define BUZZER_PIN 26

// ============================================================================
// TIMING CONSTANTS
// ============================================================================

#define DEBOUNCE_DELAY 50
#define HORN_DURATION 1000      // 1 second
#define HORN_TIMEOUT 5000       // Safety timeout
#define BUZZER_LONG_BEEP 1000   // Long beep duration
#define BUZZER_SHORT_BEEP 200   // Short beep duration
#define BUZZER_GAP 100          // Gap between beeps
#define HEARTBEAT_INTERVAL 500  // Heartbeat blink rate
#define DISPLAY_UPDATE_RATE 100 // Display update in ms
#define WEB_UPDATE_RATE 500     // Web polling rate

// Alert times (seconds before flag change)
const int ALERT_TIMES[] = {50, 40, 30, 20, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
const int NUM_ALERTS = sizeof(ALERT_TIMES) / sizeof(ALERT_TIMES[0]);

// ============================================================================
// SYSTEM ENUMERATIONS
// ============================================================================

enum CountdownMode {
  MODE_6_MINUTE = 0,
  MODE_5_MINUTE = 1,
  MODE_5_MINUTE_REPEAT = 2
};

enum SystemState {
  STATE_IDLE,
  STATE_ACTIVE,
  STATE_COUNT_UP
};

enum EventType {
  EVENT_AP_DOWN,
  EVENT_CLASS_UP,
  EVENT_PREP_UP,
  EVENT_PREP_DOWN,
  EVENT_CLASS_DOWN_START
};

// ============================================================================
// SEQUENCE EVENT STRUCTURE
// ============================================================================

struct SequenceEvent {
  int32_t timeSeconds;  // Negative = before start, 0 = start
  EventType type;
  const char* displayMsg;
  bool hornSignal;
  bool classFlag;
  bool prepFlag;
};

// ============================================================================
// COUNTDOWN SEQUENCES
// ============================================================================

// 6-minute sequence
const SequenceEvent SEQ_6_MINUTE[] = {
  {-360, EVENT_AP_DOWN, "LOWER AP FLAG", true, false, false},
  {-300, EVENT_CLASS_UP, "RAISE CLASS FLAG", true, true, false},
  {-240, EVENT_PREP_UP, "RAISE PREP FLAG", true, true, true},
  {-60, EVENT_PREP_DOWN, "LOWER PREP FLAG", true, true, false},
  {0, EVENT_CLASS_DOWN_START, "START - LOWER CLASS", true, false, false}
};
const int SEQ_6_MINUTE_LEN = sizeof(SEQ_6_MINUTE) / sizeof(SequenceEvent);

// 5-minute sequence
const SequenceEvent SEQ_5_MINUTE[] = {
  {-300, EVENT_CLASS_UP, "RAISE CLASS FLAG", true, true, false},
  {-240, EVENT_PREP_UP, "RAISE PREP FLAG", true, true, true},
  {-60, EVENT_PREP_DOWN, "LOWER PREP FLAG", true, true, false},
  {0, EVENT_CLASS_DOWN_START, "START - LOWER CLASS", true, false, false}
};
const int SEQ_5_MINUTE_LEN = sizeof(SEQ_5_MINUTE) / sizeof(SequenceEvent);

// 5-minute repeating (same as 5-minute)
const SequenceEvent SEQ_5_MINUTE_REPEAT[] = {
  {-300, EVENT_CLASS_UP, "RAISE CLASS FLAG", true, true, false},
  {-240, EVENT_PREP_UP, "RAISE PREP FLAG", true, true, true},
  {-60, EVENT_PREP_DOWN, "LOWER PREP FLAG", true, true, false},
  {0, EVENT_CLASS_DOWN_START, "START - LOWER CLASS", true, false, false}
};
const int SEQ_5_MINUTE_REPEAT_LEN = sizeof(SEQ_5_MINUTE_REPEAT) / sizeof(SequenceEvent);

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);

// ============================================================================
// GLOBAL STATE VARIABLES
// ============================================================================

CountdownMode currentMode = MODE_5_MINUTE;
SystemState systemState = STATE_IDLE;

unsigned long startTime = 0;
int currentEventIndex = 0;
const SequenceEvent* currentSequence = nullptr;
int currentSequenceLength = 0;

bool classFlag = false;
bool prepFlag = false;
bool hornActive = false;
bool buzzerActive = false;

unsigned long lastDisplayUpdate = 0;
unsigned long lastHeartbeat = 0;
bool heartbeatState = false;

unsigned long hornStartTime = 0;
unsigned long buzzerStartTime = 0;
int buzzerBeepCount = 0;
int buzzerBeepTarget = 0;

// Button debouncing
unsigned long lastBtnStartPress = 0;
unsigned long lastBtnModePress = 0;
unsigned long lastBtnHornPress = 0;
bool btnStartPressed = false;
bool btnModePressed = false;
bool btnHornPressed = false;

// Alert tracking
bool alertSounded[NUM_ALERTS];

// WiFi Configuration
const char* AP_SSID = "RaceStartTimer";
const char* AP_PASSWORD = "racestart2026";

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

void setupHardware();
void setupWiFi();
void setupWebServer();
void handleButtons();
void updateCountdown();
void updateDisplay();
void updateHeartbeat();
void updateRelays();
void checkAlerts();
void activateHorn();
void deactivateHorn();
void soundBuzzer(int beeps);
void updateBuzzer();
void startSequence();
void resetSequence();
void changeMode();
void processEvent(const SequenceEvent& event);
int32_t getTimeRemaining();
int32_t getTimeElapsed();
const SequenceEvent* getNextEvent();
String getModeString();
String getStateString();
void handleRoot();
void handleStatus();
void handleControl();
void handleConfig();
void handleNotFound();

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n===================================");
  Serial.println("Sailboat Race Starting System");
  Serial.println("Version 1.0.0");
  Serial.println("===================================\n");

  setupHardware();
  setupWiFi();
  setupWebServer();

  Serial.println("System ready!");
  Serial.println("================================\n");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  handleButtons();
  updateCountdown();
  updateDisplay();
  updateHeartbeat();
  updateRelays();
  updateBuzzer();
  server.handleClient();
}

// ============================================================================
// HARDWARE SETUP
// ============================================================================

void setupHardware() {
  Serial.println("Initializing hardware...");

  // Configure relay outputs
  pinMode(RELAY_HORN, OUTPUT);
  pinMode(RELAY_CLASS_LIGHT, OUTPUT);
  pinMode(RELAY_PREP_LIGHT, OUTPUT);
  pinMode(RELAY_AUX, OUTPUT);
  pinMode(RELAY_CH5, OUTPUT);
  pinMode(RELAY_CH6, OUTPUT);
  pinMode(RELAY_CH7, OUTPUT);
  pinMode(RELAY_CH8, OUTPUT);
  
  // Initialize all relays OFF
  digitalWrite(RELAY_HORN, LOW);
  digitalWrite(RELAY_CLASS_LIGHT, LOW);
  digitalWrite(RELAY_PREP_LIGHT, LOW);
  digitalWrite(RELAY_AUX, LOW);
  digitalWrite(RELAY_CH5, LOW);
  digitalWrite(RELAY_CH6, LOW);
  digitalWrite(RELAY_CH7, LOW);
  digitalWrite(RELAY_CH8, LOW);

  // Configure button inputs
  pinMode(BTN_START, INPUT);
  pinMode(BTN_MODE, INPUT);
  pinMode(BTN_HORN_TEST, INPUT);

  // Configure LED outputs
  pinMode(LED_CLASS, OUTPUT);
  pinMode(LED_PREP, OUTPUT);
  pinMode(LED_HEARTBEAT, OUTPUT);
  
  digitalWrite(LED_CLASS, LOW);
  digitalWrite(LED_PREP, LOW);
  digitalWrite(LED_HEARTBEAT, LOW);

  // Configure buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize I2C for OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("ERROR: OLED display initialization failed!");
    for (;;); // Halt
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("  RACE START TIMER");
  display.println("  ================");
  display.println();
  display.println("   Initializing...");
  display.display();

  Serial.println("Hardware initialized successfully");
}

// ============================================================================
// WIFI SETUP
// ============================================================================

void setupWiFi() {
  Serial.println("Setting up WiFi Access Point...");

  // Configure AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Setup mDNS
  if (MDNS.begin("racestart")) {
    Serial.println("mDNS responder started: http://racestart.local");
  } else {
    Serial.println("Error setting up mDNS");
  }

  Serial.println("WiFi setup complete");
}

// ============================================================================
// WEB SERVER SETUP
// ============================================================================

void setupWebServer() {
  Serial.println("Setting up web server...");

  server.on("/", handleRoot);
  server.on("/api/status", handleStatus);
  server.on("/api/control", HTTP_POST, handleControl);
  server.on("/config", handleConfig);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Web server started on port 80");
}

// ============================================================================
// BUTTON HANDLING
// ============================================================================

void handleButtons() {
  unsigned long currentTime = millis();

  // Start/Reset button
  bool btnStartState = digitalRead(BTN_START);
  if (btnStartState && !btnStartPressed && (currentTime - lastBtnStartPress > DEBOUNCE_DELAY)) {
    lastBtnStartPress = currentTime;
    btnStartPressed = true;
    
    if (systemState == STATE_IDLE) {
      startSequence();
    } else {
      resetSequence();
    }
    
    Serial.println("Start/Reset button pressed");
  } else if (!btnStartState) {
    btnStartPressed = false;
  }

  // Mode button (only in idle state)
  bool btnModeState = digitalRead(BTN_MODE);
  if (btnModeState && !btnModePressed && (currentTime - lastBtnModePress > DEBOUNCE_DELAY)) {
    lastBtnModePress = currentTime;
    btnModePressed = true;
    
    if (systemState == STATE_IDLE) {
      changeMode();
    }
    
    Serial.println("Mode button pressed");
  } else if (!btnModeState) {
    btnModePressed = false;
  }

  // Horn test button
  bool btnHornState = digitalRead(BTN_HORN_TEST);
  if (btnHornState && !btnHornPressed && (currentTime - lastBtnHornPress > DEBOUNCE_DELAY)) {
    lastBtnHornPress = currentTime;
    btnHornPressed = true;
    
    if (!hornActive) {
      activateHorn();
      Serial.println("Horn test activated");
    }
  } else if (!btnHornState && btnHornPressed) {
    btnHornPressed = false;
    deactivateHorn();
  }
}

// ============================================================================
// COUNTDOWN LOGIC
// ============================================================================

void updateCountdown() {
  if (systemState == STATE_IDLE) {
    return;
  }

  unsigned long currentTime = millis();
  int32_t elapsedSeconds = (currentTime - startTime) / 1000;
  int32_t timeRemaining = getTimeRemaining();

  // Check for events
  if (currentEventIndex < currentSequenceLength) {
    const SequenceEvent& nextEvent = currentSequence[currentEventIndex];
    int32_t eventTime = abs(nextEvent.timeSeconds);
    
    if (systemState == STATE_ACTIVE && timeRemaining <= 0 && nextEvent.timeSeconds == 0) {
      // Start event reached
      processEvent(nextEvent);
      currentEventIndex++;
      
      // Check if sequence is complete
      if (currentEventIndex >= currentSequenceLength) {
        if (currentMode == MODE_5_MINUTE_REPEAT) {
          // Restart sequence immediately
          Serial.println("Repeating sequence...");
          startSequence();
        } else {
          // Transition to count up
          systemState = STATE_COUNT_UP;
          startTime = currentTime;
          Serial.println("Start reached - counting up");
        }
      }
    } else if (systemState == STATE_ACTIVE && timeRemaining <= eventTime && timeRemaining > 0) {
      // Regular event during countdown
      if (timeRemaining == eventTime) {
        processEvent(nextEvent);
        currentEventIndex++;
      }
    }
  }

  // Check for alerts
  if (systemState == STATE_ACTIVE) {
    checkAlerts();
  }

  // Horn safety timeout
  if (hornActive && (currentTime - hornStartTime > HORN_TIMEOUT)) {
    deactivateHorn();
    Serial.println("WARNING: Horn safety timeout activated!");
  }
}

// ============================================================================
// ALERT CHECKING
// ============================================================================

void checkAlerts() {
  if (currentEventIndex >= currentSequenceLength) {
    return;
  }

  const SequenceEvent& nextEvent = currentSequence[currentEventIndex];
  int32_t timeRemaining = getTimeRemaining();
  int32_t timeToNextEvent = timeRemaining - abs(nextEvent.timeSeconds);

  for (int i = 0; i < NUM_ALERTS; i++) {
    if (!alertSounded[i] && timeToNextEvent == ALERT_TIMES[i]) {
      alertSounded[i] = true;
      
      // Determine beep pattern
      if (ALERT_TIMES[i] >= 20) {
        soundBuzzer(1); // Long beep (handled differently)
      } else if (ALERT_TIMES[i] >= 11) {
        soundBuzzer(1); // Single short beep
      } else if (ALERT_TIMES[i] >= 6) {
        soundBuzzer(2); // Two short beeps
      } else {
        soundBuzzer(3); // Three short beeps
      }
      
      Serial.print("Alert: ");
      Serial.print(ALERT_TIMES[i]);
      Serial.println(" seconds to next event");
      break;
    }
  }
}

// ============================================================================
// EVENT PROCESSING
// ============================================================================

void processEvent(const SequenceEvent& event) {
  Serial.print("Event: ");
  Serial.println(event.displayMsg);

  classFlag = event.classFlag;
  prepFlag = event.prepFlag;

  if (event.hornSignal) {
    activateHorn();
    
    // Deactivate after standard duration
    hornStartTime = millis();
  }
}

// ============================================================================
// SEQUENCE CONTROL
// ============================================================================

void startSequence() {
  Serial.print("Starting sequence: ");
  Serial.println(getModeString());

  systemState = STATE_ACTIVE;
  startTime = millis();
  currentEventIndex = 0;
  classFlag = false;
  prepFlag = false;

  // Clear alert tracking
  for (int i = 0; i < NUM_ALERTS; i++) {
    alertSounded[i] = false;
  }

  // Set sequence based on mode
  switch (currentMode) {
    case MODE_6_MINUTE:
      currentSequence = SEQ_6_MINUTE;
      currentSequenceLength = SEQ_6_MINUTE_LEN;
      break;
    case MODE_5_MINUTE:
      currentSequence = SEQ_5_MINUTE;
      currentSequenceLength = SEQ_5_MINUTE_LEN;
      break;
    case MODE_5_MINUTE_REPEAT:
      currentSequence = SEQ_5_MINUTE_REPEAT;
      currentSequenceLength = SEQ_5_MINUTE_REPEAT_LEN;
      break;
  }
}

void resetSequence() {
  Serial.println("Resetting sequence");

  systemState = STATE_IDLE;
  currentEventIndex = 0;
  classFlag = false;
  prepFlag = false;
  hornActive = false;
  buzzerActive = false;
  
  deactivateHorn();
  digitalWrite(BUZZER_PIN, LOW);
}

void changeMode() {
  currentMode = (CountdownMode)((currentMode + 1) % 3);
  Serial.print("Mode changed to: ");
  Serial.println(getModeString());
}

// ============================================================================
// HORN CONTROL
// ============================================================================

void activateHorn() {
  digitalWrite(RELAY_HORN, HIGH);
  hornActive = true;
  hornStartTime = millis();
}

void deactivateHorn() {
  digitalWrite(RELAY_HORN, LOW);
  hornActive = false;
}

// ============================================================================
// BUZZER CONTROL
// ============================================================================

void soundBuzzer(int beeps) {
  buzzerBeepCount = 0;
  buzzerBeepTarget = beeps;
  buzzerActive = true;
  buzzerStartTime = millis();
  digitalWrite(BUZZER_PIN, HIGH);
}

void updateBuzzer() {
  if (!buzzerActive) {
    return;
  }

  unsigned long elapsed = millis() - buzzerStartTime;
  int beepDuration = (buzzerBeepTarget == 1 && elapsed < BUZZER_LONG_BEEP) ? BUZZER_LONG_BEEP : BUZZER_SHORT_BEEP;

  if (buzzerBeepCount < buzzerBeepTarget) {
    // On phase
    if (elapsed < beepDuration) {
      digitalWrite(BUZZER_PIN, HIGH);
    }
    // Gap phase
    else if (elapsed < beepDuration + BUZZER_GAP) {
      digitalWrite(BUZZER_PIN, LOW);
    }
    // Next beep
    else {
      buzzerBeepCount++;
      buzzerStartTime = millis();
    }
  } else {
    // Done
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }
}

// ============================================================================
// RELAY UPDATE
// ============================================================================

void updateRelays() {
  digitalWrite(RELAY_CLASS_LIGHT, classFlag ? HIGH : LOW);
  digitalWrite(RELAY_PREP_LIGHT, prepFlag ? HIGH : LOW);
  
  digitalWrite(LED_CLASS, classFlag ? HIGH : LOW);
  digitalWrite(LED_PREP, prepFlag ? HIGH : LOW);

  // Deactivate horn after duration
  if (hornActive && (millis() - hornStartTime >= HORN_DURATION)) {
    deactivateHorn();
  }
}

// ============================================================================
// DISPLAY UPDATE
// ============================================================================

void updateDisplay() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastDisplayUpdate < DISPLAY_UPDATE_RATE) {
    return;
  }
  
  lastDisplayUpdate = currentTime;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (systemState == STATE_IDLE) {
    // Idle screen
    display.setCursor(0, 0);
    display.println("  RACE START TIMER");
    display.println("  ================");
    display.println();
    display.print("  Mode: ");
    display.println(getModeString());
    display.println();
    display.println(" Press START to begin");
    display.println(" Mode button to change");
  } else if (systemState == STATE_ACTIVE) {
    // Active countdown
    int32_t timeRemaining = getTimeRemaining();
    int minutes = timeRemaining / 60;
    int seconds = timeRemaining % 60;

    display.setCursor(0, 0);
    display.println(getModeString());
    display.println("------------------");
    
    // Large timer
    display.setTextSize(3);
    display.setCursor(20, 20);
    if (minutes < 10) display.print("0");
    display.print(minutes);
    display.print(":");
    if (seconds < 10) display.print("0");
    display.print(seconds);

    // Next action
    display.setTextSize(1);
    display.setCursor(0, 48);
    if (currentEventIndex < currentSequenceLength) {
      const SequenceEvent& nextEvent = currentSequence[currentEventIndex];
      display.print("NEXT: ");
      display.println(nextEvent.displayMsg);
    }

    // Status indicators
    display.setCursor(0, 56);
    display.print(classFlag ? "●" : "○");
    display.print(" CLASS  ");
    display.print(prepFlag ? "●" : "○");
    display.print(" PREP ");
    display.print(heartbeatState ? "♥" : " ");
  } else if (systemState == STATE_COUNT_UP) {
    // Post-start count up
    int32_t elapsed = getTimeElapsed();
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;

    display.setCursor(0, 0);
    display.println("  START +");
    display.println("  ===================");
    
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println(" Time since start:");
    
    display.setTextSize(3);
    display.setCursor(20, 32);
    if (minutes < 10) display.print("0");
    display.print(minutes);
    display.print(":");
    if (seconds < 10) display.print("0");
    display.print(seconds);

    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print("Press RESET for next");
  }

  display.display();
}

// ============================================================================
// HEARTBEAT UPDATE
// ============================================================================

void updateHeartbeat() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = currentTime;
    heartbeatState = !heartbeatState;
    digitalWrite(LED_HEARTBEAT, heartbeatState ? HIGH : LOW);
  }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

int32_t getTimeRemaining() {
  if (systemState != STATE_ACTIVE || currentSequenceLength == 0) {
    return 0;
  }
  
  unsigned long currentTime = millis();
  int32_t elapsedSeconds = (currentTime - startTime) / 1000;
  int32_t totalSequenceTime = abs(currentSequence[0].timeSeconds);
  
  return totalSequenceTime - elapsedSeconds;
}

int32_t getTimeElapsed() {
  if (systemState != STATE_COUNT_UP) {
    return 0;
  }
  
  unsigned long currentTime = millis();
  return (currentTime - startTime) / 1000;
}

const SequenceEvent* getNextEvent() {
  if (currentEventIndex < currentSequenceLength) {
    return &currentSequence[currentEventIndex];
  }
  return nullptr;
}

String getModeString() {
  switch (currentMode) {
    case MODE_6_MINUTE: return "6-MINUTE";
    case MODE_5_MINUTE: return "5-MINUTE";
    case MODE_5_MINUTE_REPEAT: return "5-MIN REPEAT";
    default: return "UNKNOWN";
  }
}

String getStateString() {
  switch (systemState) {
    case STATE_IDLE: return "IDLE";
    case STATE_ACTIVE: return "ACTIVE";
    case STATE_COUNT_UP: return "COUNT_UP";
    default: return "UNKNOWN";
  }
}

// ============================================================================
// WEB SERVER HANDLERS
// ============================================================================

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Race Start Timer Control</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      max-width: 800px;
      margin: 0 auto;
      padding: 20px;
      background-color: #f0f0f0;
    }
    .container {
      background-color: white;
      border-radius: 10px;
      padding: 20px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }
    h1 {
      color: #1a5490;
      text-align: center;
      margin-bottom: 30px;
    }
    .timer-display {
      font-size: 72px;
      font-weight: bold;
      text-align: center;
      margin: 30px 0;
      color: #333;
      font-family: 'Courier New', monospace;
    }
    .status-row {
      display: flex;
      justify-content: space-around;
      margin: 20px 0;
      flex-wrap: wrap;
    }
    .status-item {
      text-align: center;
      padding: 10px;
    }
    .status-label {
      font-size: 14px;
      color: #666;
    }
    .status-value {
      font-size: 20px;
      font-weight: bold;
      margin-top: 5px;
    }
    .flag-indicator {
      display: inline-block;
      width: 60px;
      height: 60px;
      border-radius: 50%;
      line-height: 60px;
      text-align: center;
      font-size: 14px;
      color: white;
      font-weight: bold;
    }
    .flag-on { opacity: 1; }
    .flag-off { opacity: 0.3; }
    .flag-class { background-color: #d32f2f; }
    .flag-prep { background-color: #1976d2; }
    .next-action {
      background-color: #fff3cd;
      border-left: 4px solid: #ffc107;
      padding: 15px;
      margin: 20px 0;
    }
    .controls {
      display: flex;
      gap: 10px;
      justify-content: center;
      flex-wrap: wrap;
      margin: 20px 0;
    }
    button {
      padding: 15px 30px;
      font-size: 16px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      transition: all 0.3s;
    }
    .btn-start {
      background-color: #4caf50;
      color: white;
    }
    .btn-start:hover {
      background-color: #45a049;
    }
    .btn-reset {
      background-color: #f44336;
      color: white;
    }
    .btn-reset:hover {
      background-color: #da190b;
    }
    .btn-mode {
      background-color: #2196f3;
      color: white;
    }
    .btn-mode:hover {
      background-color: #0b7dda;
    }
    .btn-horn {
      background-color: #ff9800;
      color: white;
    }
    .btn-horn:hover {
      background-color: #e68900;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🚢 Race Start Timer Control</h1>
    
    <div class="status-row">
      <div class="status-item">
        <div class="status-label">Mode</div>
        <div class="status-value" id="mode">--</div>
      </div>
      <div class="status-item">
        <div class="status-label">Status</div>
        <div class="status-value" id="status">--</div>
      </div>
    </div>
    
    <div class="timer-display" id="timer">00:00</div>
    
    <div class="next-action" id="nextAction" style="display:none;">
      <strong>Next Action:</strong> <span id="nextMsg">--</span>
    </div>
    
    <div class="status-row">
      <div class="status-item">
        <div class="flag-indicator flag-class flag-off" id="classFlag">CLASS</div>
      </div>
      <div class="status-item">
        <div class="flag-indicator flag-prep flag-off" id="prepFlag">PREP</div>
      </div>
    </div>
    
    <div class="controls">
      <button class="btn-start" onclick="sendCommand('start')">START</button>
      <button class="btn-reset" onclick="sendCommand('reset')">RESET</button>
      <button class="btn-mode" onclick="sendCommand('mode')">MODE</button>
      <button class="btn-horn" onclick="sendCommand('horn')">TEST HORN</button>
    </div>
  </div>
  
  <script>
    function updateStatus() {
      fetch('/api/status')
        .then(response => response.json())
        .then(data => {
          document.getElementById('mode').textContent = data.mode;
          document.getElementById('status').textContent = data.state;
          
          let minutes = Math.floor(Math.abs(data.time) / 60);
          let seconds = Math.abs(data.time) % 60;
          let sign = data.time < 0 ? '-' : (data.state === 'COUNT_UP' ? '+' : '');
          document.getElementById('timer').textContent = 
            sign + String(minutes).padStart(2, '0') + ':' + String(seconds).padStart(2, '0');
          
          if (data.nextEvent && data.state === 'ACTIVE') {
            document.getElementById('nextAction').style.display = 'block';
            document.getElementById('nextMsg').textContent = data.nextEvent;
          } else {
            document.getElementById('nextAction').style.display = 'none';
          }
          
          let classFlag = document.getElementById('classFlag');
          classFlag.className = 'flag-indicator flag-class ' + (data.classFlag ? 'flag-on' : 'flag-off');
          
          let prepFlag = document.getElementById('prepFlag');
          prepFlag.className = 'flag-indicator flag-prep ' + (data.prepFlag ? 'flag-on' : 'flag-off');
        })
        .catch(error => console.error('Error:', error));
    }
    
    function sendCommand(action) {
      fetch('/api/control', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({action: action})
      })
      .then(response => response.json())
      .then(data => {
        console.log('Command sent:', action);
        updateStatus();
      })
      .catch(error => console.error('Error:', error));
    }
    
    // Update every 500ms
    setInterval(updateStatus, 500);
    updateStatus();
  </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleStatus() {
  StaticJsonDocument<512> doc;
  
  doc["mode"] = getModeString();
  doc["state"] = getStateString();
  doc["classFlag"] = classFlag;
  doc["prepFlag"] = prepFlag;
  doc["hornActive"] = hornActive;
  
  if (systemState == STATE_ACTIVE) {
    doc["time"] = getTimeRemaining();
    const SequenceEvent* nextEvent = getNextEvent();
    if (nextEvent) {
      doc["nextEvent"] = nextEvent->displayMsg;
    }
  } else if (systemState == STATE_COUNT_UP) {
    doc["time"] = getTimeElapsed();
  } else {
    doc["time"] = 0;
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleControl() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }
  
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  
  if (error) {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }
  
  String action = doc["action"].as<String>();
  
  if (action == "start") {
    if (systemState == STATE_IDLE) {
      startSequence();
    }
  } else if (action == "reset") {
    resetSequence();
  } else if (action == "mode") {
    if (systemState == STATE_IDLE) {
      changeMode();
    }
  } else if (action == "horn") {
    if (!hornActive) {
      activateHorn();
    }
  }
  
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleConfig() {
  String html = "<html><body><h1>Configuration</h1><p>Configuration page coming soon...</p></body></html>";
  server.send(200, "text/html", html);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}
