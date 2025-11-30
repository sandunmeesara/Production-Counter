# FSM Refactoring Implementation Guide

**Status**: Phase 1 Complete ✓  
**Next Phase**: Manager Classes Implementation  
**Timeline**: 5 weeks total

---

## Phase 1: Foundation ✓ COMPLETE

### Deliverables Created:

1. **state_manager.h / state_manager.cpp** (Complete)
   - `SystemState` enum: INITIALIZATION, READY, PRODUCTION, DIAGNOSTIC, ERROR
   - `ProductionState` enum: IDLE, ACTIVE, SUSPENDED
   - `TimeState` enum: UNSYNCHRONIZED, SYNCHRONIZED, HOUR_TRANSITION
   - `SystemEvent` enum: 28 event types
   - `StateManager` class: State transitions, event queuing, guards
   - `GuardConditions` class: Hardware/system checks
   - `StateLogger` class: Diagnostic logging
   
   **Key Features:**
   - Event queue (16-item circular buffer)
   - Automatic state entry/exit actions
   - Transition guards for safety
   - State timing information
   - Event statistics/counters

2. **managers.h** (Interface only, implementation follows)
   - `ProductionManager`: Session control, counting, file management
   - `TimeManager`: Time operations, hour tracking, synchronization
   - `StorageManager`: File I/O, persistence, directory operations
   - `DisplayManager`: Screen updates, state-specific displays
   - `LoggerManager`: Multi-level logging system
   - `ConfigManager`: Settings persistence and validation
   
   **Architecture Pattern:**
   ```
   Manager Classes (Responsibilities)
   ├── ProductionManager
   │   ├── Session lifecycle (start/stop)
   │   ├── Counting logic
   │   └── Session persistence
   ├── TimeManager
   │   ├── RTC synchronization
   │   ├── Hour change detection
   │   └── Time validation
   ├── StorageManager
   │   ├── SD card I/O
   │   ├── File persistence
   │   └── Directory management
   └── DisplayManager
       ├── OLED updates
       ├── State-specific screens
       └── Refresh optimization
   ```

3. **hal.h** (Hardware Abstraction Layer)
   - `GPIO` class: Pin control, interrupt handling
   - `Timer` class: Periodic/oneshot timers
   - `Serial_HAL` class: UART abstraction
   - `I2C` class: I2C device communication
   - `SPI_HAL` class: SPI bus management
   - `Watchdog` class: System watchdog
   - `PowerManager` class: Power monitoring
   - `EEPROM_HAL` class: Persistent storage
   
   **Benefits:**
   - Decouples hardware from business logic
   - Easy to mock for testing
   - Simplifies hardware porting
   - Centralizes pin definitions

4. **fsm_main_template.cpp** (Reference Implementation)
   - Complete refactored main loop
   - 5-phase execution model
   - Event collection
   - State machine updates
   - State-specific execution
   - Display updates
   - Housekeeping tasks
   
   **Loop Structure:**
   ```
   loop() {
     collectEvents()       // Phase 1: Button press, serial, time
     stateManager.update() // Phase 2: Process events & transitions
     executeCurrentState() // Phase 3: Run state-specific logic
     displayManager.update() // Phase 4: Update screen
     housekeeping()        // Phase 5: Timers, watchdog, health
   }
   ```

---

## Phase 2: Manager Implementations (NEXT)

### Week 1-2: ProductionManager & TimeManager

#### ProductionManager Implementation
```cpp
class ProductionManager {
  // Core responsibility: Manage production session lifecycle
  
  // Key methods to implement:
  - startSession(): Initialize session, save start time, reset count
  - stopSession(): Save final count, generate session file, return statistics
  - incrementCount(): Add 1 to session count (from interrupt)
  - saveSessionToFile(): Create Production_*.txt with timestamps
  - recover(): Restore state from /prod_session.txt if power lost
};
```

**Key Data to Track:**
- `sessionActive`: Boolean flag
- `sessionCount`: Current session count (atomic int)
- `sessionStartTime`: DateTime of start
- `sessionStopTime`: DateTime of stop
- `totalSessionCount`: Lifetime count

**Integration Points:**
1. Start event → Create recovery file immediately
2. Count updates → Save periodically to recovery file
3. Stop event → Generate final session file with format: `Production_YYYY-MM-DD_HHhMMm-HHhMMm.txt`
4. Power loss → Load recovery file on startup

