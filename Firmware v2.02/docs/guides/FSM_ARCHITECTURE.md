/*
 * ESP32 Production Counter - FSM-Based Architecture
 * 
 * DESIGN PRINCIPLES:
 * - Finite State Machine (FSM) for system states
 * - Event-driven architecture
 * - Clear state transitions with guard conditions
 * - Modular subsystems (Display, Storage, Production, Time)
 * - Error handling and recovery strategies
 * 
 * STATE MACHINE DESIGN:
 * 
 * ┌─────────────────────────────────────────────────────────────────┐
 * │                        SYSTEM FSM                               │
 * └─────────────────────────────────────────────────────────────────┘
 * 
 *              ┌──────────────────────────────┐
 *              │   INITIALIZATION_STATE       │
 *              │  (Loading configs, checking  │
 *              │   hardware availability)     │
 *              └──────────────────────────────┘
 *                          │
 *                 ┌────────┴────────┐
 *                 │                 │
 *         [All OK] │           [Error]
 *                 │                 │
 *                 ▼                 ▼
 *    ┌──────────────────────┐  ┌─────────────────┐
 *    │  READY_STATE         │  │  ERROR_STATE    │
 *    │ (Waiting for input)  │  │ (Show error)    │
 *    └──────────────────────┘  │ (Retry setup)   │
 *             │ │              │ (Fatal error)   │
 *             │ │              └─────────────────┘
 *    [Prod ON] │ │ [Diagnostic]
 *             │ │
 *             ▼ ▼
 *    ┌──────────────────────┐  ┌─────────────────┐
 *    │  PRODUCTION_STATE    │  │ DIAGNOSTIC_STATE│
 *    │ (Counting & logging) │  │ (Testing H/W)   │
 *    └──────────────────────┘  └─────────────────┘
 *             │                        │
 *    [Prod OFF]│                       │ [Exit]
 *             │                        │
 *             └────────────┬───────────┘
 *                          │
 *                          ▼
 *            ┌──────────────────────┐
 *            │   READY_STATE        │
 *            │  (Return to normal)  │
 *            └──────────────────────┘
 * 
 * ┌─────────────────────────────────────────────────────────────────┐
 * │              PRODUCTION SESSION FSM (Sub-FSM)                   │
 * └─────────────────────────────────────────────────────────────────┘
 * 
 *    ┌──────────────────────┐
 *    │  SESSION_IDLE        │
 *    │ (No production)      │
 *    └──────────────────────┘
 *             │
 *    [Start Production]
 *             │
 *             ▼
 *    ┌──────────────────────┐
 *    │  SESSION_ACTIVE      │
 *    │ (Counting units)     │
 *    │ (Saving to SD)       │
 *    └──────────────────────┘
 *             │
 *    [Stop Production]
 *             │
 *             ▼
 *    ┌──────────────────────┐
 *    │  SESSION_COMPLETE    │
 *    │ (Save session file)  │
 *    │ (Update daily log)   │
 *    │ (Clear recovery)     │
 *    └──────────────────────┘
 *             │
 *    [Acknowledge]
 *             │
 *             ▼
 *    ┌──────────────────────┐
 *    │  SESSION_IDLE        │
 *    └──────────────────────┘
 * 
 * ┌─────────────────────────────────────────────────────────────────┐
 * │              TIME SYNCHRONIZATION FSM (Sub-FSM)                 │
 * └─────────────────────────────────────────────────────────────────┘
 * 
 *    ┌──────────────────────┐
 *    │  TIME_SYNCED         │
 *    │ (RTC in sync)        │
 *    └──────────────────────┘
 *             │ │
 *    [Hour Change] [Time Set Command]
 *             │ │
 *             ▼ ▼
 *    ┌──────────────────────┐
 *    │  HOUR_TRANSITION     │
 *    │ (Update cumulative)  │
 *    │ (Reset hourly)       │
 *    │ (Log session data)   │
 *    └──────────────────────┘
 *             │
 *    [Transition Complete]
 *             │
 *             ▼
 *    ┌──────────────────────┐
 *    │  TIME_SYNCED         │
 *    └──────────────────────┘
 * 
 * ┌─────────────────────────────────────────────────────────────────┐
 * │                EVENT TYPES (Input)                              │
 * └─────────────────────────────────────────────────────────────────┘
 * 
 * BUTTON EVENTS:
 *   - EVT_COUNTER_PRESSED     : GPIO 15 (unit count)
 *   - EVT_DIAGNOSTIC_PRESSED  : GPIO 27 (test hardware)
 *   - EVT_LATCH_PRESSED       : GPIO 25 (start/stop production)
 *   - EVT_LATCH_RELEASED      : GPIO 25 (released)
 * 
 * TIMING EVENTS:
 *   - EVT_HOUR_CHANGED        : RTC hour boundary crossed
 *   - EVT_SAVE_INTERVAL       : Periodic save timer
 * 
 * SYSTEM EVENTS:
 *   - EVT_STARTUP_COMPLETE    : Initialization finished
 *   - EVT_STARTUP_ERROR       : Initialization failed
 *   - EVT_SD_ERROR            : SD card disconnected
 *   - EVT_RTC_ERROR           : RTC not responding
 *   - EVT_POWER_LOSS_DETECTED : Recovery file found
 * 
 * COMMAND EVENTS:
 *   - EVT_TIME_SET_CMD        : Serial TIME command received
 *   - EVT_CONFIG_CHANGE       : Parameter changed
 * 
 * ┌─────────────────────────────────────────────────────────────────┐
 * │              GUARD CONDITIONS (Transition Guards)                │
 * └─────────────────────────────────────────────────────────────────┘
 * 
 *   - isHardwareReady()          : All subsystems initialized
 *   - isProductionActive()       : Production button held
 *   - isTimeValid()              : RTC has valid time
 *   - isSDCardAvailable()        : SD card connected
 *   - hasDataToSave()            : Pending writes exist
 * 
 * USAGE DOCUMENTATION:
 * 
 * This file provides the conceptual FSM architecture documentation.
 * The actual implementation follows these principles:
 * 
 * 1. MODULAR SUBSYSTEMS:
 *    - ProductionManager    : Handles production session lifecycle
 *    - TimeManager          : RTC and hour change management
 *    - StorageManager       : SD card I/O operations
 *    - DisplayManager       : OLED updates
 *    - ConfigManager        : Parameter persistence
 * 
 * 2. EVENT QUEUE:
 *    - Events generated by ISRs are queued
 *    - Main loop processes events sequentially
 *    - Prevents race conditions
 * 
 * 3. STATE TRANSITIONS:
 *    - Each transition checks guard conditions
 *    - Entry/Exit actions for each state
 *    - Clear error handling paths
 * 
 * 4. RELIABILITY:
 *    - Automatic error recovery
 *    - 3-attempt startup retry
 *    - Power loss recovery
 *    - Watchdog protection
 * 
 * 5. EXTENSIBILITY:
 *    - New states added without modifying existing logic
 *    - New events added to event queue
 *    - Modular subsystem replacement
 *    - Easy to add logging/monitoring
 * 
 * IMPLEMENTATION MAPPING TO CODE:
 * 
 * INITIALIZATION_STATE:
 *   ├─ setup() function
 *   ├─ initializeAllSystems()
 *   ├─ performStartup(attempt)
 *   └─ showFatalError() [Error path]
 * 
 * READY_STATE:
 *   ├─ Main loop idle section
 *   ├─ Display updates
 *   └─ Serial command processing
 * 
 * PRODUCTION_STATE (SESSION_ACTIVE):
 *   ├─ startProduction()
 *   ├─ handleInterrupt() [Event]
 *   ├─ Periodic saves via main loop
 *   └─ stopProduction()
 * 
 * DIAGNOSTIC_STATE:
 *   ├─ enterDiagnosticMode()
 *   ├─ runDiagnosticTests()
 *   ├─ displayDiagnosticResults()
 *   └─ Return to READY_STATE
 * 
 * HOUR_TRANSITION (Sub-state):
 *   ├─ Hour change detection (main loop)
 *   └─ handleHourChange(DateTime)
 *       ├─ Update cumulative count
 *       ├─ Save all count files
 *       └─ Reset counters if not in production
 * 
 * ERROR_STATE:
 *   ├─ displayStartupMessage()
 *   ├─ showStatus(message)
 *   ├─ showFatalError()
 *   └─ Recovery via retry or restart
 * 
 * DATA FLOW:
 * 
 *   ISRs (Interrupt Service Routines)
 *   │
 *   ├─ handleInterrupt()        → currentCount++, countChanged=true
 *   ├─ handleDiagnosticButton() → diagnosticRequested=true
 *   └─ handleLatchingButton()   → productionStatusChanged=true
 *   
 *   Main Loop (Sequential Event Processing)
 *   │
 *   ├─ Poll ISR-set flags
 *   ├─ Check timing conditions
 *   ├─ Execute state-specific actions
 *   ├─ Update persistent storage
 *   ├─ Refresh display
 *   └─ Sleep briefly (debouncing)
 * 
 * TIMING CONSIDERATIONS:
 * 
 * - Count updates: Real-time (ISR)
 * - Save interval: 5000ms (configurable)
 * - Display refresh: 10ms (in main loop)
 * - Hour check: Every 1000ms
 * - Button debounce: 50ms (configurable)
 * - Serial input: ~1ms response
 * 
 * CONCURRENCY & SAFETY:
 * 
 * - ISRs use volatile for shared variables
 * - Critical sections protected with noInterrupts()
 * - No blocking operations in ISRs
 * - Main loop handles all long operations
 * - Watchdog timer prevents hard hangs
 * 
 * MEMORY MANAGEMENT:
 * 
 * - Fixed-size buffers (no dynamic allocation in ISRs)
 * - EEPROM for config persistence
 * - SD card for data storage
 * - Heap used only for display and file operations
 * 
 * EXPANSION POINTS (For Future Features):
 * 
 * 1. Add IDLE_STATE with low-power sleep
 * 2. Add NETWORK_STATE for WiFi logging
 * 3. Add CALIBRATION_STATE for sensor tuning
 * 4. Add BATTERY_LOW_STATE with warnings
 * 5. Add MAINTENANCE_STATE for system checks
 * 6. Implement event logging/replay
 * 7. Add remote monitoring via cloud
 * 8. Implement predictive maintenance
 * 
 * TESTING STRATEGY:
 * 
 * 1. Unit tests: Each subsystem in isolation
 * 2. State tests: Verify valid transitions
 * 3. Event tests: Simulate button presses, time changes
 * 4. Integration tests: Full workflow scenarios
 * 5. Stress tests: High-frequency events
 * 6. Recovery tests: Power loss scenarios
 * 7. Edge cases: Midnight transitions, DST changes
 * 
 * This FSM architecture ensures:
 * ✓ Clear system behavior and predictability
 * ✓ Easy to understand code flow
 * ✓ Simple to add new features
 * ✓ Robust error handling
 * ✓ Testable components
 * ✓ Professional embedded system design
 * 
 */

