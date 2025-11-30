/**
 * FSM Main Loop Integration Template
 * 
 * This file demonstrates how to integrate the FSM foundation with the state handlers
 * to create a complete firmware loop that uses the finite state machine architecture.
 * 
 * Copy this code into your main Arduino sketch (.ino file) and adapt as needed.
 * 
 * Filename: fsm_main_integration.cpp
 * Date: November 30, 2025
 * Phase: 3 of 5
 */

#include "state_manager.h"
#include "state_handlers.h"
#include "managers.h"
#include "hal.h"

// ============================================================================
// GLOBAL STATE MACHINE INSTANCE
// ============================================================================

StateManager& fsm = StateManager::getInstance();

// ============================================================================
// ISR CALLBACKS (called from GPIO interrupts)
// ============================================================================

/**
 * Counter Button ISR
 * Called when counter button is pressed
 */
void IRAM_ATTR counterButtonISR() {
  // Queue event for processing in main loop (safe from ISR)
  fsm.queueEvent(EVT_ITEM_COUNTED);
  
  // Direct increment (if you want immediate response)
  // ProductionManager::getInstance().incrementCount();
}

/**
 * Diagnostic Button ISR
 * Called when diagnostic button is pressed
 */
void IRAM_ATTR diagnosticButtonISR() {
  // Queue diagnostic request
  fsm.queueEvent(EVT_DIAGNOSTIC_REQUESTED);
}

/**
 * Production Latch ISR
 * Called when production latch changes state
 */
void IRAM_ATTR productionLatchISR() {
  // Toggle production state
  if (ProductionManager::getInstance().isSessionActive()) {
    fsm.queueEvent(EVT_PRODUCTION_STOP);
  } else {
    fsm.queueEvent(EVT_PRODUCTION_START);
  }
}

// ============================================================================
// SETUP (Arduino initialization)
// ============================================================================

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000);  // Wait for serial to stabilize
  
  LoggerManager::initialize(LoggerManager::INFO);
  LoggerManager::info("=== Production Counter Firmware Starting ===");
  LoggerManager::info("FSM Mode - Phase 3 Integration");
  
  // Enter INITIALIZATION state
  fsm.transitionToState(STATE_INITIALIZATION);
  
  // Attach ISRs
  attachInterrupt(digitalPinToInterrupt(15), counterButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(27), diagnosticButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(25), productionLatchISR, CHANGE);
  
  LoggerManager::info("Interrupts attached");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Get current state
  SystemState currentState = fsm.getCurrentState();
  
  // Execute state-specific logic
  bool stateHealthy = executeCurrentState(currentState);
  
  if (!stateHealthy) {
    LoggerManager::error("State handler returned error");
    fsm.transitionToState(STATE_ERROR);
  }
  
  // Process queued events
  SystemEvent event;
  while (fsm.dequeueEvent(event)) {
    processEvent(event, currentState);
  }
  
  // Periodic tasks
  handleHousekeeping();
}

// ============================================================================
// STATE EXECUTION
// ============================================================================

/**
 * Execute the logic for the current state
 * 
 * This is called once per loop iteration and delegates to the appropriate
 * state handler function.
 * 
 * @param state Current FSM state
 * @return true if state executed successfully, false if recovery needed
 */
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

/**
 * Process events and execute state transitions
 * 
 * This function handles the event-to-state-transition logic.
 * It checks guard conditions before allowing transitions.
 * 
 * @param event The event to process
 * @param currentState The current state
 */