#### TimeManager Implementation
```cpp
class TimeManager {
  // Core responsibility: Manage RTC and time operations
  
  // Key methods to implement:
  - initialize(): Initialize RTC, sync if needed
  - getCurrentTime(): Return current DateTime
  - setTime(): Update RTC to new time (from serial command)
  - hasHourChanged(): Check if hour boundary crossed
  - handleHourChange(): Process hour transition
  - getTimeString(): Format time for display
};
```

**Hour Change Logic:**
```
If (currentHour != lastTrackedHour):
  1. Get count at start of previous hour
  2. Calculate hourly count = current - baseline
  3. Add to cumulative count
  4. Update cumulative_count.txt
  5. Reset count (if production not active)
  6. Update hourly_count.txt
  7. Reset baseline tracker
```

**Event Generation:**
- `EVT_RTC_AVAILABLE` on successful init
- `EVT_HOUR_CHANGED` when hour boundary crosses
- `EVT_TIME_UPDATED` when time manually set

### Week 2-3: StorageManager & DisplayManager

#### StorageManager Implementation
```cpp
class StorageManager {
  // Core responsibility: Manage all SD card operations
  
  // File operations:
  - writeFile(): Generic file write
  - readFile(): Generic file read
  - saveCount(): Write count value to specific file
  - loadCount(): Read count from file
  - saveProductionSession(): Create timestamped session file
  - saveDailyLog(): Create daily summary file
  
  // Directory operations:
  - listFiles(): Enumerate all files
  - searchFiles(): Find files by pattern
  - countFiles(): Get total file count
  - deleteFile(): Remove specific file
};
```

**File Format Responsibilities:**
1. `/count.txt` - Current session count
2. `/hourly_count.txt` - Last completed hour count
3. `/cumulative_count.txt` - Lifetime total
4. `/prod_session.txt` - Active session recovery
5. `Production_YYYY-MM-DD_HHhMMm-HHhMMm.txt` - Session files
6. `DailyProduction_YYYY-MM-DD.txt` - Daily summaries

**Integration with code_v3.cpp:**
- Move all file I/O logic from main code
- Replace direct SD calls with StorageManager calls
- Maintain exact same file formats

#### DisplayManager Implementation
```cpp
class DisplayManager {
  // Core responsibility: Manage OLED display updates
  
  // State-specific screens:
  - showInitializationScreen(): Startup message
  - showReadyScreen(): Idle/waiting state
  - showProductionScreen(): Active production with count
  - showDiagnosticScreen(): Test results
  - showErrorScreen(): Error message and recovery
  
  // Content control:
  - displayText(): Draw text at position
  - displayNumber(): Draw numeric value
  - displayCentered(): Center text on line
  - drawProgressBar(): Animated progress
};
```

**Display Optimization:**
- Track `displayDirty` flag to avoid full redraws
- Update only changed regions when possible
- Refresh rate: 100ms (10 updates/second)
- Cache calculations to reduce computation

---

## Phase 3: State Handler Functions (Week 3)

### State-Specific Execution Functions

```cpp
void executeReadyState() {
  // Waiting for production to start
  // Tasks:
  // 1. Monitor for production button
  // 2. Perform periodic saves
  // 3. Update display
  // 4. Check system health
}

void executeProductionState() {
  // Currently counting items
  // Tasks:
  // 1. Count incoming interrupts
  // 2. Save count periodically
  // 3. Update display with real-time count
  // 4. Handle hour changes (preserve count differential)
  // 5. Check for max count limit
}

void executeDiagnosticState() {
  // Running system diagnostics
  // Tasks:
  // 1. Test OLED (should already be working if we're here)
  // 2. Test RTC (check time is valid)
  // 3. Test SD card (list files, check space)
  // 4. Report memory usage
  // 5. Show FSM statistics
  // 6. Return to READY when complete
}

void executeErrorState() {
  // System error occurred
  // Tasks:
  // 1. Display error message
  // 2. Attempt recovery (check hardware)
  // 3. Auto-transition to READY if recovery successful
  // 4. Or stay in ERROR and wait for manual intervention
}
```

### Guard Condition Implementations

