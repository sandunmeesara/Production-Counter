# FSM Architecture - Developer Quick Start

**Last Updated**: November 28, 2025  
**Target Developers**: Embedded Systems Engineers  
**Skill Level Required**: Intermediate-Advanced C++

---

## What is an FSM?

A **Finite State Machine** is a design pattern where:
- System exists in one of several **discrete states**
- System transitions between states based on **events**
- Each state has well-defined **entry/exit actions**
- Transitions are guarded by **conditions**

**Real-world analogy**: Traffic light
```
States: RED, YELLOW, GREEN
Events: Timer expired
Transitions: RED → GREEN → YELLOW → RED
Guards: Can't skip GREEN, must follow sequence
```

---

## Your System's FSM

```
┌─────────────────┐
│ INITIALIZATION  │ (Startup, detect hardware)
└────────┬────────┘
         │ EVT_STARTUP_COMPLETE
         ↓
    ┌────────────┐
    │   READY    │ (Waiting for production)
    └──┬──────┬──┘
       │ │    │
    ───┘ │    └─── EVT_DIAGNOSTIC_REQUEST
   │    │         │
   │    │ EVT_PRODUCTION_START
   │    │         │
   │    ↓         ↓
   │  ┌──────────────┐        ┌───────────┐
   │  │  PRODUCTION  │        │ DIAGNOSTIC│
   │  │  (Counting)  │        │ (Testing) │
   │  └──┬───────────┘        └─┬─────────┘
   │     │ EVT_PRODUCTION_STOP   │ EVT_DIAGNOSTIC_COMPLETE
   │     └────────────┬──────────┘
   │                  ↓
   └──────→ (return to READY)
   
   ANY STATE + EVT_ERROR_DETECTED → ERROR state
```

---

## File Structure

```
code_v3.cpp              ← ORIGINAL (keep for reference)

NEW FILES CREATED:
├── state_manager.h      ← StateManager, Events, Guards
├── state_manager.cpp    ← FSM implementation
├── managers.h           ← Manager class interfaces
├── hal.h                ← Hardware abstraction
├── fsm_main_template.cpp ← Reference loop structure
├── FSM_ARCHITECTURE.md  ← Design specification
└── FSM_IMPLEMENTATION_GUIDE.md ← Detailed roadmap
```

---

## State Machine Lifecycle

### 1. Initialization
```
setup() {
  StateManager sm;
  sm.initialize();  // Enter INITIALIZATION state
  
  // Try to initialize hardware...
  
  sm.queueEvent(EVT_STARTUP_COMPLETE);  // Queue transition
  sm.update();  // Process event → READY state
}
```

### 2. Normal Operation
```
loop() {
  // 1. Collect what happened since last loop
  if (buttonPressed) sm.queueEvent(EVT_PRODUCTION_START);
  
  // 2. Process all events and state transitions
  sm.update();  // Handles state machine logic
  
  // 3. Do state-specific work
  switch (sm.getCurrentState()) {
    case READY: // Wait for production
      break;
    case PRODUCTION: // Count items
      storeCount();
      break;
  }
  
  // 4. Update display
  display.show(sm.getCurrentState());
}
```

### 3. Transition Example
```
READY state + EVT_PRODUCTION_START event:

1. Guard check: canStartProduction() → Must pass
2. If guard fails: Stay in READY, ignore event
3. If guard passes:
   a. exitReady() action (cleanup)
   b. currentState = PRODUCTION
   c. enterProduction() action (setup)
   d. Log: "READY → PRODUCTION"
```

---

## Key Concepts

### Events (Messages)
Events represent "something happened":
```cpp
enum SystemEvent {
  EVT_PRODUCTION_START,      // User pressed button
  EVT_COUNTER_PRESSED,       // Item was counted
  EVT_HOUR_CHANGED,          // Hour boundary crossed
  EVT_ERROR_DETECTED,        // Problem occurred
};
```

**Think of events as messages in a queue:**
```
Time T=0:    EVT_PRODUCTION_START ← Button pressed
Time T=10ms: EVT_COUNTER_PRESSED ← User counted 1 item
             EVT_COUNTER_PRESSED ← User counted 1 item
Time T=20ms: Loop processes queue, executes state logic
```

### Guard Conditions (Safety Checks)
Guards prevent invalid transitions:
```cpp
bool canStartProduction() {
  // Only allow if:
  return isOLEDAvailable()     // Display must work
    && currentState == READY    // Must not already running
    && currentCount < 9999      // Not at max
    && !productionActive;       // No duplicate start
}
```

### Entry/Exit Actions (Housekeeping)
Automatically run when entering/exiting states:
```cpp
void enterProduction() {
  // Setup phase
  sessionStartTime = now();
  sessionCount = 0;
  saveRecoveryFile();
  display.showProductionScreen();
}

void exitProduction() {
  // Cleanup phase
  saveSessionToFile();
  clearRecoveryFile();
  writeStatistics();
}
```

---

## Common Patterns

### Pattern 1: Button Press → State Change
```cpp
// In collectEvents()
if (digitalRead(LATCH_BUTTON) == LOW) {
  delay(50);  // Debounce
  stateManager.queueEvent(EVT_PRODUCTION_START);
}

// In state machine
if (event == EVT_PRODUCTION_START) {
  if (canStartProduction()) {  // Guard
    transitionTo(PRODUCTION);   // Move to new state
  }
}

// In executeProductionState()
// Count items here
```

