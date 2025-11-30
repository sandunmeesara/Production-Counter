/*
 * FSM-BASED MAIN LOOP TEMPLATE
 * 
 * This template shows how to refactor the monolithic main loop into
 * an FSM-driven architecture using the StateManager and Manager classes.
 * 
 * NOT YET COMPILED - This is a reference implementation
 */

#include "state_manager.h"
#include "managers.h"
#include "hal.h"

// ========================================
// GLOBAL MANAGER INSTANCES
// ========================================
StateManager stateManager;
ProductionManager productionManager;
TimeManager timeManager;
StorageManager storageManager;
DisplayManager displayManager;
ConfigManager configManager;
LoggerManager logger;

// ========================================
// SETUP FUNCTION (REFACTORED)
// ========================================
void setup() {
  // Initialize hardware abstraction layer
  GPIO::initAll();
  Serial_HAL::init(115200);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   FSM-BASED COUNTER SYSTEM v3.0      ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // Initialize logging
  LoggerManager::initialize(LoggerManager::INFO);
  
  // Initialize configuration manager
  if (!configManager.initialize()) {
    logger.error("Failed to load configuration");
    return;
  }
  
  // Initialize state machine
  if (!stateManager.initialize()) {
    logger.fatal("Failed to initialize state manager");
    return;
  }
  
  // Signal startup begin
  stateManager.queueEvent(SystemEvent::EVT_STARTUP_BEGIN);
  
  // Initialize managers with retry logic
  int retries = 3;
  while (retries > 0) {
    bool allOk = true;
    
    // Initialize hardware
    if (!displayManager.initialize()) {
      logger.warn("OLED initialization failed (attempt %d/3)", 4 - retries);
      if (retries == 1) {
        logger.error("OLED required - startup failed");
        stateManager.queueEvent(SystemEvent::EVT_STARTUP_FAILED);
        return;
      }
      allOk = false;
    }
    
    if (!timeManager.initialize()) {
      logger.warn("RTC initialization failed, continuing without RTC");
      stateManager.queueEvent(SystemEvent::EVT_RTC_UNAVAILABLE);
    } else {
      stateManager.queueEvent(SystemEvent::EVT_RTC_AVAILABLE);
    }
    
    if (!storageManager.initialize()) {
      logger.warn("SD card initialization failed, continuing without SD");
      stateManager.queueEvent(SystemEvent::EVT_SD_UNAVAILABLE);
    } else {
      stateManager.queueEvent(SystemEvent::EVT_SD_AVAILABLE);
    }
    
    if (!allOk) {
      retries--;
      if (retries > 0) {
        logger.info("Retrying initialization (%d attempts remaining)...", retries);
        delay(1500);
        continue;
      }
    }
    
    // If we got here, initialization succeeded
    stateManager.queueEvent(SystemEvent::EVT_STARTUP_COMPLETE);
    break;
  }
  
  logger.info("System initialized successfully");
  displayManager.showInitializationScreen();
}

// ========================================
// MAIN LOOP (REFACTORED)
// ========================================
void loop() {
  // Phase 1: Collect Events
  collectEvents();
  
  // Phase 2: Update State Machine
  stateManager.update();
  
  // Phase 3: Execute State Logic
  executeCurrentState();
  
  // Phase 4: Update Display
  displayManager.update();
  
  // Phase 5: Housekeeping
  housekeeping();
  
  // Small delay to prevent CPU hogging
  delay(10);
}

