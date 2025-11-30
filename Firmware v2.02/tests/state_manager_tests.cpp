/**
 * StateManager Unit Tests
 * Tests for FSM core functionality
 * 
 * Test Categories:
 * 1. Initialization tests
 * 2. Event queue tests
 * 3. State transition tests
 * 4. Guard condition tests
 * 5. Stress tests
 */

#include <Arduino.h>
#include "../state_manager.h"

// Test framework macros
#define TEST_PASS 1
#define TEST_FAIL 0
#define ASSERT_EQUAL(actual, expected) ((actual) == (expected) ? TEST_PASS : TEST_FAIL)
#define ASSERT_NOT_EQUAL(actual, expected) ((actual) != (expected) ? TEST_PASS : TEST_FAIL)
#define ASSERT_TRUE(condition) ((condition) ? TEST_PASS : TEST_FAIL)
#define ASSERT_FALSE(condition) (!(condition) ? TEST_PASS : TEST_FAIL)

// Test state and result tracking
struct TestResult {
  const char* testName;
  bool passed;
  const char* message;
  unsigned long executionTime;
};

TestResult testResults[50];
int testCount = 0;

/**
 * Helper function to record test result
 */
void recordTest(const char* name, bool result, const char* message = "") {
  if (testCount < 50) {
    testResults[testCount].testName = name;
    testResults[testCount].passed = result;
    testResults[testCount].message = message;
    testResults[testCount].executionTime = 0;
    testCount++;
  }
}

/**
 * Test 1: StateManager Initialization
 * Verify FSM initializes to INITIALIZATION state
 */
bool test_StateManagerInitialization() {
  StateManager fsm;
  fsm.initialize();
  
  bool result = ASSERT_EQUAL(fsm.getCurrentState(), STATE_INITIALIZATION);
  recordTest("SM_Init_State", result, "Initial state should be INITIALIZATION");
  return result;
}

/**
 * Test 2: Event Queue Initialization
 * Verify event queue starts empty
 */
bool test_EventQueueEmpty() {
  StateManager fsm;
  fsm.initialize();
  
  SystemEvent event;
  bool result = !fsm.dequeueEvent(event);
  recordTest("SM_EventQueue_Empty", result, "Event queue should be empty initially");
  return result;
}

/**
 * Test 3: Enqueue Single Event
 * Verify single event can be enqueued and dequeued
 */
bool test_EnqueueSingleEvent() {
  StateManager fsm;
  fsm.initialize();
  
  // Enqueue event
  fsm.enqueueEvent(EVT_INIT_COMPLETE);
  
  // Dequeue and verify
  SystemEvent event;
  bool hasEvent = fsm.dequeueEvent(event);
  bool eventCorrect = (event == EVT_INIT_COMPLETE);
  
  bool result = hasEvent && eventCorrect;
  recordTest("SM_Enqueue_Single", result, "Should enqueue and dequeue event correctly");
  return result;
}

/**
 * Test 4: Enqueue Multiple Events
 * Verify queue handles multiple events in FIFO order
 */
bool test_EnqueueMultipleEvents() {
  StateManager fsm;
  fsm.initialize();
  
  // Enqueue 5 events
  fsm.enqueueEvent(EVT_PRODUCTION_START);
  fsm.enqueueEvent(EVT_COUNTER_PRESSED);
  fsm.enqueueEvent(EVT_PRODUCTION_STOP);
  fsm.enqueueEvent(EVT_DIAGNOSTIC_MODE);
  fsm.enqueueEvent(EVT_INIT_COMPLETE);
  
  // Dequeue all and verify order
  SystemEvent events[5];
  bool success = true;
  for (int i = 0; i < 5; i++) {
    if (!fsm.dequeueEvent(events[i])) {
      success = false;
      break;
    }
  }
  
  // Verify FIFO order
  if (success) {
    success = (events[0] == EVT_PRODUCTION_START &&
               events[1] == EVT_COUNTER_PRESSED &&
               events[2] == EVT_PRODUCTION_STOP &&
               events[3] == EVT_DIAGNOSTIC_MODE &&
               events[4] == EVT_INIT_COMPLETE);
  }
  
  recordTest("SM_Enqueue_Multiple", success, "Should maintain FIFO order");
  return success;
}

/**
 * Test 5: Event Queue Overflow
 * Verify queue handles overflow gracefully (drops oldest event)
 */
bool test_EventQueueOverflow() {
  StateManager fsm;
  fsm.initialize();
  
  // Enqueue 17 events (exceeds 16-item capacity)
  for (int i = 0; i < 17; i++) {
    fsm.enqueueEvent(EVT_COUNTER_PRESSED);
  }
  
  // Only 16 events should be retrievable
  int eventCount = 0;
  SystemEvent event;
  while (fsm.dequeueEvent(event) && eventCount < 20) {
    eventCount++;
  }
  
  bool result = (eventCount == 16);
  recordTest("SM_Queue_Overflow", result, "Queue should hold max 16 events");
  return result;
}