### Pattern 2: Time-based Event
```cpp
// In collectEvents()
static unsigned long lastCheck = 0;
if (millis() - lastCheck > 1000) {  // Every second
  if (timeManager.hasHourChanged()) {
    stateManager.queueEvent(EVT_HOUR_CHANGED);
  }
  lastCheck = millis();
}

// In state machine
if (event == EVT_HOUR_CHANGED) {
  timeManager.handleHourChange();  // Update counts
  // Stay in current state (READY or PRODUCTION)
}
```

### Pattern 3: Error Detection → Error State
```cpp
// In housekeeping()
if (!GuardConditions::isHeapHealthy()) {
  stateManager.queueEvent(EVT_ERROR_DETECTED);
}

// In state machine (any state)
if (event == EVT_ERROR_DETECTED) {
  transitionTo(ERROR);  // Go to error state
}

// In executeErrorState()
display.showErrorScreen("Heap Low");
// Optionally transition back to READY if recovered
```

---

## Debugging Tips

### 1. Enable FSM Logging
```cpp
// state_manager.cpp logs all transitions
// Serial output:
[FSM] State transition: READY → PRODUCTION
[FSM] Event: COUNT [✓]
[FSM] Guard check for PRODUCTION: PASS
```

### 2. Check Current State
```cpp
Serial.print("Current state: ");
Serial.println(stateManager.getCurrentStateName());
// Output: "PRODUCTION"
```

### 3. Count Events
```cpp
Serial.print("Events processed: ");
Serial.println(stateManager.getEventCount());
// Shows: 127 events since startup
```

### 4. Check Event Queue
```cpp
// If events are being lost:
if (stateManager.hasQueuedEvents()) {
  Serial.println("Events waiting in queue");
}
// If queue is full, oldest events are dropped
```

---

## Comparing Old vs New

### OLD: code_v3.cpp
```cpp
void loop() {
  // 100+ lines of code
  if (diagnosticRequested) { /* 20 lines */ }
  if (Serial.available()) { /* 30 lines */ }
  if (currentMode == MODE_NORMAL) {
    if (productionStatusChanged) { /* 15 lines */ }
    if (rtcAvailable && rtcNow.hour() != lastHour) { /* 5 lines */ }
    if (countChanged && sdAvailable) { /* 10 lines */ }
    if (countChanged || needsFullRedraw) { /* 5 lines */ }
    if (currentStatus != STATUS_IDLE) { /* 3 lines */ }
  }
  delay(10);
}
// Problems:
// - Hard to understand flow
// - Difficult to add features
// - Hard to test
// - Global state scattered
```

### NEW: FSM-based
```cpp
void loop() {
  collectEvents();              // 30 lines - clear purpose
  stateManager.update();        // Handles transitions
  executeCurrentState();        // 30 lines - state-specific
  displayManager.update();      // Handled separately
  housekeeping();               // 15 lines - clear purpose
  delay(10);
}

// Benefits:
// - Clear 5-phase structure
// - Easy to understand flow
// - Simple to add new states
// - Testable components
// - Modular and reusable
```

---

## Your Next Steps

### Immediate (Before coding)
1. Read this document completely
2. Study `FSM_ARCHITECTURE.md` 
3. Review `state_manager.h` and `state_manager.cpp`
4. Understand the 5-phase loop structure

### Short Term (Week 1)
1. Implement `ProductionManager` class
2. Implement `TimeManager` class
3. Write unit tests for both

### Medium Term (Week 2-3)
1. Implement `StorageManager` and `DisplayManager`
2. Create HAL implementations (GPIO, Serial_HAL, etc.)
3. Write integration tests

### Long Term (Week 4-5)
1. Replace main loop with FSM version
2. Migrate all state logic
3. Comprehensive testing and validation

---

## Important Rules

### ✓ DO:
- Queue events from ISRs, don't change state
- Use guard conditions before transitions
- Keep ISRs minimal and fast
- Log state changes during debugging
- Test state transitions independently
- Use meaningful event names

### ✗ DON'T:
- Directly modify state outside transitionTo()
- Skip guard condition checks
- Do heavy work in ISRs
- Ignore event queue overflow
- Hardcode state values
- Mix old global state with FSM state

---

## Questions?

Common questions answered:

**Q: Why not just use if/else like code_v3.cpp?**  
A: FSMs scale better, are testable, and prevent invalid states.

**Q: What if event queue overflows?**  
A: Oldest event is dropped, logged as warning.

**Q: Can multiple events happen at once?**  
A: Yes, queue them all, they process in order next loop iteration.

**Q: How do I transition between states not in diagram?**  
A: Implement guard, add transition, log it, test it.

**Q: Will code_v3.cpp files still work?**  
A: Yes, file formats unchanged, commands same.

---

## Ready to Code?

1. **Start with**: `ProductionManager::startSession()`
2. **Follow**: FSM_IMPLEMENTATION_GUIDE.md Phase 2
3. **Reference**: fsm_main_template.cpp for patterns
4. **Test with**: State-specific tests first, then integration tests
5. **Validate**: All files and commands still work

Good luck! 🚀

