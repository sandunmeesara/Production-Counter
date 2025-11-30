#include "state_manager.h"

// ========================================
// STATE MANAGER IMPLEMENTATION
// ========================================

StateManager::StateManager() {
  memset(eventQueue, 0, sizeof(eventQueue));
}

bool StateManager::initialize() {
  currentState = SystemState::INITIALIZATION;
  previousState = SystemState::INITIALIZATION;
  productionSubState = ProductionState::IDLE;
  timeSubState = TimeState::UNSYNCHRONIZED;
  stateChangeTime = millis();
  
  Serial.println("[FSM] StateManager initialized");
  return true;
}

void StateManager::queueEvent(SystemEvent event) {
  uint8_t nextTail = (eventQueueTail + 1) % EVENT_QUEUE_SIZE;
  
  if (nextTail == eventQueueHead) {
    Serial.print("[FSM] WARNING: Event queue full, dropping event: ");
    Serial.println(getEventName(event));
    return;
  }
  
  eventQueue[eventQueueTail] = event;
  eventQueueTail = nextTail;
}

bool StateManager::hasQueuedEvents() const {
  return eventQueueHead != eventQueueTail;
}

void StateManager::processEvent(SystemEvent event) {
  eventCounter++;
  lastEventTime = millis();
  
  // Log event processing
  StateLogger::logEvent(event, true);
  
  // Route event to current state handler
  switch (currentState) {
    case SystemState::INITIALIZATION:
      handleEventInInitialization(event);
      break;
    case SystemState::READY:
      handleEventInReady(event);
      break;
    case SystemState::PRODUCTION:
      handleEventInProduction(event);
      break;
    case SystemState::DIAGNOSTIC:
      handleEventInDiagnostic(event);
      break;
    case SystemState::ERROR:
      handleEventInError(event);
      break;
  }
}

void StateManager::update() {
  // Process all queued events
  while (hasQueuedEvents()) {
    SystemEvent event = eventQueue[eventQueueHead];
    eventQueueHead = (eventQueueHead + 1) % EVENT_QUEUE_SIZE;
    processEvent(event);
  }
  
  // Update sub-states based on current state
  switch (currentState) {
    case SystemState::INITIALIZATION:
      // Check for initialization timeout
      if (getTimeInCurrentState() > 30000) {  // 30 second timeout
        transitionTo(SystemState::ERROR);
        Serial.println("[FSM] ERROR: Initialization timeout");
      }
      break;
      
    case SystemState::READY:
      // Ready state maintenance
      break;
      
    case SystemState::PRODUCTION:
      // Production state updates handled by event handlers
      break;
      
    case SystemState::DIAGNOSTIC:
      // Diagnostic mode timeout
      if (getTimeInCurrentState() > 60000) {  // 60 second timeout
        transitionTo(SystemState::READY);
        Serial.println("[FSM] Diagnostic timeout, returning to READY");
      }
      break;
      
    case SystemState::ERROR:
      // Error recovery check
      if (getTimeInCurrentState() > 5000) {  // 5 second error display
        transitionTo(SystemState::READY);
        Serial.println("[FSM] Auto-recovery from ERROR state");
      }
      break;
  }
}

bool StateManager::transitionTo(SystemState newState) {
  if (!canTransitionTo(newState)) {
    StateLogger::logTransitionGuard(newState, false);
    return false;
  }
  
  // Exit current state
  switch (currentState) {
    case SystemState::INITIALIZATION: exitInitialization(); break;
    case SystemState::READY: exitReady(); break;
    case SystemState::PRODUCTION: exitProduction(); break;
    case SystemState::DIAGNOSTIC: exitDiagnostic(); break;
    case SystemState::ERROR: exitError(); break;
  }
  
  previousState = currentState;
  currentState = newState;
  stateChangeTime = millis();
  transitionCounter++;
  
  // Enter new state
  switch (currentState) {
    case SystemState::INITIALIZATION: enterInitialization(); break;
    case SystemState::READY: enterReady(); break;
    case SystemState::PRODUCTION: enterProduction(); break;
    case SystemState::DIAGNOSTIC: enterDiagnostic(); break;
    case SystemState::ERROR: enterError(); break;
  }
  
  StateLogger::logStateChange(previousState, currentState);
  return true;
}

