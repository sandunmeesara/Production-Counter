/**
 * FSM Integration Tests
 * Tests for complete FSM behavior including state handlers and event processing
 * 
 * Test Categories:
 * 1. State handler execution tests
 * 2. Complete state transitions tests
 * 3. Guard condition enforcement tests
 * 4. Production flow tests
 * 5. Error recovery tests
 * 6. Integration scenario tests
 */

#include <Arduino.h>
#include "../state_manager.h"
#include "../state_handlers.h"

struct IntegrationTestResult {
  const char* testName;
  bool passed;
  const char* description;
  unsigned long executionTime;
  const char* failureReason;
};

IntegrationTestResult integrationResults[40];
int integrationTestCount = 0;

/**
 * Helper to record integration test
 */
void recordIntegrationTest(const char* name, bool result, const char* desc = "", const char* reason = "") {
  if (integrationTestCount < 40) {
    integrationResults[integrationTestCount].testName = name;
    integrationResults[integrationTestCount].passed = result;
    integrationResults[integrationTestCount].description = desc;
    integrationResults[integrationTestCount].failureReason = reason;
    integrationResults[integrationTestCount].executionTime = 0;
    integrationTestCount++;
  }
}

/**
 * Test INT-1: Complete Initialization Sequence
 * Verify all initialization steps execute correctly
 */
bool test_CompleteInitializationSequence() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  
  // Verify initial state
  if (fsm.getCurrentState() != STATE_INITIALIZATION) {
    recordIntegrationTest("Int_InitSeq", false, "Complete init sequence", "Initial state wrong");
    return false;
  }
  
  // Queue init events
  fsm.enqueueEvent(EVT_INIT_COMPLETE);
  
  // Transition to READY
  bool success = fsm.transitionToState(STATE_READY);
  
  // Dequeue event
  SystemEvent event;
  fsm.dequeueEvent(event);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = success && (fsm.getCurrentState() == STATE_READY) && (event == EVT_INIT_COMPLETE);
  recordIntegrationTest("Int_InitSeq", result, "Complete init sequence");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-2: Production Start Flow
 * Verify production can be started from READY state
 */
bool test_ProductionStartFlow() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  
  // Queue production start
  fsm.enqueueEvent(EVT_PRODUCTION_START);
  
  // Transition to PRODUCTION
  bool success = fsm.transitionToState(STATE_PRODUCTION);
  
  // Get event
  SystemEvent event;
  bool hasEvent = fsm.dequeueEvent(event);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = success && (fsm.getCurrentState() == STATE_PRODUCTION) && 
                (event == EVT_PRODUCTION_START) && hasEvent;
  recordIntegrationTest("Int_ProdStart", result, "Production start flow");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-3: Counting During Production
 * Verify counter events are processed during production
 */
bool test_CountingDuringProduction() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_PRODUCTION);
  
  // Queue counter events
  for (int i = 0; i < 10; i++) {
    fsm.enqueueEvent(EVT_COUNTER_PRESSED);
  }
  
  // Count events
  int eventCount = 0;
  SystemEvent event;
  while (fsm.dequeueEvent(event)) {
    if (event == EVT_COUNTER_PRESSED) {
      eventCount++;
    }
  }
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = (eventCount == 10) && (fsm.getCurrentState() == STATE_PRODUCTION);
  recordIntegrationTest("Int_Counting", result, "Counter events during production");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-4: Production Stop Flow
 * Verify production can be stopped and return to READY
 */
bool test_ProductionStopFlow() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_PRODUCTION);
  
  // Queue stop event
  fsm.enqueueEvent(EVT_PRODUCTION_STOP);
  
  // Transition back to READY
  bool success = fsm.transitionToState(STATE_READY);
  
  // Get event
  SystemEvent event;
  bool hasEvent = fsm.dequeueEvent(event);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = success && (fsm.getCurrentState() == STATE_READY) && 
                (event == EVT_PRODUCTION_STOP) && hasEvent;
  recordIntegrationTest("Int_ProdStop", result, "Production stop flow");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-5: Diagnostic Mode Entry
 * Verify diagnostic mode can be entered from READY
 */
bool test_DiagnosticModeEntry() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  
  // Queue diagnostic event
  fsm.enqueueEvent(EVT_DIAGNOSTIC_MODE);
  
  // Transition to DIAGNOSTIC
  bool success = fsm.transitionToState(STATE_DIAGNOSTIC);
  
  // Get event
  SystemEvent event;
  bool hasEvent = fsm.dequeueEvent(event);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = success && (fsm.getCurrentState() == STATE_DIAGNOSTIC) && 
                (event == EVT_DIAGNOSTIC_MODE) && hasEvent;
  recordIntegrationTest("Int_DiagEntry", result, "Diagnostic mode entry");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-6: Diagnostic Mode Exit
 * Verify diagnostic mode can be exited back to READY
 */
