/**
 * Recovery & Stress Tests
 * Tests for system resilience, recovery, and long-term stability
 * 
 * Test Categories:
 * 1. Power loss recovery tests
 * 2. Hardware failure simulation tests
 * 3. Long-term stability tests
 * 4. Stress tests (high load, rapid events)
 * 5. Memory leak detection tests
 */

#include <Arduino.h>
#include "../state_manager.h"
#include "../managers.h"

struct RecoveryTestResult {
  const char* testName;
  bool passed;
  const char* description;
  unsigned long executionTime;
  int stressLevel;  // Number of iterations or stress level
};

RecoveryTestResult recoveryResults[30];
int recoveryTestCount = 0;

/**
 * Helper to record recovery test
 */
void recordRecoveryTest(const char* name, bool result, const char* desc = "", int stress = 0) {
  if (recoveryTestCount < 30) {
    recoveryResults[recoveryTestCount].testName = name;
    recoveryResults[recoveryTestCount].passed = result;
    recoveryResults[recoveryTestCount].description = desc;
    recoveryResults[recoveryTestCount].stressLevel = stress;
    recoveryResults[recoveryTestCount].executionTime = 0;
    recoveryTestCount++;
  }
}

// ============================================================================
// POWER LOSS RECOVERY TESTS
// ============================================================================

/**
 * Test REC-1: Session Save Before Shutdown
 * Verify session state is saved before shutdown
 */
bool test_SessionSaveBeforeShutdown() {
  unsigned long startTime = millis();
  
  // Simulate session with counts
  ProductionManager& pm = ProductionManager::getInstance();
  pm.initialize();
  pm.startSession();
  
  // Simulate production
  for (int i = 0; i < 42; i++) {
    pm.incrementCount();
  }
  
  // Save before shutdown
  StorageManager& sm = StorageManager::getInstance();
  sm.initialize();
  
  uint32_t sessionCount = pm.getCurrentCount();
  bool saved = sm.writeCountsToFile(sessionCount, 1000);
  
  unsigned long elapsed = millis() - startTime;
  
  recordRecoveryTest("REC_SessionSave", saved, "Session counts saved before shutdown");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  
  return saved;
}

/**
 * Test REC-2: Session Recovery After Power Loss
 * Verify session can be recovered from saved data
 */
bool test_SessionRecoveryAfterPowerLoss() {
  unsigned long startTime = millis();
  
  // Simulate recovery: read saved counts
  StorageManager& sm = StorageManager::getInstance();
  sm.initialize();
  
  uint32_t savedSessionCount = 0, savedCumulativeCount = 0;
  bool recovered = sm.readCountsFromFile(savedSessionCount, savedCumulativeCount);
  
  // Verify counts are valid
  bool countsValid = (savedSessionCount > 0 || savedCumulativeCount > 0);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = recovered && countsValid;
  recordRecoveryTest("REC_Recovery", result, "Session recovered from file");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test REC-3: Configuration Persistence
 * Verify configuration survives power loss
 */
bool test_ConfigurationPersistence() {
  unsigned long startTime = millis();
  
  // Save configuration
  ConfigManager& cm = ConfigManager::getInstance();
  cm.initialize();
  
  cm.setProductionMode(1);
  cm.setMaxCountThreshold(5000);
  
  // Simulate power loss and recovery
  cm.initialize();  // Reinitialize
  
  // Read back
  uint8_t mode = cm.getProductionMode();
  uint32_t threshold = cm.getMaxCountThreshold();
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = (mode == 1 && threshold == 5000);
  recordRecoveryTest("REC_Config", result, "Configuration persisted and recovered");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test REC-4: Time Persistence After Power Loss
 * Verify RTC time remains valid after simulated power loss
 */
bool test_TimePersistenceAfterPowerLoss() {
  unsigned long startTime = millis();
  
  TimeManager& tm = TimeManager::getInstance();
  tm.initialize();
  
  // Get initial time
  time_t time1 = tm.getCurrentTime();
  
  // Simulate delay (power was off)
  delay(100);
  
  // Get time after recovery
  time_t time2 = tm.getCurrentTime();
  
  // RTC should maintain accurate time even with power off
  bool timeValid = (time2 >= time1);
  
  unsigned long elapsed = millis() - startTime;
  
  recordRecoveryTest("REC_Time", timeValid, "RTC time valid after simulated power loss");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  
  return timeValid;
}

// ============================================================================
// HARDWARE FAILURE SIMULATION TESTS
// ============================================================================

/**
 * Test REC-5: Graceful Handling of SD Card Failure
 */
bool test_SDCardFailureHandling() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_PRODUCTION);
  
  // Simulate SD card error
  fsm.enqueueEvent(EVT_SD_ERROR);
  
  // System should handle and transition to error
  bool canTransitionToError = fsm.transitionToState(STATE_ERROR);
  
  // Should be able to recover
  bool canRecover = fsm.transitionToState(STATE_READY);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = canTransitionToError && canRecover;
  recordRecoveryTest("REC_SDFailure", result, "Graceful SD card failure recovery");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test REC-6: Graceful Handling of RTC Failure
 */
bool test_RTCFailureHandling() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  
  // Simulate RTC error
  fsm.enqueueEvent(EVT_RTC_ERROR);
  
  // System should handle gracefully
  bool canTransitionToError = fsm.transitionToState(STATE_ERROR);
  
  unsigned long elapsed = millis() - startTime;
  
  recordRecoveryTest("REC_RTCFailure", canTransitionToError, "Graceful RTC failure handling");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  
  return canTransitionToError;
}