bool StateManager::canTransitionTo(SystemState newState) const {
  // Implement transition guards
  switch (newState) {
    case SystemState::READY:
      return canEnterReady();
    case SystemState::PRODUCTION:
      return canStartProduction();
    case SystemState::DIAGNOSTIC:
      return canEnterDiagnostic();
    case SystemState::ERROR:
      return true;  // Can always enter error state
    case SystemState::INITIALIZATION:
      return currentState == SystemState::INITIALIZATION;  // Only at startup
  }
  return false;
}

unsigned long StateManager::getTimeInCurrentState() const {
  return millis() - stateChangeTime;
}

unsigned long StateManager::getLastStateChangeTime() const {
  return stateChangeTime;
}

const char* StateManager::getCurrentStateName() const {
  switch (currentState) {
    case SystemState::INITIALIZATION: return "INITIALIZATION";
    case SystemState::READY: return "READY";
    case SystemState::PRODUCTION: return "PRODUCTION";
    case SystemState::DIAGNOSTIC: return "DIAGNOSTIC";
    case SystemState::ERROR: return "ERROR";
  }
  return "UNKNOWN";
}

const char* StateManager::getEventName(SystemEvent event) const {
  switch (event) {
    case SystemEvent::EVT_STARTUP_BEGIN: return "STARTUP_BEGIN";
    case SystemEvent::EVT_STARTUP_COMPLETE: return "STARTUP_COMPLETE";
    case SystemEvent::EVT_STARTUP_FAILED: return "STARTUP_FAILED";
    case SystemEvent::EVT_PRODUCTION_START: return "PRODUCTION_START";
    case SystemEvent::EVT_PRODUCTION_STOP: return "PRODUCTION_STOP";
    case SystemEvent::EVT_COUNTER_PRESSED: return "COUNTER_PRESSED";
    case SystemEvent::EVT_COUNT_UPDATED: return "COUNT_UPDATED";
    case SystemEvent::EVT_COUNT_SAVED: return "COUNT_SAVED";
    case SystemEvent::EVT_TIME_UPDATED: return "TIME_UPDATED";
    case SystemEvent::EVT_HOUR_CHANGED: return "HOUR_CHANGED";
    case SystemEvent::EVT_HOUR_LOGGED: return "HOUR_LOGGED";
    case SystemEvent::EVT_RTC_AVAILABLE: return "RTC_AVAILABLE";
    case SystemEvent::EVT_RTC_UNAVAILABLE: return "RTC_UNAVAILABLE";
    case SystemEvent::EVT_SD_AVAILABLE: return "SD_AVAILABLE";
    case SystemEvent::EVT_SD_UNAVAILABLE: return "SD_UNAVAILABLE";
    case SystemEvent::EVT_OLED_AVAILABLE: return "OLED_AVAILABLE";
    case SystemEvent::EVT_OLED_UNAVAILABLE: return "OLED_UNAVAILABLE";
    case SystemEvent::EVT_DIAGNOSTIC_REQUEST: return "DIAGNOSTIC_REQUEST";
    case SystemEvent::EVT_DIAGNOSTIC_COMPLETE: return "DIAGNOSTIC_COMPLETE";
    case SystemEvent::EVT_SERIAL_COMMAND: return "SERIAL_COMMAND";
    case SystemEvent::EVT_SERIAL_TIME_SET: return "SERIAL_TIME_SET";
    case SystemEvent::EVT_ENTER_STATE: return "ENTER_STATE";
    case SystemEvent::EVT_EXIT_STATE: return "EXIT_STATE";
    case SystemEvent::EVT_STATE_TIMEOUT: return "STATE_TIMEOUT";
    case SystemEvent::EVT_ERROR_DETECTED: return "ERROR_DETECTED";
    case SystemEvent::EVT_ERROR_RECOVERED: return "ERROR_RECOVERED";
    case SystemEvent::EVT_ERROR_FATAL: return "ERROR_FATAL";
  }
  return "UNKNOWN_EVENT";
}

// ========================================
// STATE HANDLER IMPLEMENTATIONS
// ========================================