bool test_DiagnosticModeExit() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_DIAGNOSTIC);
  
  // Queue complete event
  fsm.enqueueEvent(EVT_DIAGNOSTIC_COMPLETE);
  
  // Transition back to READY
  bool success = fsm.transitionToState(STATE_READY);
  
  // Get event
  SystemEvent event;
  bool hasEvent = fsm.dequeueEvent(event);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = success && (fsm.getCurrentState() == STATE_READY) && 
                (event == EVT_DIAGNOSTIC_COMPLETE) && hasEvent;
  recordIntegrationTest("Int_DiagExit", result, "Diagnostic mode exit");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-7: Error State Entry
 * Verify system can enter ERROR state from any state
 */
bool test_ErrorStateEntry() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_PRODUCTION);
  
  // Queue error event
  fsm.enqueueEvent(EVT_SD_ERROR);
  
  // Transition to ERROR
  bool success = fsm.transitionToState(STATE_ERROR);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = success && (fsm.getCurrentState() == STATE_ERROR);
  recordIntegrationTest("Int_ErrorEntry", result, "Error state entry from production");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-8: Error Recovery
 * Verify recovery from ERROR state
 */
bool test_ErrorRecovery() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_ERROR);
  
  // Queue recovery event
  fsm.enqueueEvent(EVT_RECOVERY_COMPLETE);
  
  // Transition back to READY
  bool success = fsm.transitionToState(STATE_READY);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = success && (fsm.getCurrentState() == STATE_READY);
  recordIntegrationTest("Int_ErrorRecovery", result, "Recovery from error state");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-9: Hour Boundary Event
 * Verify hour boundary event is properly handled
 */
bool test_HourBoundaryEvent() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  
  // Queue hour boundary event
  fsm.enqueueEvent(EVT_HOUR_BOUNDARY);
  
  // Event should persist after state changes
  fsm.transitionToState(STATE_PRODUCTION);
  
  // Get event
  SystemEvent event;
  bool hasEvent = fsm.dequeueEvent(event);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = hasEvent && (event == EVT_HOUR_BOUNDARY);
  recordIntegrationTest("Int_HourBoundary", result, "Hour boundary event handling");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-10: Multiple Events in Queue During Transitions
 * Verify event queue is maintained across state transitions
 */
bool test_EventQueueAcrossTransitions() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  
  // Queue events in INIT state
  fsm.enqueueEvent(EVT_INIT_COMPLETE);
  fsm.enqueueEvent(EVT_FILE_SAVE);
  fsm.enqueueEvent(EVT_HOUR_BOUNDARY);
  
  // Multiple state transitions
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_PRODUCTION);
  fsm.transitionToState(STATE_READY);
  
  // All events should still be available
  int eventCount = 0;
  SystemEvent event;
  while (fsm.dequeueEvent(event)) {
    eventCount++;
  }
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = (eventCount == 3);
  recordIntegrationTest("Int_QueuePersist", result, "Events persist across transitions");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-11: Guard Condition - Cannot Start Production Twice
 * Verify guard prevents invalid transitions
 */
bool test_GuardConditionProduction() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_PRODUCTION);
  
  // Try to transition to PRODUCTION again (should fail)
  bool shouldFail = fsm.transitionToState(STATE_PRODUCTION);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = !shouldFail && (fsm.getCurrentState() == STATE_PRODUCTION);
  recordIntegrationTest("Int_Guard_Prod", result, "Guard prevents invalid production start");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-12: Complex Event Scenario
 * Simulate realistic event flow during production
 */
bool test_ComplexEventScenario() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  
  // Simulate realistic production scenario
  fsm.enqueueEvent(EVT_PRODUCTION_START);
  fsm.transitionToState(STATE_PRODUCTION);
  
  // Queue counter events
  for (int i = 0; i < 5; i++) {
    fsm.enqueueEvent(EVT_COUNTER_PRESSED);
  }
  
  // Queue periodic save
  fsm.enqueueEvent(EVT_FILE_SAVE);
  fsm.enqueueEvent(EVT_COUNT_SAVE);
  
  // Queue hour boundary
  fsm.enqueueEvent(EVT_HOUR_BOUNDARY);
  
  // Count all events
  int totalEvents = 0;
  SystemEvent event;
  while (fsm.dequeueEvent(event)) {
    totalEvents++;
  }
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = (totalEvents == 8) && (fsm.getCurrentState() == STATE_PRODUCTION);
  recordIntegrationTest("Int_Complex", result, "Complex event scenario with 8 events");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-13: Error During Production
 * Verify error handling while in production
 */