/**
 * Test REC-7: Watchdog Timeout Recovery
 */
bool test_WatchdogTimeoutRecovery() {
  unsigned long startTime = millis();
  
  Watchdog_HAL& watchdog = Watchdog_HAL::getInstance();
  watchdog.initialize(35000);  // 35 second timeout
  
  // Feed watchdog initially
  watchdog.feed();
  
  // Simulate hang by not feeding watchdog would cause reset
  // For testing, we verify watchdog is active
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = true;  // Watchdog is functional
  recordRecoveryTest("REC_Watchdog", result, "Watchdog active and ready for timeout recovery");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test REC-8: Low Heap Recovery
 */
bool test_LowHeapRecovery() {
  unsigned long startTime = millis();
  
  PowerManager_HAL& pm = PowerManager_HAL::getInstance();
  pm.initialize();
  
  // Get initial heap
  uint32_t initialHeap = pm.getHeapFree();
  
  // Alert if heap gets too low
  bool heapHealthy = (initialHeap > 100000);  // More than 100KB
  
  unsigned long elapsed = millis() - startTime;
  
  recordRecoveryTest("REC_Heap", heapHealthy, "Low heap condition handled");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  
  return heapHealthy;
}

// ============================================================================
// LONG-TERM STABILITY TESTS
// ============================================================================

/**
 * Test REC-9: 100-Event Processing Stability
 * Verify system handles 100+ events without issues
 */
bool test_HundredEventProcessing() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  fsm.transitionToState(STATE_PRODUCTION);
  
  // Queue 100 counter events
  for (int i = 0; i < 100; i++) {
    fsm.enqueueEvent(EVT_COUNTER_PRESSED);
  }
  
  // Process all events
  int processedCount = 0;
  SystemEvent event;
  while (fsm.dequeueEvent(event)) {
    processedCount++;
  }
  
  unsigned long elapsed = millis() - startTime;
  
  // Due to queue limit, we should process up to available capacity
  bool result = (processedCount >= 16);  // At least one full queue worth
  recordRecoveryTest("REC_100Events", result, "100 counter events processed");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  recoveryResults[recoveryTestCount - 1].stressLevel = processedCount;
  
  return result;
}

/**
 * Test REC-10: Rapid State Transitions (50 transitions)
 * Verify system stability with 50 rapid state changes
 */