void StateManager::handleEventInInitialization(SystemEvent event) {
  switch (event) {
    case SystemEvent::EVT_RTC_AVAILABLE:
      timeSubState = TimeState::SYNCHRONIZED;
      break;
    case SystemEvent::EVT_SD_AVAILABLE:
      break;
    case SystemEvent::EVT_STARTUP_COMPLETE:
      transitionTo(SystemState::READY);
      break;
    case SystemEvent::EVT_STARTUP_FAILED:
      transitionTo(SystemState::ERROR);
      break;
    default:
      break;
  }
}

void StateManager::handleEventInReady(SystemEvent event) {
  switch (event) {
    case SystemEvent::EVT_PRODUCTION_START:
      transitionTo(SystemState::PRODUCTION);
      break;
    case SystemEvent::EVT_DIAGNOSTIC_REQUEST:
      transitionTo(SystemState::DIAGNOSTIC);
      break;
    case SystemEvent::EVT_HOUR_CHANGED:
      // Handle in-place, stay in READY
      break;
    case SystemEvent::EVT_ERROR_DETECTED:
      transitionTo(SystemState::ERROR);
      break;
    default:
      break;
  }
}

void StateManager::handleEventInProduction(SystemEvent event) {
  switch (event) {
    case SystemEvent::EVT_COUNTER_PRESSED:
      productionSubState = ProductionState::ACTIVE;
      break;
    case SystemEvent::EVT_PRODUCTION_STOP:
      transitionTo(SystemState::READY);
      break;
    case SystemEvent::EVT_HOUR_CHANGED:
      // Handle during production (stay in PRODUCTION)
      break;
    case SystemEvent::EVT_ERROR_DETECTED:
      transitionTo(SystemState::ERROR);
      break;
    default:
      break;
  }
}

void StateManager::handleEventInDiagnostic(SystemEvent event) {
  switch (event) {
    case SystemEvent::EVT_DIAGNOSTIC_COMPLETE:
      transitionTo(SystemState::READY);
      break;
    case SystemEvent::EVT_ERROR_DETECTED:
      transitionTo(SystemState::ERROR);
      break;
    default:
      break;
  }
}

void StateManager::handleEventInError(SystemEvent event) {
  switch (event) {
    case SystemEvent::EVT_ERROR_RECOVERED:
      transitionTo(SystemState::READY);
      break;
    case SystemEvent::EVT_ERROR_FATAL:
      // Stay in error state, system must be rebooted
      break;
    default:
      break;
  }
}

// ========================================
// STATE ENTRY/EXIT ACTIONS
// ========================================

void StateManager::enterInitialization() {
  Serial.println("[FSM] >>> Entering INITIALIZATION state");
}

void StateManager::exitInitialization() {
  Serial.println("[FSM] <<< Exiting INITIALIZATION state");
}

void StateManager::enterReady() {
  Serial.println("[FSM] >>> Entering READY state");
  productionSubState = ProductionState::IDLE;
}

void StateManager::exitReady() {
  Serial.println("[FSM] <<< Exiting READY state");
}

void StateManager::enterProduction() {
  Serial.println("[FSM] >>> Entering PRODUCTION state");
  productionSubState = ProductionState::ACTIVE;
}

void StateManager::exitProduction() {
  Serial.println("[FSM] <<< Exiting PRODUCTION state");
  productionSubState = ProductionState::IDLE;
}

void StateManager::enterDiagnostic() {
  Serial.println("[FSM] >>> Entering DIAGNOSTIC state");
}

void StateManager::exitDiagnostic() {
  Serial.println("[FSM] <<< Exiting DIAGNOSTIC state");
}

void StateManager::enterError() {
  Serial.println("[FSM] >>> Entering ERROR state");
}

void StateManager::exitError() {
  Serial.println("[FSM] <<< Exiting ERROR state");
}

// ========================================
// GUARD CONDITION IMPLEMENTATIONS
// ========================================

bool StateManager::canEnterReady() const {
  // All hardware must be initialized
  return GuardConditions::isOLEDAvailable();
}

bool StateManager::canStartProduction() const {
  // Current state must be READY
  return currentState == SystemState::READY && 
         GuardConditions::canStartProduction();
}

bool StateManager::canStopProduction() const {
  // Only can stop if currently in PRODUCTION
  return currentState == SystemState::PRODUCTION;
}

bool StateManager::canEnterDiagnostic() const {
  // Can enter diagnostic from READY state only
  return currentState == SystemState::READY;
}

// ========================================
// GUARD CONDITIONS IMPLEMENTATIONS
// ========================================