/**
 * Test 6: State Transition - INIT to READY
 * Verify valid state transition
 */
bool test_TransitionInitToReady() {
  StateManager fsm;
  fsm.initialize();
  
  // Transition from INITIALIZATION to READY
  bool success = fsm.transitionToState(STATE_READY);
  bool stateCorrect = (fsm.getCurrentState() == STATE_READY);
  
  bool result = success && stateCorrect;
  recordTest("SM_Trans_Init_Ready", result, "Should transition INIT→READY");
  return result;
}

/**
 * Test 7: State Transition - READY to PRODUCTION
 * Verify valid state transition
 */
bool test_TransitionReadyToProduction() {
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  
  bool success = fsm.transitionToState(STATE_PRODUCTION);
  bool stateCorrect = (fsm.getCurrentState() == STATE_PRODUCTION);
  
  bool result = success && stateCorrect;
  recordTest("SM_Trans_Ready_Prod", result, "Should transition READY→PRODUCTION");
  return result;
}

/**
 * Test 8: State Transition - PRODUCTION to READY
 * Verify production can return to ready
 */
bool test_TransitionProductionToReady() {
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_PRODUCTION);
  
  bool success = fsm.transitionToState(STATE_READY);
  bool stateCorrect = (fsm.getCurrentState() == STATE_READY);
  
  bool result = success && stateCorrect;
  recordTest("SM_Trans_Prod_Ready", result, "Should transition PRODUCTION→READY");
  return result;
}

/**
 * Test 9: State Transition - READY to DIAGNOSTIC
 * Verify diagnostic mode accessible from READY
 */
bool test_TransitionReadyToDiagnostic() {
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  
  bool success = fsm.transitionToState(STATE_DIAGNOSTIC);
  bool stateCorrect = (fsm.getCurrentState() == STATE_DIAGNOSTIC);
  
  bool result = success && stateCorrect;
  recordTest("SM_Trans_Ready_Diag", result, "Should transition READY→DIAGNOSTIC");
  return result;
}

/**
 * Test 10: State Transition - DIAGNOSTIC to READY
 * Verify recovery from diagnostic
 */
bool test_TransitionDiagnosticToReady() {
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_DIAGNOSTIC);
  
  bool success = fsm.transitionToState(STATE_READY);
  bool stateCorrect = (fsm.getCurrentState() == STATE_READY);
  
  bool result = success && stateCorrect;
  recordTest("SM_Trans_Diag_Ready", result, "Should transition DIAGNOSTIC→READY");
  return result;
}

/**
 * Test 11: Invalid State Transition
 * Verify system prevents direct PRODUCTION→DIAGNOSTIC
 */
bool test_InvalidTransition() {
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_PRODUCTION);
  
  // Try invalid transition (PRODUCTION → DIAGNOSTIC)
  bool success = fsm.transitionToState(STATE_DIAGNOSTIC);
  bool stateUnchanged = (fsm.getCurrentState() == STATE_PRODUCTION);
  
  bool result = !success && stateUnchanged;
  recordTest("SM_Trans_Invalid", result, "Should reject invalid transition");
  return result;
}

/**
 * Test 12: State Transition to ERROR
 * Verify error state accessible from any state
 */
bool test_TransitionToError() {
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_PRODUCTION);
  
  // Transition to ERROR (should work from PRODUCTION)
  bool success = fsm.transitionToState(STATE_ERROR);
  bool stateCorrect = (fsm.getCurrentState() == STATE_ERROR);
  
  bool result = success && stateCorrect;
  recordTest("SM_Trans_To_Error", result, "Should allow transition to ERROR");
  return result;
}

/**
 * Test 13: State Transition from ERROR
 * Verify recovery from ERROR state
 */
bool test_TransitionFromError() {
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_ERROR);
  
  // Recover from ERROR to READY
  bool success = fsm.transitionToState(STATE_READY);
  bool stateCorrect = (fsm.getCurrentState() == STATE_READY);
  
  bool result = success && stateCorrect;
  recordTest("SM_Trans_From_Error", result, "Should recover from ERROR");
  return result;
}

/**
 * Test 14: Complex State Sequence
 * Verify multi-step state transitions
 */