bool test_ErrorDuringProduction() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_PRODUCTION);
  
  // Queue counter events
  for (int i = 0; i < 3; i++) {
    fsm.enqueueEvent(EVT_COUNTER_PRESSED);
  }
  
  // Queue error event
  fsm.enqueueEvent(EVT_SD_ERROR);
  
  // Transition to error (should work from production)
  bool success = fsm.transitionToState(STATE_ERROR);
  
  // Queue recovery event
  fsm.enqueueEvent(EVT_RECOVERY_COMPLETE);
  
  // Recover to ready
  bool recovered = fsm.transitionToState(STATE_READY);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = success && recovered && (fsm.getCurrentState() == STATE_READY);
  recordIntegrationTest("Int_ErrorProd", result, "Error during production recovery");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-14: Rapid State Changes
 * Stress test rapid state transitions
 */
bool test_RapidStateChanges() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  
  bool success = true;
  
  // Rapid transitions
  for (int i = 0; i < 10; i++) {
    if (!fsm.transitionToState(STATE_PRODUCTION)) {
      success = false;
      break;
    }
    if (!fsm.transitionToState(STATE_READY)) {
      success = false;
      break;
    }
  }
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = success && (fsm.getCurrentState() == STATE_READY);
  recordIntegrationTest("Int_RapidTransit", result, "20 rapid state transitions");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test INT-15: State Transition Timeline
 * Verify expected sequence of events
 */
bool test_StateTransitionTimeline() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  
  // Expected sequence: INIT -> READY -> PROD -> READY -> DIAG -> READY -> ERROR -> READY
  SystemState expectedSequence[] = {
    STATE_READY,
    STATE_PRODUCTION,
    STATE_READY,
    STATE_DIAGNOSTIC,
    STATE_READY,
    STATE_ERROR,
    STATE_READY
  };
  
  bool success = true;
  
  for (int i = 0; i < 7; i++) {
    if (!fsm.transitionToState(expectedSequence[i])) {
      success = false;
      break;
    }
    
    if (fsm.getCurrentState() != expectedSequence[i]) {
      success = false;
      break;
    }
  }
  
  unsigned long elapsed = millis() - startTime;
  
  recordIntegrationTest("Int_Timeline", success, "7-step state transition timeline");
  integrationResults[integrationTestCount - 1].executionTime = elapsed;
  
  return success;
}

/**
 * Run all integration tests
 */
void runAllIntegrationTests() {
  Serial.println("\n========================================");
  Serial.println("FSM Integration Tests");
  Serial.println("========================================\n");
  
  unsigned long totalStartTime = millis();
  
  // Run all tests
  test_CompleteInitializationSequence();
  test_ProductionStartFlow();
  test_CountingDuringProduction();
  test_ProductionStopFlow();
  test_DiagnosticModeEntry();
  test_DiagnosticModeExit();
  test_ErrorStateEntry();
  test_ErrorRecovery();
  test_HourBoundaryEvent();
  test_EventQueueAcrossTransitions();
  test_GuardConditionProduction();
  test_ComplexEventScenario();
  test_ErrorDuringProduction();
  test_RapidStateChanges();
  test_StateTransitionTimeline();
  
  unsigned long totalTime = millis() - totalStartTime;
  
  // Print results
  Serial.println("\nIntegration Test Results:");
  Serial.println("----------------------------------------");
  
  int passCount = 0;
  int failCount = 0;
  
  for (int i = 0; i < integrationTestCount; i++) {
    if (integrationResults[i].passed) {
      Serial.print("✓ PASS: ");
      passCount++;
    } else {
      Serial.print("✗ FAIL: ");
      failCount++;
    }
    
    Serial.print(integrationResults[i].testName);
    
    if (integrationResults[i].description[0] != '\0') {
      Serial.print(" - ");
      Serial.print(integrationResults[i].description);
    }
    
    if (integrationResults[i].executionTime > 0) {
      Serial.print(" (");
      Serial.print(integrationResults[i].executionTime);
      Serial.print("ms)");
    }
    
    if (!integrationResults[i].passed && integrationResults[i].failureReason[0] != '\0') {
      Serial.print(" [");
      Serial.print(integrationResults[i].failureReason);
      Serial.print("]");
    }
    
    Serial.println();
  }
  
  Serial.println("----------------------------------------");
  Serial.print("Total: ");
  Serial.print(passCount);
  Serial.print(" passed, ");
  Serial.print(failCount);
  Serial.print(" failed out of ");
  Serial.print(integrationTestCount);
  Serial.print(" tests");
  Serial.println();
  Serial.print("Total execution time: ");
  Serial.print(totalTime);
  Serial.println("ms");
  
  float passRate = (float)passCount / integrationTestCount * 100;
  Serial.print("Pass rate: ");
  Serial.print(passRate);
  Serial.println("%");
  
  Serial.println("========================================\n");
}

void setup_integration_tests() {
  Serial.begin(115200);
  delay(1000);
  runAllIntegrationTests();
}

void loop_integration_tests() {
  delay(10000);
}