void processEvent(SystemEvent event, SystemState currentState) {
  LoggerManager::debug("Processing event: %d in state: %d", event, currentState);
  
  // Handle events based on current state
  switch (currentState) {
    
    // ====================================================================
    // INITIALIZATION STATE TRANSITIONS
    // ====================================================================
    case STATE_INITIALIZATION:
      if (event == EVT_INIT_COMPLETE) {
        LoggerManager::info("Transitioning to READY");
        fsm.transitionToState(STATE_READY);
      }
      else if (event == EVT_SYSTEM_ERROR) {
        LoggerManager::error("Initialization failed");
        fsm.transitionToState(STATE_ERROR);
      }
      break;
    
    // ====================================================================
    // READY STATE TRANSITIONS
    // ====================================================================
    case STATE_READY:
      if (event == EVT_PRODUCTION_START) {
        if (canStartProduction()) {
          LoggerManager::info("Starting production session");
          ProductionManager::getInstance().startSession();
          fsm.transitionToState(STATE_PRODUCTION);
        } else {
          LoggerManager::warn("Cannot start production - guard condition failed");
          DisplayManager::getInstance().showErrorScreen("Cannot Start");
        }
      }
      else if (event == EVT_DIAGNOSTIC_REQUESTED) {
        LoggerManager::info("Entering diagnostic mode");
        fsm.transitionToState(STATE_DIAGNOSTIC);
      }
      else if (event == EVT_SYSTEM_ERROR) {
        LoggerManager::error("Error detected in READY state");
        fsm.transitionToState(STATE_ERROR);
      }
      else if (event == EVT_HOUR_CHANGED) {
        LoggerManager::info("Hour changed - updating counts");
        handleHourBoundary();
      }
      break;
    
    // ====================================================================
    // PRODUCTION STATE TRANSITIONS
    // ====================================================================
    case STATE_PRODUCTION:
      if (event == EVT_PRODUCTION_STOP) {
        if (canStopProduction()) {
          LoggerManager::info("Stopping production session");
          ProductionManager::getInstance().stopSession();
          fsm.transitionToState(STATE_READY);
        }
      }
      else if (event == EVT_ITEM_COUNTED) {
        handleItemCounted();
      }
      else if (event == EVT_HOUR_CHANGED) {
        LoggerManager::info("Hour changed during production");
        handleHourBoundary();
      }
      else if (event == EVT_SYSTEM_ERROR) {
        LoggerManager::error("Error detected during production");
        // Save progress before error
        saveProductionProgress();
        fsm.transitionToState(STATE_ERROR);
      }
      else if (event == EVT_DIAGNOSTIC_REQUESTED) {
        LoggerManager::info("Diagnostic requested - pausing production");
        // Save session state
        saveProductionProgress();
        fsm.transitionToState(STATE_DIAGNOSTIC);
      }
      break;
    
    // ====================================================================
    // DIAGNOSTIC STATE TRANSITIONS
    // ====================================================================
    case STATE_DIAGNOSTIC:
      if (event == EVT_DIAG_COMPLETE) {
        LoggerManager::info("Diagnostics complete");
        fsm.transitionToState(STATE_READY);
      }
      else if (event == EVT_SYSTEM_ERROR) {
        LoggerManager::error("Error during diagnostics");
        fsm.transitionToState(STATE_ERROR);
      }
      break;
    
    // ====================================================================
    // ERROR STATE TRANSITIONS
    // ====================================================================
    case STATE_ERROR:
      if (event == EVT_ERROR_RECOVERED) {
        LoggerManager::info("Error recovered - returning to READY");
        fsm.transitionToState(STATE_READY);
      }
      else if (event == EVT_PRODUCTION_STOP) {
        // User might press stop button even in error state
        LoggerManager::info("Stop pressed in error state - stopping session");
        ProductionManager::getInstance().stopSession();
      }
      // Stay in error state until recovery or reboot
      break;
    
    default:
      LoggerManager::error("Unknown state in event processing: %d", currentState);
      break;
  }
}

// ============================================================================
// HOUSEKEEPING & MAINTENANCE
// ============================================================================

/**
 * Periodic housekeeping tasks
 * 
 * Called once per loop iteration for maintenance tasks that don't
 * belong in specific state handlers.
 */
void handleHousekeeping() {
  // Blink status LED (if you want visual feedback)
  blinkStatusLED();
  
  // Yield to other tasks on dual-core ESP32
  delay(1);
}

/**
 * Blink status LED based on current state
 * 
 * This provides visual feedback about the system state:
 * - INITIALIZATION: Fast blink (200ms on/off)
 * - READY: Slow blink (1s on, 4s off)
 * - PRODUCTION: Continuous on
 * - DIAGNOSTIC: Double blink (100ms on/off, 500ms pause)
 * - ERROR: Triple blink (100ms on/off, 1s pause)
 */
void blinkStatusLED() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  unsigned long now = millis();
  
  SystemState state = fsm.getCurrentState();
  unsigned long blinkInterval = 0;
  
  switch (state) {
    case STATE_INITIALIZATION:
      blinkInterval = 200;  // Fast blink
      break;
    case STATE_READY:
      blinkInterval = 1000;  // Slow blink
      break;
    case STATE_PRODUCTION:
      // LED continuously on during production
      GPIO::write(2, HIGH);
      return;
    case STATE_DIAGNOSTIC:
      blinkInterval = 100;  // Double blink pattern
      break;
    case STATE_ERROR:
      blinkInterval = 100;  // Triple blink pattern
      break;
  }
  
  if (now - lastBlink >= blinkInterval) {
    ledState = !ledState;
    GPIO::write(2, ledState ? HIGH : LOW);  // GPIO 2 is typically LED on ESP32
    lastBlink = now;
  }
}

// ============================================================================
// STANDALONE HELPER FUNCTIONS
// ============================================================================

/**
 * Debug function to print current system state
 * 
 * Useful for troubleshooting - can be called from serial commands
 */
void printSystemStatus() {
  SystemState state = fsm.getCurrentState();
  
  Serial.println("=== SYSTEM STATUS ===");
  Serial.print("State: ");
  printStateName(state);
  Serial.println();
  
  Serial.print("Free Heap: ");
  Serial.print(PowerManager::getFreeHeap());
  Serial.println(" bytes");
  
  Serial.print("Chip Temp: ");
  Serial.print(PowerManager::getChipTemperature());
  Serial.println("°C");
  
  if (TimeManager::getInstance().isTimeValid()) {
    Serial.print("Time: ");
    Serial.println(TimeManager::getInstance().getTimeString().c_str());
  } else {
    Serial.println("Time: INVALID");
  }
  
  Serial.print("Session Active: ");
  Serial.println(ProductionManager::getInstance().isSessionActive() ? "YES" : "NO");
  
  Serial.print("Total Count: ");
  Serial.println(ProductionManager::getInstance().getTotalSessionCount());
  
  Serial.print("Event Queue Size: ");
  Serial.println(fsm.getEventQueueSize());
  
  Serial.println("====================");
}