bool test_ComplexStateSequence() {
  StateManager fsm;
  fsm.initialize();
  
  bool success = true;
  
  // Sequence: INIT → READY → PRODUCTION → READY → DIAGNOSTIC → READY
  success = success && fsm.transitionToState(STATE_READY) && (fsm.getCurrentState() == STATE_READY);
  success = success && fsm.transitionToState(STATE_PRODUCTION) && (fsm.getCurrentState() == STATE_PRODUCTION);
  success = success && fsm.transitionToState(STATE_READY) && (fsm.getCurrentState() == STATE_READY);
  success = success && fsm.transitionToState(STATE_DIAGNOSTIC) && (fsm.getCurrentState() == STATE_DIAGNOSTIC);
  success = success && fsm.transitionToState(STATE_READY) && (fsm.getCurrentState() == STATE_READY);
  
  recordTest("SM_Complex_Sequence", success, "Should handle complex state sequence");
  return success;
}

/**
 * Test 15: Event Processing During States
 * Verify events are accessible in different states
 */
bool test_EventProcessingInStates() {
  StateManager fsm;
  fsm.initialize();
  
  // Queue events in INIT state
  fsm.enqueueEvent(EVT_PRODUCTION_START);
  fsm.enqueueEvent(EVT_COUNTER_PRESSED);
  
  // Transition to READY
  fsm.transitionToState(STATE_READY);
  
  // Events should still be available
  SystemEvent event1, event2;
  bool hasEvent1 = fsm.dequeueEvent(event1);
  bool hasEvent2 = fsm.dequeueEvent(event2);
  
  bool result = hasEvent1 && hasEvent2 && (event1 == EVT_PRODUCTION_START) && (event2 == EVT_COUNTER_PRESSED);
  recordTest("SM_Events_In_States", result, "Events should persist across state changes");
  return result;
}

/**
 * Test 16: Event Queue Stress Test
 * Verify queue handles rapid enqueue/dequeue
 */
bool test_EventQueueStress() {
  StateManager fsm;
  fsm.initialize();
  
  unsigned long startTime = millis();
  
  // Rapid enqueue/dequeue cycles
  int cycles = 100;
  bool success = true;
  
  for (int i = 0; i < cycles; i++) {
    fsm.enqueueEvent(EVT_COUNTER_PRESSED);
    SystemEvent event;
    if (!fsm.dequeueEvent(event) || event != EVT_COUNTER_PRESSED) {
      success = false;
      break;
    }
  }
  
  unsigned long elapsed = millis() - startTime;
  
  recordTest("SM_Queue_Stress", success, "Should handle 100 enqueue/dequeue cycles");
  testResults[testCount - 1].executionTime = elapsed;
  return success;
}

/**
 * Test 17: State Transition Timing
 * Verify state transitions are fast
 */
bool test_StateTransitionTiming() {
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  
  unsigned long startTime = micros();
  
  // 10 rapid state transitions
  for (int i = 0; i < 10; i++) {
    fsm.transitionToState(STATE_PRODUCTION);
    fsm.transitionToState(STATE_READY);
  }
  
  unsigned long elapsed = micros() - startTime;
  unsigned long avgTime = elapsed / 20;  // Average per transition
  
  bool result = (avgTime < 1000);  // Should be < 1ms per transition
  recordTest("SM_Trans_Timing", result, "Transitions should be < 1ms");
  testResults[testCount - 1].executionTime = elapsed;
  return result;
}

/**
 * Test 18: Multiple Enqueue Same Event
 * Verify queue handles multiple same events
 */
bool test_MultipleSameEvents() {
  StateManager fsm;
  fsm.initialize();
  
  // Enqueue same event 5 times
  for (int i = 0; i < 5; i++) {
    fsm.enqueueEvent(EVT_COUNTER_PRESSED);
  }
  
  // Dequeue all
  int count = 0;
  SystemEvent event;
  while (fsm.dequeueEvent(event)) {
    if (event == EVT_COUNTER_PRESSED) {
      count++;
    }
  }
  
  bool result = (count == 5);
  recordTest("SM_Multiple_Same", result, "Should queue multiple same events");
  return result;
}

/**
 * Test 19: All Event Types Queueable
 * Verify all 28 event types can be queued
 */