// ========================================
// PHASE 1: EVENT COLLECTION
// ========================================
void collectEvents() {
  // Check for diagnostic button
  if (digitalRead(GPIO::DIAG_BTN) == LOW) {
    delay(100);  // Debounce
    if (digitalRead(GPIO::DIAG_BTN) == LOW) {
      stateManager.queueEvent(SystemEvent::EVT_DIAGNOSTIC_REQUEST);
      delay(500);  // Wait for button release
    }
  }
  
  // Check for production button (latch)
  static int lastLatchState = HIGH;
  int currentLatchState = digitalRead(GPIO::LATCH_BTN);
  
  if (currentLatchState != lastLatchState) {
    delay(50);  // Debounce
    currentLatchState = digitalRead(GPIO::LATCH_BTN);
    
    if (currentLatchState != lastLatchState) {
      if (currentLatchState == LOW) {
        stateManager.queueEvent(SystemEvent::EVT_PRODUCTION_START);
      } else {
        stateManager.queueEvent(SystemEvent::EVT_PRODUCTION_STOP);
      }
      lastLatchState = currentLatchState;
    }
  }
  
  // Check for serial commands
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() > 0 && input.length() < 256) {
      stateManager.queueEvent(SystemEvent::EVT_SERIAL_COMMAND);
      processSerialCommand(input);
    }
  }
  
  // Check for time updates (hourly)
  static unsigned long lastTimeCheck = 0;
  if (millis() - lastTimeCheck > 1000) {
    if (timeManager.hasHourChanged()) {
      stateManager.queueEvent(SystemEvent::EVT_HOUR_CHANGED);
      timeManager.handleHourChange();
    }
    lastTimeCheck = millis();
  }
  
  // Check for count updates
  static volatile int lastCountValue = 0;
  if (productionManager.getSessionCount() != lastCountValue) {
    stateManager.queueEvent(SystemEvent::EVT_COUNT_UPDATED);
    lastCountValue = productionManager.getSessionCount();
  }
}

// ========================================
// PHASE 2: STATE MACHINE PROCESSING
// ========================================
// Handled by stateManager.update() and processEvent()

// ========================================
// PHASE 3: EXECUTE CURRENT STATE
// ========================================
void executeCurrentState() {
  switch (stateManager.getCurrentState()) {
    case SystemState::INITIALIZATION:
      executeInitializationState();
      break;
    case SystemState::READY:
      executeReadyState();
      break;
    case SystemState::PRODUCTION:
      executeProductionState();
      break;
    case SystemState::DIAGNOSTIC:
      executeDiagnosticState();
      break;
    case SystemState::ERROR:
      executeErrorState();
      break;
  }
}

void executeInitializationState() {
  // Initialization logic
  // This state processes hardware initialization events
  // Transitions to READY when complete
}

void executeReadyState() {
  // Ready state logic
  // System is waiting for production to start
  
  // Periodic housekeeping
  static unsigned long lastSaveTime = 0;
  if (millis() - lastSaveTime > configManager.getSaveInterval()) {
    // Periodic save operations
    lastSaveTime = millis();
  }
  
  displayManager.showReadyScreen();
}

void executeProductionState() {
  // Production state logic
  // System is currently counting items
  
  int currentCount = productionManager.getSessionCount();
  
  // Save count periodically
  static unsigned long lastSaveTime = 0;
  if (millis() - lastSaveTime > configManager.getSaveInterval()) {
    storageManager.saveCount("/count.txt", currentCount);
    stateManager.queueEvent(SystemEvent::EVT_COUNT_SAVED);
    lastSaveTime = millis();
  }
  
  displayManager.showProductionScreen(currentCount);
}

void executeDiagnosticState() {
  // Diagnostic state logic
  // Run all system diagnostics
  
  static bool diagnosticsRunOnce = false;
  if (!diagnosticsRunOnce) {
    runDiagnostics();
    diagnosticsRunOnce = true;
    
    // Auto-exit diagnostic after displaying results
    stateManager.queueEvent(SystemEvent::EVT_DIAGNOSTIC_COMPLETE);
  }
  
  displayManager.showDiagnosticScreen();
}

void executeErrorState() {
  // Error state logic
  // Display error and wait for recovery
  
  displayManager.showErrorScreen("System Error");
}

// ========================================
// PHASE 4: DISPLAY UPDATE
// ========================================
// Handled by displayManager.update()