// ========== EXAMPLE: FSM EVENT TYPE DEFINITIONS ==========
// (For reference - actual implementation uses flag variables)

enum SystemEvent {
  EVT_NONE = 0,
  
  // Button events
  EVT_COUNTER_PRESSED,
  EVT_DIAGNOSTIC_PRESSED,
  EVT_LATCH_PRESSED,
  EVT_LATCH_RELEASED,
  
  // Timing events
  EVT_HOUR_CHANGED,
  EVT_SAVE_INTERVAL,
  EVT_DISPLAY_REFRESH,
  
  // System events
  EVT_STARTUP_COMPLETE,
  EVT_STARTUP_ERROR,
  EVT_SD_ERROR,
  EVT_RTC_ERROR,
  EVT_POWER_LOSS_DETECTED,
  
  // Command events
  EVT_TIME_SET_CMD,
  EVT_CONFIG_CHANGE,
  EVT_SERIAL_INPUT,
};

enum SystemState {
  STATE_INITIALIZATION,
  STATE_READY,
  STATE_PRODUCTION,
  STATE_DIAGNOSTIC,
  STATE_ERROR,
  STATE_HOUR_TRANSITION,
};

// ========== STATE TRANSITION EXAMPLE ==========
// Pseudo-code showing FSM concept:

/*
STATE_READY + EVT_LATCH_PRESSED
├─ Guard: isHardwareReady() && isTimeValid()
├─ Action: 
│  └─ startProduction()
│     ├─ Set productionActive = true
│     ├─ Record productionStartTime
│     ├─ Initialize countAtHourStart
│     └─ Display "Production Started"
└─ Next State: STATE_PRODUCTION

STATE_PRODUCTION + EVT_LATCH_RELEASED
├─ Guard: productionActive == true
├─ Action:
│  └─ stopProduction()
│     ├─ Set productionActive = false
│     ├─ Calculate productionCount
│     ├─ Save production session file
│     ├─ Update daily production log
│     └─ Display "Production Stopped"
└─ Next State: STATE_READY

STATE_PRODUCTION + EVT_HOUR_CHANGED
├─ Guard: currentHour != lastHour
├─ Action:
│  └─ handleHourChange(now)
│     ├─ Calculate count for this hour
│     ├─ Accumulate to cumulativeCount
│     ├─ Save cumulative to file
│     ├─ Reset countAtHourStart (if not in production)
│     └─ Update lastHour
└─ Next State: STATE_PRODUCTION (stay in state)

STATE_READY + EVT_DIAGNOSTIC_PRESSED
├─ Guard: !productionActive
├─ Action:
│  └─ enterDiagnosticMode()
│     ├─ Set diagnosticRequested = true
│     ├─ Run diagnostic tests
│     ├─ Display results
│     └─ Wait for button release
└─ Next State: STATE_DIAGNOSTIC

STATE_DIAGNOSTIC + EVT_DIAGNOSTIC_RELEASED
├─ Guard: True
├─ Action:
│  └─ exitDiagnosticMode()
│     ├─ Set diagnosticRequested = false
│     ├─ Clear display
│     └─ Restore normal operation
└─ Next State: STATE_READY

STATE_INITIALIZATION + EVT_STARTUP_COMPLETE
├─ Guard: allSystemsOk == true
├─ Action:
│  └─ showReadyMessage()
│     ├─ Display "SYSTEM READY"
│     └─ Log startup success
└─ Next State: STATE_READY

STATE_INITIALIZATION + EVT_STARTUP_ERROR
├─ Guard: allSystemsOk == false && attempt < MAX_RETRIES
├─ Action:
│  └─ performStartup(attempt + 1)
│     └─ Retry initialization
└─ Next State: STATE_INITIALIZATION

STATE_INITIALIZATION + EVT_STARTUP_ERROR
├─ Guard: allSystemsOk == false && attempt >= MAX_RETRIES
├─ Action:
│  └─ showFatalError()
│     ├─ Display error on OLED
│     ├─ Log error to serial
│     └─ Enter infinite loop
└─ Next State: STATE_ERROR

STATE_PRODUCTION + EVT_SAVE_INTERVAL
├─ Guard: countChanged == true && (now - lastSaveTime) > SAVE_INTERVAL
├─ Action:
│  └─ saveCountToFile()
│     ├─ Write count.txt
│     ├─ Update lastSaveTime
│     ├─ Clear countChanged flag
│     └─ Save production state (recovery file)
└─ Next State: STATE_PRODUCTION (stay)

STATE_READY + EVT_SERIAL_INPUT
├─ Guard: Serial.available() > 0
├─ Action:
│  └─ processSerialCommand(input)
│     ├─ Parse command
│     ├─ Execute command
│     ├─ Send response
│     └─ Update parameters if SET command
└─ Next State: STATE_READY (stay)

*/

// ========== BENEFITS OF THIS FSM DESIGN ==========

/*
✓ CLARITY:
  - States are explicit and well-defined
  - Transitions are clear and documented
  - Easy to understand system behavior

✓ MAINTAINABILITY:
  - Changes to one state don't affect others
  - Adding new states is straightforward
  - Debugging is simplified (know exact state)

✓ RELIABILITY:
  - Guard conditions prevent invalid transitions
  - Error states are well-handled
  - Recovery paths are explicit

✓ TESTABILITY:
  - Each state can be tested independently
  - Transitions can be verified
  - Edge cases are obvious

✓ SCALABILITY:
  - Easy to add new features as new states
  - Modular design allows sub-FSMs
  - Event-driven makes adding events simple

✓ PROFESSIONAL:
  - Standard embedded systems pattern
  - Used in aerospace, automotive, medical
  - Preferred in code reviews and certifications

✓ DOCUMENTATION:
  - FSM diagram serves as live documentation
  - State names explain intent
  - Guard conditions document assumptions

*/

#endif // FSM_ARCHITECTURE_DESIGN_H