bool test_AllEventTypes() {
  StateManager fsm;
  fsm.initialize();
  
  // All event types (should have 28)
  SystemEvent allEvents[] = {
    EVT_INIT_COMPLETE, EVT_PRODUCTION_START, EVT_PRODUCTION_STOP,
    EVT_COUNTER_PRESSED, EVT_DIAGNOSTIC_MODE, EVT_DIAGNOSTIC_COMPLETE,
    EVT_HOUR_BOUNDARY, EVT_FILE_SAVE, EVT_FILE_ERROR,
    EVT_RTC_ERROR, EVT_SD_ERROR, EVT_HEAP_LOW,
    EVT_BUTTON_DEBOUNCE, EVT_DISPLAY_UPDATE, EVT_LOG_FULL,
    EVT_EEPROM_WRITE, EVT_I2C_ERROR, EVT_SPI_ERROR,
    EVT_WATCHDOG_FEED, EVT_POWER_SAVE, EVT_RECOVERY_START,
    EVT_SESSION_RESUME, EVT_COUNT_SAVE, EVT_STATE_SAVE,
    EVT_LATCH_PRESSED, EVT_LATCH_RELEASED, EVT_THERMAL_WARN,
    EVT_RECOVERY_COMPLETE
  };
  
  // Enqueue all events
  for (int i = 0; i < 28; i++) {
    fsm.enqueueEvent(allEvents[i]);
  }
  
  // Dequeue and count
  int count = 0;
  SystemEvent event;
  while (fsm.dequeueEvent(event)) {
    count++;
  }
  
  bool result = (count == 28);
  recordTest("SM_All_Events", result, "Should queue all 28 event types");
  return result;
}

/**
 * Test 20: Queue Circular Behavior
 * Verify queue properly wraps around
 */
bool test_QueueCircular() {
  StateManager fsm;
  fsm.initialize();
  
  // Fill queue completely (16 items)
  for (int i = 0; i < 16; i++) {
    fsm.enqueueEvent(EVT_COUNTER_PRESSED);
  }
  
  // Dequeue 8 items
  SystemEvent event;
  for (int i = 0; i < 8; i++) {
    fsm.dequeueEvent(event);
  }
  
  // Enqueue 8 more
  for (int i = 0; i < 8; i++) {
    fsm.enqueueEvent(EVT_PRODUCTION_START);
  }
  
  // Dequeue all - first 8 should be PRODUCTION_START
  int correctCount = 0;
  for (int i = 0; i < 16; i++) {
    fsm.dequeueEvent(event);
    if (i < 8 && event == EVT_PRODUCTION_START) {
      correctCount++;
    }
  }
  
  bool result = (correctCount == 8);
  recordTest("SM_Queue_Circular", result, "Queue should wrap around correctly");
  return result;
}

/**
 * Run all tests and print results
 */
void runAllTests() {
  Serial.println("\n========================================");
  Serial.println("StateManager Unit Tests");
  Serial.println("========================================\n");
  
  unsigned long totalStartTime = millis();
  
  // Run all tests
  test_StateManagerInitialization();
  test_EventQueueEmpty();
  test_EnqueueSingleEvent();
  test_EnqueueMultipleEvents();
  test_EventQueueOverflow();
  test_TransitionInitToReady();
  test_TransitionReadyToProduction();
  test_TransitionProductionToReady();
  test_TransitionReadyToDiagnostic();
  test_TransitionDiagnosticToReady();
  test_InvalidTransition();
  test_TransitionToError();
  test_TransitionFromError();
  test_ComplexStateSequence();
  test_EventProcessingInStates();
  test_EventQueueStress();
  test_StateTransitionTiming();
  test_MultipleSameEvents();
  test_AllEventTypes();
  test_QueueCircular();
  
  unsigned long totalTime = millis() - totalStartTime;
  
  // Print results
  Serial.println("\nTest Results:");
  Serial.println("----------------------------------------");
  
  int passCount = 0;
  int failCount = 0;
  
  for (int i = 0; i < testCount; i++) {
    if (testResults[i].passed) {
      Serial.print("✓ PASS: ");
      passCount++;
    } else {
      Serial.print("✗ FAIL: ");
      failCount++;
    }
    
    Serial.print(testResults[i].testName);
    if (testResults[i].message[0] != '\0') {
      Serial.print(" - ");
      Serial.print(testResults[i].message);
    }
    if (testResults[i].executionTime > 0) {
      Serial.print(" (");
      Serial.print(testResults[i].executionTime);
      Serial.print("ms)");
    }
    Serial.println();
  }
  
  Serial.println("----------------------------------------");
  Serial.print("Total: ");
  Serial.print(passCount);
  Serial.print(" passed, ");
  Serial.print(failCount);
  Serial.print(" failed out of ");
  Serial.print(testCount);
  Serial.print(" tests");
  Serial.println();
  Serial.print("Total execution time: ");
  Serial.print(totalTime);
  Serial.println("ms");
  
  float passRate = (float)passCount / testCount * 100;
  Serial.print("Pass rate: ");
  Serial.print(passRate);
  Serial.println("%");
  
  Serial.println("========================================\n");
}

// Entry point for testing
void setup_state_manager_tests() {
  Serial.begin(115200);
  delay(1000);
  runAllTests();
}

void loop_state_manager_tests() {
  delay(10000);  // Run tests once
}