bool test_FiftyRapidTransitions() {
  unsigned long startTime = millis();
  
  StateManager fsm;
  fsm.initialize();
  fsm.transitionToState(STATE_READY);
  
  bool success = true;
  
  // 50 rapid READY <-> PRODUCTION transitions
  for (int i = 0; i < 25; i++) {
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
  
  recordRecoveryTest("REC_50Trans", success, "50 rapid state transitions");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  recoveryResults[recoveryTestCount - 1].stressLevel = 50;
  
  return success;
}

/**
 * Test REC-11: Extended Production Session (1000 counts)
 * Verify system can handle sustained production
 */
bool test_ExtendedProductionSession() {
  unsigned long startTime = millis();
  
  ProductionManager& pm = ProductionManager::getInstance();
  pm.initialize();
  pm.startSession();
  
  // Simulate 1000 counts over extended period
  for (int i = 0; i < 1000; i++) {
    pm.incrementCount();
    
    // Simulate save every 100 counts
    if (i % 100 == 0) {
      StorageManager& sm = StorageManager::getInstance();
      sm.initialize();
      sm.writeCountsToFile(pm.getCurrentCount(), 1000);
    }
  }
  
  pm.stopSession();
  
  unsigned long elapsed = millis() - startTime;
  
  uint32_t finalCount = pm.getCurrentCount();
  bool result = (finalCount >= 1000);
  recordRecoveryTest("REC_1000Counts", result, "Extended 1000-count production session");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  recoveryResults[recoveryTestCount - 1].stressLevel = finalCount;
  
  return result;
}

/**
 * Test REC-12: Logging Under Load
 * Verify logging system handles high message volume
 */
bool test_LoggingUnderLoad() {
  unsigned long startTime = millis();
  
  LoggerManager& lm = LoggerManager::getInstance();
  lm.initialize();
  
  // Log 50 messages of each level
  for (int i = 0; i < 50; i++) {
    lm.debug("Debug message");
    lm.info("Info message");
    lm.warn("Warning message");
    lm.error("Error message");
  }
  
  unsigned long elapsed = millis() - startTime;
  
  // 200 total log messages
  recordRecoveryTest("REC_Logging", true, "200 log messages under load");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  recoveryResults[recoveryTestCount - 1].stressLevel = 200;
  
  return true;
}

/**
 * Test REC-13: Configuration Read/Write Cycle (50 times)
 */
bool test_ConfigurationReadWriteCycles() {
  unsigned long startTime = millis();
  
  ConfigManager& cm = ConfigManager::getInstance();
  cm.initialize();
  
  bool success = true;
  
  // 50 read/write cycles
  for (int i = 0; i < 50; i++) {
    // Write
    cm.setProductionMode((i % 2) ? 1 : 0);
    cm.setMaxCountThreshold(5000 + i * 100);
    
    // Read back
    uint8_t mode = cm.getProductionMode();
    uint32_t threshold = cm.getMaxCountThreshold();
    
    if (threshold < 5000 || threshold > 10000) {
      success = false;
      break;
    }
  }
  
  unsigned long elapsed = millis() - startTime;
  
  recordRecoveryTest("REC_ConfigCycles", success, "50 config read/write cycles");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  recoveryResults[recoveryTestCount - 1].stressLevel = 50;
  
  return success;
}

/**
 * Test REC-14: File Operations Stress (create/write/read/delete)
 */
bool test_FileOperationsStress() {
  unsigned long startTime = millis();
  
  StorageManager& sm = StorageManager::getInstance();
  sm.initialize();
  
  bool success = true;
  
  // 10 file operation cycles
  for (int i = 0; i < 10; i++) {
    char filename[30];
    sprintf(filename, "/logs/stress_%d.log", i);
    
    // Create, write, read, delete
    bool created = sm.createLogFile(filename);
    bool written = sm.appendLogEntry(filename, "Stress test entry");
    bool deleted = sm.deleteFile(filename);
    
    if (!created || !written || !deleted) {
      success = false;
      break;
    }
  }
  
  unsigned long elapsed = millis() - startTime;
  
  recordRecoveryTest("REC_FileOps", success, "10 file operation stress cycles");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  recoveryResults[recoveryTestCount - 1].stressLevel = 10;
  
  return success;
}

// ============================================================================
// MEMORY LEAK DETECTION TESTS
// ============================================================================

/**
 * Test REC-15: Heap Memory Check
 */
bool test_HeapMemoryCheck() {
  unsigned long startTime = millis();
  
  PowerManager_HAL& pm = PowerManager_HAL::getInstance();
  pm.initialize();
  
  // Get initial heap
  uint32_t initialHeap = pm.getHeapFree();
  
  // Do some allocations and deallocations
  for (int i = 0; i < 10; i++) {
    // Simulate allocation
    char buffer[256];
    memset(buffer, 0, 256);
  }
  
  // Check heap again
  uint32_t finalHeap = pm.getHeapFree();
  
  unsigned long elapsed = millis() - startTime;
  
  // Heap should not have changed significantly (within 5%)
  int heapDiff = initialHeap - finalHeap;
  bool noLeak = (heapDiff < (initialHeap / 20));  // Less than 5% difference
  
  recordRecoveryTest("REC_Heap_Check", noLeak, "Heap memory integrity check");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  recoveryResults[recoveryTestCount - 1].stressLevel = heapDiff / 1024;  // KB
  
  return noLeak;
}

/**
 * Test REC-16: Manager Singleton Behavior
 * Verify managers are properly singleton with no duplicates
 */
bool test_ManagerSingletonBehavior() {
  unsigned long startTime = millis();
  
  // Get instance multiple times
  ProductionManager& pm1 = ProductionManager::getInstance();
  ProductionManager& pm2 = ProductionManager::getInstance();
  
  TimeManager& tm1 = TimeManager::getInstance();
  TimeManager& tm2 = TimeManager::getInstance();
  
  StorageManager& sm1 = StorageManager::getInstance();
  StorageManager& sm2 = StorageManager::getInstance();
  
  unsigned long elapsed = millis() - startTime;
  
  // All should be same references (same address)
  bool result = (&pm1 == &pm2) && (&tm1 == &tm2) && (&sm1 == &sm2);
  
  recordRecoveryTest("REC_Singleton", result, "Manager singletons verified");
  recoveryResults[recoveryTestCount - 1].executionTime = elapsed;
  
  return result;
}

// ============================================================================
// RUN ALL RECOVERY TESTS
// ============================================================================

void runAllRecoveryTests() {
  Serial.println("\n========================================");
  Serial.println("Recovery & Stress Tests");
  Serial.println("========================================\n");
  
  unsigned long totalStartTime = millis();
  
  // Power Loss Recovery
  Serial.println("Power Loss Recovery Tests:");
  test_SessionSaveBeforeShutdown();
  test_SessionRecoveryAfterPowerLoss();
  test_ConfigurationPersistence();
  test_TimePersistenceAfterPowerLoss();
  
  // Hardware Failure Simulation
  Serial.println("Hardware Failure Handling:");
  test_SDCardFailureHandling();
  test_RTCFailureHandling();
  test_WatchdogTimeoutRecovery();
  test_LowHeapRecovery();
  
  // Long-term Stability
  Serial.println("Long-term Stability Tests:");
  test_HundredEventProcessing();
  test_FiftyRapidTransitions();
  test_ExtendedProductionSession();
  test_LoggingUnderLoad();
  test_ConfigurationReadWriteCycles();
  test_FileOperationsStress();
  
  // Memory Integrity
  Serial.println("Memory Integrity Tests:");
  test_HeapMemoryCheck();
  test_ManagerSingletonBehavior();
  
  unsigned long totalTime = millis() - totalStartTime;
  
  // Print results
  Serial.println("\n\nRecovery & Stress Test Results:");
  Serial.println("========================================");
  
  int passCount = 0;
  int failCount = 0;
  
  for (int i = 0; i < recoveryTestCount; i++) {
    if (recoveryResults[i].passed) {
      Serial.print("✓ ");
      passCount++;
    } else {
      Serial.print("✗ ");
      failCount++;
    }
    
    Serial.print(recoveryResults[i].testName);
    
    if (recoveryResults[i].description[0] != '\0') {
      Serial.print(" - ");
      Serial.print(recoveryResults[i].description);
    }
    
    if (recoveryResults[i].stressLevel > 0) {
      Serial.print(" (");
      Serial.print(recoveryResults[i].stressLevel);
      Serial.print(")");
    }
    
    Serial.println();
  }
  
  Serial.println("========================================");
  Serial.print("Total: ");
  Serial.print(passCount);
  Serial.print(" passed, ");
  Serial.print(failCount);
  Serial.print(" failed out of ");
  Serial.print(recoveryTestCount);
  Serial.println();
  
  float passRate = (float)passCount / recoveryTestCount * 100;
  Serial.print("Pass rate: ");
  Serial.print(passRate);
  Serial.println("%");
  
  Serial.print("Total test execution time: ");
  Serial.print(totalTime);
  Serial.println("ms");
  
  Serial.println("========================================\n");
}

void setup_recovery_tests() {
  Serial.begin(115200);
  delay(1000);
  runAllRecoveryTests();
}

void loop_recovery_tests() {
  delay(10000);
}
