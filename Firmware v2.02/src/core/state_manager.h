#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include <cstring>

// ========================================
// SYSTEM STATE ENUM
// ========================================
enum class SystemState {
  INITIALIZATION,    // System starting up, initializing hardware
  READY,            // All systems initialized, waiting for production
  PRODUCTION,       // Currently counting production items
  DIAGNOSTIC,       // Diagnostic mode active
  ERROR             // System error detected
};

// ========================================
// PRODUCTION SESSION SUB-STATE
// ========================================
enum class ProductionState {
  IDLE,             // Not counting
  ACTIVE,           // Currently counting items
  SUSPENDED         // Paused (for multi-part sessions)
};

// ========================================
// TIME SYNCHRONIZATION STATE
// ========================================
enum class TimeState {
  UNSYNCHRONIZED,   // RTC not set or invalid
  SYNCHRONIZED,     // RTC is valid and updated
  HOUR_TRANSITION   // Currently in hour change handler
};

// ========================================
// EVENT TYPES
// ========================================
enum class SystemEvent {
  // Startup events
  EVT_STARTUP_BEGIN,
  EVT_STARTUP_COMPLETE,
  EVT_STARTUP_FAILED,
  
  // Production events
  EVT_PRODUCTION_START,      // Latch button pressed (held down)
  EVT_PRODUCTION_STOP,       // Latch button released
  EVT_COUNTER_PRESSED,       // Counter button pressed (during production)
  EVT_COUNT_UPDATED,         // Count value changed
  EVT_COUNT_SAVED,           // Count persisted to SD
  
  // Time events
  EVT_TIME_UPDATED,          // RTC time updated
  EVT_HOUR_CHANGED,          // Hour boundary crossed
  EVT_HOUR_LOGGED,           // Hour change processed and saved
  
  // Hardware events
  EVT_RTC_AVAILABLE,         // RTC chip detected and working
  EVT_RTC_UNAVAILABLE,       // RTC failed or not present
  EVT_SD_AVAILABLE,          // SD card detected and initialized
  EVT_SD_UNAVAILABLE,        // SD card disconnected or failed
  EVT_OLED_AVAILABLE,        // OLED display working
  EVT_OLED_UNAVAILABLE,      // OLED display failed
  
  // Diagnostic events
  EVT_DIAGNOSTIC_REQUEST,    // Diagnostic button pressed
  EVT_DIAGNOSTIC_COMPLETE,   // Diagnostic tests finished
  
  // Serial command events
  EVT_SERIAL_COMMAND,        // Command received via serial
  EVT_SERIAL_TIME_SET,       // Time set via serial
  
  // State machine events
  EVT_ENTER_STATE,
  EVT_EXIT_STATE,
  EVT_STATE_TIMEOUT,
  
  // Error events
  EVT_ERROR_DETECTED,
  EVT_ERROR_RECOVERED,
  EVT_ERROR_FATAL
};

// ========================================
// STATE MANAGER CLASS
// ========================================
class StateManager {
public:
  // Constructor
  StateManager();
  
  // State machine control
  bool initialize();
  void processEvent(SystemEvent event);
  void update();
  
  // State queries
  SystemState getCurrentState() const { return currentState; }
  SystemState getPreviousState() const { return previousState; }
  ProductionState getProductionState() const { return productionSubState; }
  TimeState getTimeState() const { return timeSubState; }
  
  // State transitions
  bool transitionTo(SystemState newState);
  bool canTransitionTo(SystemState newState) const;
  
  // Event management
  void queueEvent(SystemEvent event);
  bool hasQueuedEvents() const;
  
  // State-specific information
  const char* getCurrentStateName() const;
  const char* getEventName(SystemEvent event) const;
  
  // Timing
  unsigned long getTimeInCurrentState() const;
  unsigned long getLastStateChangeTime() const;
  
  // Statistics
  uint32_t getEventCount() const { return eventCounter; }
  uint32_t getTransitionCount() const { return transitionCounter; }
  
private:
  // Current states
  SystemState currentState = SystemState::INITIALIZATION;
  SystemState previousState = SystemState::INITIALIZATION;
  ProductionState productionSubState = ProductionState::IDLE;
  TimeState timeSubState = TimeState::UNSYNCHRONIZED;
  
  // Event queue (circular buffer)
  static const uint8_t EVENT_QUEUE_SIZE = 16;
  SystemEvent eventQueue[EVENT_QUEUE_SIZE];
  uint8_t eventQueueHead = 0;
  uint8_t eventQueueTail = 0;
  
  // Timing
  unsigned long stateChangeTime = 0;
  unsigned long lastEventTime = 0;
  
  // Statistics
  uint32_t eventCounter = 0;
  uint32_t transitionCounter = 0;
  
  // State machine logic
  void handleEventInInitialization(SystemEvent event);
  void handleEventInReady(SystemEvent event);
  void handleEventInProduction(SystemEvent event);
  void handleEventInDiagnostic(SystemEvent event);
  void handleEventInError(SystemEvent event);
  
  // Entry/exit actions
  void enterInitialization();
  void exitInitialization();
  void enterReady();
  void exitReady();
  void enterProduction();
  void exitProduction();
  void enterDiagnostic();
  void exitDiagnostic();
  void enterError();
  void exitError();
  
  // Guard conditions
  bool canEnterReady() const;
  bool canStartProduction() const;
  bool canStopProduction() const;
  bool canEnterDiagnostic() const;
};

// ========================================
// GUARD CONDITIONS CLASS
// ========================================
class GuardConditions {
public:
  // Hardware availability checks
  static bool isRTCAvailable();
  static bool isSDAvailable();
  static bool isOLEDAvailable();
  
  // Production checks
  static bool canStartProduction();
  static bool canStopProduction();
  static bool isCountValidRange(int count);
  
  // Time checks
  static bool isTimeValid();
  static bool isNewHour(int currentHour, int lastHour);
  
  // System checks
  static bool isHeapHealthy();
  static bool isStackHealthy();
  static bool hasFreeDiskSpace();
  
  // Recovery checks
  static bool canRecoverFromPowerLoss();
  static bool isSessionRecoveryValid();
};

// ========================================
// EVENT HANDLER INTERFACE
// ========================================
class EventHandler {
public:
  virtual ~EventHandler() = default;
  
  virtual void onStateEnter(SystemState state) = 0;
  virtual void onStateExit(SystemState state) = 0;
  virtual void onEvent(SystemEvent event) = 0;
  virtual void onTransition(SystemState from, SystemState to) = 0;
};

// ========================================
// LOGGER FOR STATE MACHINE
// ========================================
class StateLogger {
public:
  static void logStateChange(SystemState from, SystemState to);
  static void logEvent(SystemEvent event, bool processed);
  static void logTransitionGuard(SystemState target, bool result);
  static void logError(const char* message);
};

#endif // STATE_MANAGER_H