// Extern variables from main code (will be linked)
extern bool rtcAvailable;
extern bool sdAvailable;

bool GuardConditions::isRTCAvailable() {
  return rtcAvailable;
}

bool GuardConditions::isSDAvailable() {
  return sdAvailable;
}

bool GuardConditions::isOLEDAvailable() {
  // OLED availability check would be implemented
  return true;  // Assume OLED is always available if we reach this point
}

bool GuardConditions::canStartProduction() {
  // Can start if OLED is available
  return isOLEDAvailable();
}

bool GuardConditions::canStopProduction() {
  return true;  // Can always stop production
}

bool GuardConditions::isCountValidRange(int count) {
  return count >= 0 && count <= 9999;
}

bool GuardConditions::isTimeValid() {
  return isRTCAvailable();
}

bool GuardConditions::isNewHour(int currentHour, int lastHour) {
  return currentHour != lastHour && currentHour >= 0 && currentHour < 24;
}

bool GuardConditions::isHeapHealthy() {
  uint32_t freeHeap = ESP.getFreeHeap();
  return freeHeap > 50000;  // At least 50KB free
}

bool GuardConditions::isStackHealthy() {
  uint32_t stackSpace = uxTaskGetStackHighWaterMark(NULL);
  return stackSpace > 1024;  // At least 1KB of stack
}

bool GuardConditions::hasFreeDiskSpace() {
  return sdAvailable;  // Simplified check
}

bool GuardConditions::canRecoverFromPowerLoss() {
  return sdAvailable;
}

bool GuardConditions::isSessionRecoveryValid() {
  return canRecoverFromPowerLoss();
}

// ========================================
// STATE LOGGER IMPLEMENTATIONS
// ========================================

void StateLogger::logStateChange(SystemState from, SystemState to) {
  Serial.print("[FSM] State transition: ");
  
  // Print from state
  switch (from) {
    case SystemState::INITIALIZATION: Serial.print("INITIALIZATION"); break;
    case SystemState::READY: Serial.print("READY"); break;
    case SystemState::PRODUCTION: Serial.print("PRODUCTION"); break;
    case SystemState::DIAGNOSTIC: Serial.print("DIAGNOSTIC"); break;
    case SystemState::ERROR: Serial.print("ERROR"); break;
  }
  
  Serial.print(" → ");
  
  // Print to state
  switch (to) {
    case SystemState::INITIALIZATION: Serial.print("INITIALIZATION"); break;
    case SystemState::READY: Serial.print("READY"); break;
    case SystemState::PRODUCTION: Serial.print("PRODUCTION"); break;
    case SystemState::DIAGNOSTIC: Serial.print("DIAGNOSTIC"); break;
    case SystemState::ERROR: Serial.print("ERROR"); break;
  }
  
  Serial.println();
}

void StateLogger::logEvent(SystemEvent event, bool processed) {
  Serial.print("[FSM] Event: ");
  
  // Event names (abbreviated for log)
  switch (event) {
    case SystemEvent::EVT_PRODUCTION_START: Serial.print("START"); break;
    case SystemEvent::EVT_PRODUCTION_STOP: Serial.print("STOP"); break;
    case SystemEvent::EVT_COUNTER_PRESSED: Serial.print("COUNT"); break;
    case SystemEvent::EVT_HOUR_CHANGED: Serial.print("HOUR"); break;
    default: Serial.print("?"); break;
  }
  
  Serial.print(processed ? " [✓]" : " [✗]");
  Serial.println();
}

void StateLogger::logTransitionGuard(SystemState target, bool result) {
  Serial.print("[FSM] Guard check for ");
  
  switch (target) {
    case SystemState::INITIALIZATION: Serial.print("INITIALIZATION"); break;
    case SystemState::READY: Serial.print("READY"); break;
    case SystemState::PRODUCTION: Serial.print("PRODUCTION"); break;
    case SystemState::DIAGNOSTIC: Serial.print("DIAGNOSTIC"); break;
    case SystemState::ERROR: Serial.print("ERROR"); break;
  }
  
  Serial.print(": ");
  Serial.println(result ? "PASS" : "FAIL");
}

void StateLogger::logError(const char* message) {
  Serial.print("[FSM] ERROR: ");
  Serial.println(message);
}