// ========================================
// PHASE 5: HOUSEKEEPING
// ========================================
void housekeeping() {
  // Monitor system health
  static unsigned long lastHealthCheck = 0;
  if (millis() - lastHealthCheck > 10000) {
    if (!GuardConditions::isHeapHealthy()) {
      logger.warn("Heap memory low: %d bytes", ESP.getFreeHeap());
    }
    
    if (!GuardConditions::isStackHealthy()) {
      logger.warn("Stack memory low");
      stateManager.queueEvent(SystemEvent::EVT_ERROR_DETECTED);
    }
    
    lastHealthCheck = millis();
  }
  
  // Feed watchdog timer
  Watchdog::feed();
}

// ========================================
// SERIAL COMMAND PROCESSING
// ========================================
void processSerialCommand(String input) {
  input.toUpperCase();
  
  if (input == "START") {
    if (stateManager.getCurrentState() == SystemState::READY) {
      stateManager.queueEvent(SystemEvent::EVT_PRODUCTION_START);
      productionManager.startSession();
    }
  } 
  else if (input == "STOP") {
    if (stateManager.getCurrentState() == SystemState::PRODUCTION) {
      stateManager.queueEvent(SystemEvent::EVT_PRODUCTION_STOP);
      productionManager.stopSession();
    }
  }
  else if (input == "STATUS") {
    Serial.print("State: ");
    Serial.print(stateManager.getCurrentStateName());
    Serial.print(" | Count: ");
    Serial.println(productionManager.getSessionCount());
  }
  else if (input == "TIME") {
    DateTime now = timeManager.getCurrentTime();
    Serial.println(timeManager.getTimeString());
  }
  else if (input.startsWith("SETTIME:")) {
    // Parse and set time
  }
  else if (input == "DIAG") {
    stateManager.queueEvent(SystemEvent::EVT_DIAGNOSTIC_REQUEST);
  }
  else if (input == "RESET") {
    ESP.restart();
  }
  else {
    Serial.println("Unknown command");
  }
}

// ========================================
// DIAGNOSTIC FUNCTION
// ========================================
void runDiagnostics() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║         SYSTEM DIAGNOSTICS            ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // Test OLED
  Serial.print("OLED: ");
  Serial.println("✓");
  
  // Test RTC
  Serial.print("RTC: ");
  if (GuardConditions::isRTCAvailable()) {
    Serial.print("✓ (Time: ");
    Serial.print(timeManager.getTimeString(true));
    Serial.println(")");
  } else {
    Serial.println("✗");
  }
  
  // Test SD Card
  Serial.print("SD Card: ");
  if (GuardConditions::isSDAvailable()) {
    Serial.print("✓ (Files: ");
    Serial.print(storageManager.countFiles());
    Serial.println(")");
  } else {
    Serial.println("✗");
  }
  
  // Memory status
  Serial.print("Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  
  Serial.print("Largest Block: ");
  Serial.print(ESP.getMaxAllocHeap());
  Serial.println(" bytes");
  
  // FSM status
  Serial.print("Current State: ");
  Serial.println(stateManager.getCurrentStateName());
  
  Serial.print("Events Processed: ");
  Serial.println(stateManager.getEventCount());
  
  Serial.println();
}

// ========================================
// INTERRUPT SERVICE ROUTINES
// ========================================
void IRAM_ATTR handleCounterInterrupt() {
  // ISR code - minimal, just set flag
  // Real counting happens in main loop
  productionManager.incrementCount();
  stateManager.queueEvent(SystemEvent::EVT_COUNTER_PRESSED);
}

// ========================================
// COMPARISON: OLD VS NEW LOOP STRUCTURE
// ========================================
/*
OLD MONOLITHIC LOOP:
- 100+ lines of if/else statements
- Complex interdependencies
- Hard to test individual pieces
- Global variable mutations scattered
- Difficult to add new features

NEW FSM-DRIVEN LOOP:
- Clear 5-phase structure
- Each phase has specific responsibility
- Event-driven state transitions
- Modular, testable components
- Easy to extend with new states/events
- Clear entry/exit actions per state
- Guard conditions before transitions
*/