```cpp
bool GuardConditions::canStartProduction() {
  // Check: OLED available (minimum requirement)
  // Check: Not already in production
  // Check: Count not at max
  return isOLEDAvailable() && !productionActive && currentCount < maxCount;
}

bool GuardConditions::canStopProduction() {
  // Always allowed, but need to save session
  return true;
}

bool GuardConditions::isHeapHealthy() {
  // Check: At least 50KB free
  return ESP.getFreeHeap() > 50000;
}

bool GuardConditions::isTimeValid() {
  // Check: RTC is initialized and time seems reasonable
  // (year > 2020, hour 0-23, month 1-12, etc.)
  return rtcAvailable && ...;
}
```

---

## Phase 4: Main Loop Refactoring (Week 3-4)

### Step-by-Step Refactoring Plan

**Step 1: Create FSM-driven loop structure**
```cpp
void loop() {
  collectEvents();        // EVENT COLLECTION PHASE
  stateManager.update();  // STATE MACHINE PHASE
  executeCurrentState();  // STATE EXECUTION PHASE
  displayManager.update(); // DISPLAY UPDATE PHASE
  housekeeping();         // HOUSEKEEPING PHASE
  delay(10);
}
```

**Step 2: Event Collection Implementation**
- Migrate interrupt handling to event queueing
- Replace button polling with event generation
- Replace global flags with event queue

**Step 3: State Execution Migration**
- Extract READY logic → `executeReadyState()`
- Extract PRODUCTION logic → `executeProductionState()`
- Extract DIAGNOSTIC logic → `executeDiagnosticState()`
- Extract ERROR logic → `executeErrorState()`

**Step 4: Manager Integration**
- Replace direct SD calls → `storageManager.saveCount()`
- Replace time checks → `timeManager.hasHourChanged()`
- Replace display code → `displayManager.update()`
- Replace config reads → `configManager.getSettings()`

**Step 5: Interrupt Handler Updates**
```cpp
// OLD:
void IRAM_ATTR handleInterrupt() {
  if (productionActive) {
    currentCount++;
    countChanged = true;
  }
}

// NEW:
void IRAM_ATTR handleInterrupt() {
  // Just queue the event, no state changes in ISR
  stateManager.queueEvent(SystemEvent::EVT_COUNTER_PRESSED);
  // Counter increment happens in main loop
}
```

### Backward Compatibility

**Files must remain compatible:**
- `/count.txt` - Same format
- `/cumulative_count.txt` - Same format
- `/hourly_count.txt` - Same format
- Production files - Same format
- Daily logs - Same format
- Serial commands - Same interface

**Verification:**
1. Load files created by code_v3.cpp → Parse with new code
2. Save files with new code → Read with code_v3.cpp
3. Serial commands (START, STOP, etc.) → Work identically
4. Reset behavior → Same as before

---

## Phase 5: Testing & Validation (Week 4-5)

### Unit Test Structure

```cpp
// test/test_state_manager.cpp
TEST(StateManager, TransitionFromInitToReady) {
  StateManager sm;
  sm.initialize();
  sm.queueEvent(SystemEvent::EVT_STARTUP_COMPLETE);
  sm.update();
  EXPECT_EQ(sm.getCurrentState(), SystemState::READY);
}

TEST(StateManager, QueuedEventProcessing) {
  StateManager sm;
  sm.initialize();
  sm.transitionTo(SystemState::READY);
  
  // Queue events
  sm.queueEvent(SystemEvent::EVT_PRODUCTION_START);
  sm.queueEvent(SystemEvent::EVT_COUNTER_PRESSED);
  
  // Process queue
  sm.update();
  EXPECT_EQ(sm.getCurrentState(), SystemState::PRODUCTION);
}

TEST(ProductionManager, SessionLifecycle) {
  ProductionManager pm;
  
  EXPECT_FALSE(pm.isSessionActive());
  pm.startSession();
  EXPECT_TRUE(pm.isSessionActive());
  
  pm.incrementCount();
  pm.incrementCount();
  EXPECT_EQ(pm.getSessionCount(), 2);
  
  pm.stopSession();
  EXPECT_FALSE(pm.isSessionActive());
}
```

### Integration Testing

