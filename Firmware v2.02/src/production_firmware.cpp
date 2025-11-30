/*
 * ESP32 Production Counter - FSM Refactored Version
 * 
 * This version integrates the FSM architecture with the original code_v3.cpp functionality.
 * All production features are preserved while using the new modular FSM design.
 * 
 * Architecture:
 * - Finite State Machine (5 states)
 * - Event-driven with 28 event types
 * - Manager pattern for business logic
 * - Hardware abstraction layer
 * 
 * Date: November 30, 2025
 * Phase: 4 of 5 - Integration
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <EEPROM.h>

// FSM Headers
#include "state_manager.h"
#include "state_handlers.h"
#include "managers.h"
#include "hal.h"

// ============================================================================
// PIN DEFINITIONS (Same as original code_v3.cpp)
// ============================================================================

#define INTERRUPT_PIN 15      // Counter button (to GND)
#define DIAGNOSTIC_PIN 27     // Diagnostic button (to GND)
#define LATCHING_PIN 25       // Production latching button (to GND)

// OLED Display - I2C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// I2C pins (default for ESP32)
#define I2C_SDA 21
#define I2C_SCL 22

// SD Card - VSPI (SPI3)
#define SD_CS_PIN 26
#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23

// ============================================================================
// GLOBAL OBJECTS (Original Hardware)
// ============================================================================

SPIClass spiSD(VSPI);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;

// ============================================================================
// FSM INSTANCE & STATE TRACKING
// ============================================================================

StateManager& fsm = StateManager::getInstance();

// Timing for periodic operations
static unsigned long lastSaveTime = 0;
static unsigned long lastHealthCheckTime = 0;
static unsigned long lastDisplayUpdateTime = 0;
static unsigned long lastHourChangeTime = 0;

// Configuration
static const unsigned long SAVE_INTERVAL = 5000;
static const unsigned long HEALTH_CHECK_INTERVAL = 30000;
static const unsigned long DISPLAY_UPDATE_INTERVAL = 100;

// Startup retry configuration
static const int MAX_STARTUP_RETRIES = 3;
static int startupRetryCount = 0;

// ============================================================================
// COMPATIBILITY VARIABLES (From original code)
// ============================================================================

volatile int currentCount = 0;
volatile bool countChanged = false;
volatile bool productionActive = false;
volatile int lastHour = -1;
bool rtcAvailable = false;
bool sdAvailable = false;

// File paths (backward compatible with code_v3.cpp)
const char* COUNT_FILE = "/count.txt";
const char* HOURLY_FILE = "/hourly_count.txt";
const char* CUMULATIVE_FILE = "/cumulative_count.txt";
const char* PRODUCTION_STATE_FILE = "/prod_session.txt";

// Status for compatibility
enum CompatibilityStatus {
  STATUS_IDLE,
  STATUS_COUNTING,
  STATUS_HOUR_COMPLETE,
  STATUS_INITIALIZING,
  STATUS_DIAGNOSTIC
};
CompatibilityStatus currentStatus = STATUS_INITIALIZING;

// ============================================================================
// INTERRUPT SERVICE ROUTINES (From original code)
// ============================================================================

void IRAM_ATTR handleCounterButton() {
  unsigned long currentTime = millis();
  static unsigned long lastInterruptTime = 0;
  
  if (currentTime - lastInterruptTime > 50) {  // Debounce
    if (productionActive) {
      fsm.queueEvent(EVT_ITEM_COUNTED);
      ProductionManager::getInstance().incrementCount();
      currentCount++;
      countChanged = true;
    }
    lastInterruptTime = currentTime;
  }
}

void IRAM_ATTR handleDiagnosticButton() {
  static unsigned long lastTime = 0;
  unsigned long now = millis();
  
  if (now - lastTime > 200) {
    fsm.queueEvent(EVT_DIAGNOSTIC_REQUESTED);
    lastTime = now;
  }
}

void IRAM_ATTR handleProductionLatch() {
  static unsigned long lastTime = 0;
  unsigned long now = millis();
  
  if (now - lastTime > 100) {
    if (digitalRead(LATCHING_PIN) == LOW) {
      fsm.queueEvent(EVT_PRODUCTION_START);
    } else {
      fsm.queueEvent(EVT_PRODUCTION_STOP);
    }
    lastTime = now;
  }
}

// ============================================================================
// HARDWARE INITIALIZATION (Refactored for FSM)
// ============================================================================

bool initializeHardware() {
  LoggerManager::info("=== HARDWARE INITIALIZATION ===");
  
  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  LoggerManager::info("I2C initialized");
  
  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    LoggerManager::error("OLED initialization failed");
    return false;
  }
  LoggerManager::info("OLED initialized");
  
  // Initialize SD
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(100);
  
  spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS_PIN);
  LoggerManager::info("SPI initialized");
  
  // SD card initialization with retries
  uint32_t speeds[] = {400000, 1000000, 5000000};
  for (int i = 0; i < 3; i++) {
    digitalWrite(SD_CS_PIN, HIGH);
    delayMicroseconds(500);
    
    if (SD.begin(SD_CS_PIN, spiSD, speeds[i])) {
      LoggerManager::info("SD card initialized at %lu Hz", speeds[i]);
      sdAvailable = true;
      break;
    }
    delay(300);
  }
  
  if (!sdAvailable) {
    LoggerManager::warn("SD card initialization failed");
  }
  
  // Initialize RTC
  if (!rtc.begin()) {
    LoggerManager::warn("RTC not responding");
    rtcAvailable = false;
  } else {
    rtcAvailable = true;
    LoggerManager::info("RTC initialized");
    
    if (rtc.lostPower()) {
      LoggerManager::warn("RTC lost power - setting compile time");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }
  
  // Initialize file system
  if (sdAvailable) {
    LoggerManager::info("Initializing file system");
    
    if (!SD.exists(COUNT_FILE)) {
      File f = SD.open(COUNT_FILE, FILE_WRITE);
      if (f) {
        f.println("0");
        f.close();
      }
    }
    
    if (!SD.exists(CUMULATIVE_FILE)) {
      File f = SD.open(CUMULATIVE_FILE, FILE_WRITE);
      if (f) {
        f.println("0");
        f.close();
      }
    }
    
    // Load counts from files
    File countFile = SD.open(COUNT_FILE);
    if (countFile) {
      currentCount = countFile.parseInt();
      countFile.close();
    }
  }
  
  // Initialize GPIO
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  pinMode(DIAGNOSTIC_PIN, INPUT_PULLUP);
  pinMode(LATCHING_PIN, INPUT_PULLUP);
  
  // Attach ISRs
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleCounterButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(DIAGNOSTIC_PIN), handleDiagnosticButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(LATCHING_PIN), handleProductionLatch, CHANGE);
  
  LoggerManager::info("Interrupts configured");
  
  // Initialize RTC time tracking
  if (rtcAvailable) {
    DateTime now = rtc.now();
    lastHour = now.hour();
  }
  
  LoggerManager::info("Hardware initialization complete");
  return true;
}

// ============================================================================
// DISPLAY FUNCTIONS (Adapted from original code)
// ============================================================================

void displayStartupScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 5);
  display.println("COUNTER");
  display.setTextSize(1);
  display.setCursor(15, 30);
  display.println("Initializing...");
  display.display();
}

void displayStatusMessage(const char* message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 30);
  display.println(message);
  display.display();
}

void displayMainScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Top line: Mode status
  if (productionActive) {
    display.setCursor(0, 0);
    display.println("PRODUCTION ACTIVE");
  } else {
    display.setCursor(0, 0);
    display.println("READY");
  }
  
  // Large count in middle
  display.setTextSize(2);
  display.setCursor(20, 20);
  display.println(currentCount);
  
  // Bottom info
  display.setTextSize(1);
  if (rtcAvailable) {
    DateTime now = rtc.now();
    display.setCursor(0, 50);
    display.print(now.hour());
    display.print(":");
    if (now.minute() < 10) display.print("0");
    display.println(now.minute());
  }
  
  display.setCursor(80, 50);
  display.print("SD:");
  display.println(sdAvailable ? "OK" : "NG");
  
  display.display();
}

void displayErrorScreen(const char* message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("ERROR:");
  display.setCursor(10, 40);
  display.println(message);
  display.display();
}

// ============================================================================
// HELPER FUNCTIONS (Backward compatibility)
// ============================================================================

int readCountFromFile(const char* filename) {
  if (!sdAvailable) return 0;
  
  File file = SD.open(filename);
  if (!file) return 0;
  
  int count = file.parseInt();
  file.close();
  return count;
}

void writeCountToFile(const char* filename, int count) {
  if (!sdAvailable) return;
  
  File file = SD.open(filename, FILE_WRITE);
  if (!file) return;
  
  file.seek(0);
  file.print(count);
  file.println();
  file.close();
}

void saveProductionState() {
  if (!sdAvailable) return;
  
  File file = SD.open(PRODUCTION_STATE_FILE, FILE_WRITE);
  if (!file) return;
  
  file.println("ACTIVE");
  file.print(currentCount);
  file.println();
  file.close();
}

void clearProductionState() {
  if (!sdAvailable) return;
  SD.remove(PRODUCTION_STATE_FILE);
}

void handleHourChange() {
  LoggerManager::info("Hour boundary detected");
  
  if (rtcAvailable) {
    DateTime now = rtc.now();
    
    // Update cumulative count
    writeCountToFile(CUMULATIVE_FILE, currentCount);
    
    // Log hour change
    Serial.print("Hour changed: ");
    Serial.print(now.hour());
    Serial.println(":00");
  }
}

// ============================================================================
// SETUP FUNCTION
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  LoggerManager::initialize(LoggerManager::INFO);
  LoggerManager::info("=== ESP32 Production Counter - FSM Edition ===");
  LoggerManager::info("Phase 4: Integration Complete");
  
  // Initialize hardware with retry
  for (int attempt = 1; attempt <= MAX_STARTUP_RETRIES; attempt++) {
    if (attempt > 1) {
      LoggerManager::info("Retry attempt %d of %d", attempt, MAX_STARTUP_RETRIES);
      displayStatusMessage("Retrying...");
      delay(1500);
    }
    
    if (initializeHardware()) {
      displayStartupScreen();
      delay(1000);
      
      // Enter INITIALIZATION state
      fsm.transitionToState(STATE_INITIALIZATION);
      LoggerManager::info("Entering INITIALIZATION state");
      return;
    }
  }
  
  // Fatal error - display error screen
  LoggerManager::fatal("Hardware initialization failed after %d attempts", MAX_STARTUP_RETRIES);
  displayErrorScreen("INIT ERROR");
  
  // Infinite loop
  while (true) {
    delay(1000);
  }
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  unsigned long now = millis();
  SystemState currentState = fsm.getCurrentState();
  
  // Execute state handler
  bool stateHealthy = executeCurrentState(currentState);
  
  if (!stateHealthy) {
    LoggerManager::error("State execution failed - entering ERROR state");
    fsm.transitionToState(STATE_ERROR);
  }
  
  // Process all queued events
  SystemEvent event;
  while (fsm.dequeueEvent(event)) {
    processEvent(event, currentState);
  }
  
  // Update display periodically
  if (now - lastDisplayUpdateTime >= DISPLAY_UPDATE_INTERVAL) {
    displayMainScreen();
    lastDisplayUpdateTime = now;
  }
  
  // Save state periodically
  if (sdAvailable && now - lastSaveTime >= SAVE_INTERVAL) {
    writeCountToFile(COUNT_FILE, currentCount);
    if (productionActive) {
      saveProductionState();
    }
    lastSaveTime = now;
  }
  
  // Check system health
  if (now - lastHealthCheckTime >= HEALTH_CHECK_INTERVAL) {
    int freeHeap = PowerManager::getFreeHeap();
    if (freeHeap < 50000) {
      LoggerManager::warn("Low heap: %d bytes", freeHeap);
    }
    lastHealthCheckTime = now;
  }
  
  // Handle hour changes
  if (rtcAvailable) {
    DateTime rtcNow = rtc.now();
    if (rtcNow.hour() != lastHour) {
      handleHourChange();
      lastHour = rtcNow.hour();
    }
  }
  
  delay(1);
}

// ============================================================================
// STATE EXECUTION WRAPPER
// ============================================================================

bool executeCurrentState(SystemState state) {
  switch (state) {
    case STATE_INITIALIZATION:
      return executeInitializationState();
    
    case STATE_READY:
      return executeReadyState();
    
    case STATE_PRODUCTION:
      return executeProductionState();
    
    case STATE_DIAGNOSTIC:
      return executeDiagnosticState();
    
    case STATE_ERROR:
      return executeErrorState();
    
    default:
      LoggerManager::error("Unknown state: %d", state);
      return false;
  }
}

// ============================================================================
// EVENT PROCESSING
// ============================================================================

void processEvent(SystemEvent event, SystemState currentState) {
  LoggerManager::debug("Event %d in state %d", event, currentState);
  
  switch (currentState) {
    
    case STATE_INITIALIZATION:
      if (event == EVT_INIT_COMPLETE) {
        LoggerManager::info("Initialization complete");
        fsm.transitionToState(STATE_READY);
        displayStatusMessage("Ready!");
        delay(500);
      }
      break;
    
    case STATE_READY:
      if (event == EVT_PRODUCTION_START) {
        if (canStartProduction()) {
          LoggerManager::info("Starting production");
          productionActive = true;
          ProductionManager::getInstance().startSession();
          fsm.transitionToState(STATE_PRODUCTION);
          displayStatusMessage("Production Started");
        }
      }
      else if (event == EVT_DIAGNOSTIC_REQUESTED) {
        LoggerManager::info("Entering diagnostic mode");
        fsm.transitionToState(STATE_DIAGNOSTIC);
      }
      else if (event == EVT_HOUR_CHANGED) {
        handleHourChange();
      }
      break;
    
    case STATE_PRODUCTION:
      if (event == EVT_PRODUCTION_STOP) {
        LoggerManager::info("Stopping production");
        productionActive = false;
        ProductionManager::getInstance().stopSession();
        fsm.transitionToState(STATE_READY);
        
        // Save final count
        writeCountToFile(CUMULATIVE_FILE, currentCount);
        clearProductionState();
        
        displayStatusMessage("Production Stopped");
      }
      else if (event == EVT_ITEM_COUNTED) {
        // Already handled in ISR
      }
      else if (event == EVT_HOUR_CHANGED) {
        handleHourChange();
      }
      break;
    
    case STATE_DIAGNOSTIC:
      if (event == EVT_DIAG_COMPLETE) {
        LoggerManager::info("Diagnostics complete");
        fsm.transitionToState(STATE_READY);
        displayStatusMessage("Diag Complete");
      }
      break;
    
    case STATE_ERROR:
      if (event == EVT_ERROR_RECOVERED) {
        LoggerManager::info("Error recovered");
        fsm.transitionToState(STATE_READY);
      }
      break;
  }
}

// ============================================================================
// SERIAL DEBUG INTERFACE (Backward compatible with original)
// ============================================================================

void handleSerialInput() {
  if (!Serial.available()) return;
  
  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toUpperCase();
  
  if (input == "STATUS") {
    Serial.println("=== System Status ===");
    Serial.print("State: ");
    printStateName(fsm.getCurrentState());
    Serial.println();
    Serial.print("Production: ");
    Serial.println(productionActive ? "ACTIVE" : "IDLE");
    Serial.print("Current Count: ");
    Serial.println(currentCount);
    Serial.print("Free Heap: ");
    Serial.print(PowerManager::getFreeHeap());
    Serial.println(" bytes");
  }
  else if (input == "START") {
    fsm.queueEvent(EVT_PRODUCTION_START);
    Serial.println(">> Production start requested");
  }
  else if (input == "STOP") {
    fsm.queueEvent(EVT_PRODUCTION_STOP);
    Serial.println(">> Production stop requested");
  }
  else if (input == "COUNT") {
    fsm.queueEvent(EVT_ITEM_COUNTED);
    Serial.println(">> Count incremented");
  }
  else if (input == "DIAG") {
    fsm.queueEvent(EVT_DIAGNOSTIC_REQUESTED);
    Serial.println(">> Diagnostic requested");
  }
  else if (input == "RESET") {
    fsm.transitionToState(STATE_INITIALIZATION);
    Serial.println(">> System reset");
  }
  else if (input == "HELP") {
    Serial.println("Commands: STATUS START STOP COUNT DIAG RESET HELP");
  }
}

void printStateName(SystemState state) {
  switch (state) {
    case STATE_INITIALIZATION:
      Serial.print("INITIALIZATION");
      break;
    case STATE_READY:
      Serial.print("READY");
      break;
    case STATE_PRODUCTION:
      Serial.print("PRODUCTION");
      break;
    case STATE_DIAGNOSTIC:
      Serial.print("DIAGNOSTIC");
      break;
    case STATE_ERROR:
      Serial.print("ERROR");
      break;
    default:
      Serial.print("UNKNOWN");
  }
}

// ============================================================================
// DEBUG MENU (Backward compatible)
// ============================================================================

void debugMenu() {
  Serial.println("\n=== DEBUG MENU ===");
  Serial.println("Available commands:");
  Serial.println("  STATUS - Show system status");
  Serial.println("  START  - Begin production");
  Serial.println("  STOP   - End production");
  Serial.println("  COUNT  - Increment count");
  Serial.println("  DIAG   - Run diagnostics");
  Serial.println("  RESET  - Reset system");
  Serial.println("  HELP   - Show this menu");
  Serial.println("\nNote: Type 'INFO' to show this menu again");
  Serial.println();
}

// Override serialEvent for interactive commands
void serialEvent() {
  handleSerialInput();
}