/**
 * Print human-readable state name
 */
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

/**
 * Debug function to process serial commands
 * 
 * Useful for testing without hardware buttons
 */
void handleSerialCommand(String command) {
  command.trim();
  command.toUpperCase();
  
  if (command == "STATUS") {
    printSystemStatus();
  }
  else if (command == "START") {
    fsm.queueEvent(EVT_PRODUCTION_START);
    Serial.println(">> Production start requested");
  }
  else if (command == "STOP") {
    fsm.queueEvent(EVT_PRODUCTION_STOP);
    Serial.println(">> Production stop requested");
  }
  else if (command == "COUNT") {
    fsm.queueEvent(EVT_ITEM_COUNTED);
    Serial.println(">> Item count incremented");
  }
  else if (command == "DIAG") {
    fsm.queueEvent(EVT_DIAGNOSTIC_REQUESTED);
    Serial.println(">> Diagnostic mode requested");
  }
  else if (command == "RESET") {
    fsm.transitionToState(STATE_INITIALIZATION);
    Serial.println(">> System reset to INITIALIZATION");
  }
  else if (command == "ERROR") {
    fsm.transitionToState(STATE_ERROR);
    Serial.println(">> Entering ERROR state");
  }
  else if (command == "HELP") {
    Serial.println("Available commands:");
    Serial.println("  STATUS - Print system status");
    Serial.println("  START  - Start production");
    Serial.println("  STOP   - Stop production");
    Serial.println("  COUNT  - Increment count");
    Serial.println("  DIAG   - Enter diagnostic mode");
    Serial.println("  RESET  - Reset to initialization");
    Serial.println("  ERROR  - Enter error state");
    Serial.println("  HELP   - Show this help");
  }
  else {
    Serial.println("Unknown command. Type 'HELP' for available commands.");
  }
}

/**
 * Serial event handler - process incoming serial commands
 * 
 * Add this to your setup(): serialEventRun();
 * Or override serialEvent() in your sketch if not using Arduino's default
 */
void serialEvent() {
  while (Serial.available()) {
    static String inputBuffer = "";
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        Serial.print(">> ");
        Serial.println(inputBuffer);
        handleSerialCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}

// ============================================================================
// ERROR HANDLING & RECOVERY
// ============================================================================

/**
 * Global exception handler
 * 
 * Catches fatal errors and gracefully enters error state
 * This is called if any manager or handler throws an exception
 */
void handleFatalError(const char* message) {
  LoggerManager::fatal("FATAL ERROR: %s", message);
  
  // Try to log to SD card
  StorageManager::getInstance().writeFile("/error_log.txt", message);
  
  // Enter error state
  fsm.transitionToState(STATE_ERROR);
  
  // Display error on OLED
  DisplayManager::getInstance().showErrorScreen(message);
  
  // Loop indefinitely until watchdog resets us
  while (true) {
    Watchdog::feed();
    delay(100);
  }
}

// ============================================================================
// INTEGRATION NOTES
// ============================================================================

/**
 * IMPORTANT: Before using this code:
 * 
 * 1. Include all necessary headers in your .ino file:
 *    #include "state_manager.h"
 *    #include "state_handlers.h"
 *    #include "managers.h"
 *    #include "hal.h"
 * 
 * 2. Ensure all source files are in your Arduino sketch folder:
 *    - state_manager.h/cpp
 *    - state_handlers.h/cpp
 *    - managers.h/cpp
 *    - hal.h/cpp
 * 
 * 3. Configure pin numbers if different from hardware:
 *    - Counter button: GPIO 15
 *    - Diagnostic button: GPIO 27
 *    - Production latch: GPIO 25
 *    - SD chip select: GPIO 26
 *    - Status LED: GPIO 2
 * 
 * 4. Adjust logging level in setup():
 *    LoggerManager::initialize(LoggerManager::DEBUG);  // For verbose output
 *    LoggerManager::initialize(LoggerManager::INFO);   // For normal output
 *    LoggerManager::initialize(LoggerManager::WARN);   // For warnings only
 * 
 * 5. Add serial event handling if using serial commands:
 *    In setup(): enable serialEvent with serialEventRun();
 * 
 * 6. Test incrementally:
 *    - Start with INITIALIZATION state
 *    - Verify all hardware initializes
 *    - Test button interrupts
 *    - Test state transitions
 *    - Test production counting
 * 
 * 7. Monitor serial output for debugging:
 *    Open serial monitor at 115200 baud
 *    Watch for [INFO], [WARN], [ERROR], [DEBUG] messages
 */