```cpp
// test/test_complete_workflow.cpp
TEST(CompleteWorkflow, ProductionSession) {
  // 1. Initialize all systems
  // 2. Start production
  // 3. Simulate button presses (count)
  // 4. Verify count display
  // 5. Simulate hour change
  // 6. Verify cumulative count updated
  // 7. Stop production
  // 8. Verify files created
}

TEST(CompleteWorkflow, PowerLossRecovery) {
  // 1. Start production
  // 2. Count some items
  // 3. Simulate power loss (reload from recovery file)
  // 4. Verify count preserved
  // 5. Verify production can continue
}

TEST(Compatibility, FileFormats) {
  // 1. Load files created by code_v3.cpp
  // 2. Verify parsing works
  // 3. Modify with new code
  // 4. Save with new format
  // 5. Verify old code can still read
}
```

### Validation Checklist

- [ ] All states reachable via valid transitions
- [ ] No undefined state transitions
- [ ] Guard conditions prevent invalid transitions
- [ ] Event queue handles overflow gracefully
- [ ] Memory usage ≤ original code
- [ ] CPU usage ≤ original code
- [ ] All existing serial commands work
- [ ] All existing file formats preserved
- [ ] Power loss recovery still works
- [ ] Hour change handling identical
- [ ] Count accuracy maintained
- [ ] Display updates smooth
- [ ] Startup time acceptable
- [ ] No memory leaks
- [ ] Stack usage safe

---

## Code Size & Memory Impact

### Estimated Changes

**Binary Size:**
- Original code_v3.cpp: ~150KB
- FSM headers: ~15KB
- Manager classes: ~40KB
- **Total: ~205KB** (vs 150KB originally)
- **Impact: +36%** (still well within ESP32 1MB limit)

**RAM Usage:**
- StateManager: ~1KB
- Event queue: ~256B
- Managers: ~2KB
- **Total: ~3.3KB extra** (out of 520KB available)
- **Impact: <1%** negligible

**Performance:**
- State machine: <100µs per update
- Event queue: O(1) queue/dequeue
- Manager calls: Same as direct calls
- **Impact: Negligible** (~0.1% CPU overhead)

---

## Rollback Plan

If issues arise, can revert to code_v3.cpp:

1. **No file format changes** → Old code can still read files
2. **Backward compatible** → Can mix old/new code versions
3. **Git history** → Can checkout previous version instantly
4. **Testing** → Unit tests verify compatibility

---

## Success Criteria

✓ Phase 1 (Foundation): Complete
  ✓ StateManager class
  ✓ Manager class interfaces
  ✓ HAL layer
  ✓ Main loop template

→ Phase 2 (Implementation): Starting
  [ ] ProductionManager fully implemented
  [ ] TimeManager fully implemented
  [ ] StorageManager fully implemented
  [ ] DisplayManager fully implemented

→ Phase 3 (State Handlers): Not started
  [ ] All state execution functions
  [ ] Guard conditions
  [ ] Event handlers

→ Phase 4 (Loop Refactoring): Not started
  [ ] Main loop restructured
  [ ] All interrupts integrated
  [ ] Serial command integration

→ Phase 5 (Testing): Not started
  [ ] Unit tests passing
  [ ] Integration tests passing
  [ ] Backward compatibility verified
  [ ] Performance validated

---

## Next Steps

1. **Implement ProductionManager** (fully)
   - Session start/stop
   - Count tracking
   - File persistence
   
2. **Implement TimeManager** (fully)
   - RTC integration
   - Hour change detection
   - Time synchronization

3. **Create remaining Manager implementations**
   - StorageManager
   - DisplayManager
   - ConfigManager
   - LoggerManager

4. **Write state execution functions**
   - executeReadyState()
   - executeProductionState()
   - executeDiagnosticState()
   - executeErrorState()

5. **Refactor main loop**
   - Replace monolithic code
   - Integrate managers
   - Test each phase

6. **Comprehensive testing**
   - Unit tests
   - Integration tests
   - Backward compatibility
   - Performance validation

---

## Questions & Decisions

**Q1: Should we keep code_v3.cpp as reference?**
- A: Yes, keep it for 1-2 months, then archive

**Q2: How to handle gradual migration?**
- A: Create new files, keep main loop using both old/new for transition

**Q3: Testing strategy - simulate vs hardware?**
- A: Unit tests simulated, integration tests on hardware

**Q4: How to debug FSM issues?**
- A: StateLogger with detailed event/transition logging

---

## Document Version

- **Version**: 1.0
- **Date**: November 28, 2025
- **Status**: Phase 1 Complete, Ready for Phase 2
- **Next Review**: After Phase 2 completion

