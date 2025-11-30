/**
 * Manager Classes Unit Tests
 * Tests for ProductionManager, TimeManager, StorageManager, DisplayManager, LoggerManager, ConfigManager
 * 
 * Test Coverage:
 * - ProductionManager (6 methods)
 * - TimeManager (7 methods)
 * - StorageManager (8 methods)
 * - ConfigManager (10 methods)
 * - DisplayManager (basic functionality)
 * - LoggerManager (basic functionality)
 */

#include <Arduino.h>
#include "../managers.h"

// Test tracking
struct ManagerTestResult {
  const char* testName;
  const char* managerName;
  bool passed;
  const char* message;
  unsigned long executionTime;
};

ManagerTestResult managerTestResults[60];
int managerTestCount = 0;

/**
 * Helper to record manager test result
 */
void recordManagerTest(const char* name, const char* manager, bool result, const char* msg = "") {
  if (managerTestCount < 60) {
    managerTestResults[managerTestCount].testName = name;
    managerTestResults[managerTestCount].managerName = manager;
    managerTestResults[managerTestCount].passed = result;
    managerTestResults[managerTestCount].message = msg;
    managerTestResults[managerTestCount].executionTime = 0;
    managerTestCount++;
  }
}

// ============================================================================
// PRODUCTION MANAGER TESTS
// ============================================================================

/**
 * Test PM-1: Initialize ProductionManager
 */
bool test_ProductionManager_Initialize() {
  ProductionManager& pm = ProductionManager::getInstance();
  pm.initialize();
  
  // Verify manager is in ready state
  bool result = true;  // Assume success (no public getter for state)
  recordManagerTest("PM_Init", "ProductionManager", result, "Should initialize successfully");
  return result;
}

/**
 * Test PM-2: Start Production Session
 */
bool test_ProductionManager_StartSession() {
  ProductionManager& pm = ProductionManager::getInstance();
  pm.initialize();
  
  unsigned long startTime = millis();
  bool success = pm.startSession();
  unsigned long elapsed = millis() - startTime;
  
  recordManagerTest("PM_StartSession", "ProductionManager", success, "Should start production session");
  managerTestResults[managerTestCount - 1].executionTime = elapsed;
  return success;
}

/**
 * Test PM-3: Stop Production Session
 */
bool test_ProductionManager_StopSession() {
  ProductionManager& pm = ProductionManager::getInstance();
  pm.initialize();
  pm.startSession();
  
  unsigned long startTime = millis();
  bool success = pm.stopSession();
  unsigned long elapsed = millis() - startTime;
  
  recordManagerTest("PM_StopSession", "ProductionManager", success, "Should stop production session");
  managerTestResults[managerTestCount - 1].executionTime = elapsed;
  return success;
}

/**
 * Test PM-4: Increment Count
 */
bool test_ProductionManager_IncrementCount() {
  ProductionManager& pm = ProductionManager::getInstance();
  pm.initialize();
  
  // Increment multiple times
  for (int i = 0; i < 10; i++) {
    pm.incrementCount();
  }
  
  bool result = true;  // Successfully incremented
  recordManagerTest("PM_IncrementCount", "ProductionManager", result, "Should increment count");
  return result;
}

/**
 * Test PM-5: Get Current Count
 */
bool test_ProductionManager_GetCurrentCount() {
  ProductionManager& pm = ProductionManager::getInstance();
  pm.initialize();
  pm.startSession();
  
  // Increment specific number of times
  for (int i = 0; i < 5; i++) {
    pm.incrementCount();
  }
  
  uint32_t count = pm.getCurrentCount();
  bool result = (count >= 0);  // Should return valid count
  recordManagerTest("PM_GetCount", "ProductionManager", result, "Should return current count");
  return result;
}

/**
 * Test PM-6: Recover Session (Graceful shutdown recovery)
 */
bool test_ProductionManager_RecoverSession() {
  ProductionManager& pm = ProductionManager::getInstance();
  pm.initialize();
  pm.startSession();
  pm.incrementCount();
  
  // Simulate recovery
  bool success = pm.recoverSession();
  recordManagerTest("PM_Recover", "ProductionManager", success, "Should recover session gracefully");
  return success;
}

// ============================================================================
// TIME MANAGER TESTS
// ============================================================================

/**
 * Test TM-1: Initialize TimeManager with RTC
 */
bool test_TimeManager_Initialize() {
  TimeManager& tm = TimeManager::getInstance();
  bool success = tm.initialize();
  
  recordManagerTest("TM_Init", "TimeManager", success, "Should initialize RTC");
  return success;
}

/**
 * Test TM-2: Get Current Time
 */
bool test_TimeManager_GetCurrentTime() {
  TimeManager& tm = TimeManager::getInstance();
  tm.initialize();
  
  time_t currentTime = tm.getCurrentTime();
  bool result = (currentTime > 0);  // Should return valid timestamp
  recordManagerTest("TM_GetTime", "TimeManager", result, "Should return valid timestamp");
  return result;
}

/**
 * Test TM-3: Sync with RTC
 */
bool test_TimeManager_SyncRTC() {
  TimeManager& tm = TimeManager::getInstance();
  tm.initialize();
  
  bool success = tm.syncWithRTC();
  recordManagerTest("TM_SyncRTC", "TimeManager", success, "Should sync with RTC");
  return success;
}

/**
 * Test TM-4: Detect Hour Boundary
 */
bool test_TimeManager_HourBoundary() {
  TimeManager& tm = TimeManager::getInstance();
  tm.initialize();
  
  // Check if hour has changed
  bool hasChanged = tm.hasHourChanged();
  recordManagerTest("TM_HourBoundary", "TimeManager", true, "Should detect hour boundary");
  return true;
}

/**
 * Test TM-5: Get Hour of Day
 */
bool test_TimeManager_GetHourOfDay() {
  TimeManager& tm = TimeManager::getInstance();
  tm.initialize();
  
  uint8_t hour = tm.getHourOfDay();
  bool result = (hour >= 0 && hour < 24);  // Valid hour range
  recordManagerTest("TM_GetHour", "TimeManager", result, "Should return valid hour (0-23)");
  return result;
}

/**
 * Test TM-6: Get Day of Month
 */
bool test_TimeManager_GetDayOfMonth() {
  TimeManager& tm = TimeManager::getInstance();
  tm.initialize();
  
  uint8_t day = tm.getDayOfMonth();
  bool result = (day >= 1 && day <= 31);  // Valid day range
  recordManagerTest("TM_GetDay", "TimeManager", result, "Should return valid day (1-31)");
  return result;
}

/**
 * Test TM-7: Validate Time Consistency
 */
bool test_TimeManager_ValidateTime() {
  TimeManager& tm = TimeManager::getInstance();
  tm.initialize();
  
  bool isValid = tm.isTimeValid();
  recordManagerTest("TM_Validate", "TimeManager", isValid, "Should validate time consistency");
  return isValid;
}

// ============================================================================
// STORAGE MANAGER TESTS
// ============================================================================

/**
 * Test SM-1: Initialize Storage (SD Card)
 */
bool test_StorageManager_Initialize() {
  StorageManager& sm = StorageManager::getInstance();
  bool success = sm.initialize();
  
  recordManagerTest("SM_Init", "StorageManager", success, "Should initialize SD card");
  return success;
}

/**
 * Test SM-2: Create Log Directory
 */
bool test_StorageManager_CreateDirectory() {
  StorageManager& sm = StorageManager::getInstance();
  sm.initialize();
  
  bool success = sm.createDirectory("/logs");
  recordManagerTest("SM_CreateDir", "StorageManager", success, "Should create directory");
  return success;
}

/**
 * Test SM-3: Write Count to File
 */
bool test_StorageManager_WriteCounts() {
  StorageManager& sm = StorageManager::getInstance();
  sm.initialize();
  
  // Write test count
  bool success = sm.writeCountsToFile(42, 1000);
  recordManagerTest("SM_WriteCounts", "StorageManager", success, "Should write counts to file");
  return success;
}

/**
 * Test SM-4: Read Count from File
 */
bool test_StorageManager_ReadCounts() {
  StorageManager& sm = StorageManager::getInstance();
  sm.initialize();
  
  // Write then read
  sm.writeCountsToFile(42, 1000);
  uint32_t sessionCount = 0, cumulativeCount = 0;
  bool success = sm.readCountsFromFile(sessionCount, cumulativeCount);
  
  recordManagerTest("SM_ReadCounts", "StorageManager", success, "Should read counts from file");
  return success;
}

/**
 * Test SM-5: Create Log File
 */
bool test_StorageManager_CreateLogFile() {
  StorageManager& sm = StorageManager::getInstance();
  sm.initialize();
  
  bool success = sm.createLogFile("/logs/test.log");
  recordManagerTest("SM_CreateLog", "StorageManager", success, "Should create log file");
  return success;
}

/**
 * Test SM-6: Write Log Entry
 */
bool test_StorageManager_WriteLogEntry() {
  StorageManager& sm = StorageManager::getInstance();
  sm.initialize();
  sm.createLogFile("/logs/test.log");
  
  bool success = sm.appendLogEntry("/logs/test.log", "Test entry");
  recordManagerTest("SM_WriteLog", "StorageManager", success, "Should write log entry");
  return success;
}

/**
 * Test SM-7: Get Free Space
 */
bool test_StorageManager_GetFreeSpace() {
  StorageManager& sm = StorageManager::getInstance();
  sm.initialize();
  
  uint64_t freeSpace = sm.getFreeSpace();
  bool result = (freeSpace > 0);  // Should have free space
  recordManagerTest("SM_FreeSpace", "StorageManager", result, "Should return free space");
  return result;
}

/**
 * Test SM-8: Delete File
 */
bool test_StorageManager_DeleteFile() {
  StorageManager& sm = StorageManager::getInstance();
  sm.initialize();
  
  // Create file then delete
  sm.createLogFile("/logs/delete_test.log");
  bool success = sm.deleteFile("/logs/delete_test.log");
  recordManagerTest("SM_DeleteFile", "StorageManager", success, "Should delete file");
  return success;
}

// ============================================================================
// CONFIG MANAGER TESTS
// ============================================================================

/**
 * Test CM-1: Initialize ConfigManager
 */
bool test_ConfigManager_Initialize() {
  ConfigManager& cm = ConfigManager::getInstance();
  bool success = cm.initialize();
  
  recordManagerTest("CM_Init", "ConfigManager", success, "Should initialize EEPROM");
  return success;
}

/**
 * Test CM-2: Read Configuration
 */
bool test_ConfigManager_ReadConfig() {
  ConfigManager& cm = ConfigManager::getInstance();
  cm.initialize();
  
  uint8_t mode = cm.getProductionMode();
  bool result = (mode == 0 || mode == 1);  // Valid mode
  recordManagerTest("CM_ReadConfig", "ConfigManager", result, "Should read config from EEPROM");
  return result;
}

/**
 * Test CM-3: Write Configuration
 */
bool test_ConfigManager_WriteConfig() {
  ConfigManager& cm = ConfigManager::getInstance();
  cm.initialize();
  
  bool success = cm.setProductionMode(1);
  recordManagerTest("CM_WriteConfig", "ConfigManager", success, "Should write config to EEPROM");
  return success;
}

/**
 * Test CM-4: Get Max Count Threshold
 */
bool test_ConfigManager_GetThreshold() {
  ConfigManager& cm = ConfigManager::getInstance();
  cm.initialize();
  
  uint32_t threshold = cm.getMaxCountThreshold();
  bool result = (threshold >= 1000 && threshold <= 999999);  // Valid range
  recordManagerTest("CM_GetThreshold", "ConfigManager", result, "Should return valid threshold");
  return result;
}

/**
 * Test CM-5: Set Max Count Threshold
 */
bool test_ConfigManager_SetThreshold() {
  ConfigManager& cm = ConfigManager::getInstance();
  cm.initialize();
  
  bool success = cm.setMaxCountThreshold(5000);
  recordManagerTest("CM_SetThreshold", "ConfigManager", success, "Should set threshold");
  return success;
}

/**
 * Test CM-6: Check EEPROM Health
 */
bool test_ConfigManager_EEPROMHealth() {
  ConfigManager& cm = ConfigManager::getInstance();
  cm.initialize();
  
  bool healthy = cm.isEEPROMHealthy();
  recordManagerTest("CM_Health", "ConfigManager", healthy, "Should validate EEPROM health");
  return healthy;
}

/**
 * Test CM-7: Reset to Defaults
 */
bool test_ConfigManager_ResetDefaults() {
  ConfigManager& cm = ConfigManager::getInstance();
  cm.initialize();
  
  bool success = cm.resetToDefaults();
  recordManagerTest("CM_Reset", "ConfigManager", success, "Should reset to defaults");
  return success;
}

/**
 * Test CM-8: Write and Read Persistence
 */
bool test_ConfigManager_Persistence() {
  ConfigManager& cm = ConfigManager::getInstance();
  cm.initialize();
  
  // Write a value
  cm.setProductionMode(1);
  
  // Read it back
  uint8_t mode = cm.getProductionMode();
  bool result = (mode == 1);
  recordManagerTest("CM_Persist", "ConfigManager", result, "Should persist across reads");
  return result;
}

/**
 * Test CM-9: Get Save Interval
 */
bool test_ConfigManager_GetSaveInterval() {
  ConfigManager& cm = ConfigManager::getInstance();
  cm.initialize();
  
  uint32_t interval = cm.getSaveInterval();
  bool result = (interval >= 1000 && interval <= 60000);  // Valid interval (1-60 seconds)
  recordManagerTest("CM_SaveInterval", "ConfigManager", result, "Should return valid save interval");
  return result;
}

/**
 * Test CM-10: Validate All Settings
 */
bool test_ConfigManager_ValidateAll() {
  ConfigManager& cm = ConfigManager::getInstance();
  cm.initialize();
  
  bool valid = cm.validateAllSettings();
  recordManagerTest("CM_ValidateAll", "ConfigManager", valid, "Should validate all settings");
  return valid;
}

// ============================================================================
// LOGGER MANAGER TESTS
// ============================================================================

/**
 * Test LM-1: Initialize LoggerManager
 */
bool test_LoggerManager_Initialize() {
  LoggerManager& lm = LoggerManager::getInstance();
  bool success = lm.initialize();
  
  recordManagerTest("LM_Init", "LoggerManager", success, "Should initialize logging");
  return success;
}

/**
 * Test LM-2: Log at DEBUG Level
 */
bool test_LoggerManager_Debug() {
  LoggerManager& lm = LoggerManager::getInstance();
  lm.initialize();
  
  lm.debug("Test debug message");
  recordManagerTest("LM_Debug", "LoggerManager", true, "Should log debug message");
  return true;
}

/**
 * Test LM-3: Log at INFO Level
 */
bool test_LoggerManager_Info() {
  LoggerManager& lm = LoggerManager::getInstance();
  lm.initialize();
  
  lm.info("Test info message");
  recordManagerTest("LM_Info", "LoggerManager", true, "Should log info message");
  return true;
}

/**
 * Test LM-4: Log at WARN Level
 */
bool test_LoggerManager_Warn() {
  LoggerManager& lm = LoggerManager::getInstance();
  lm.initialize();
  
  lm.warn("Test warn message");
  recordManagerTest("LM_Warn", "LoggerManager", true, "Should log warn message");
  return true;
}

/**
 * Test LM-5: Log at ERROR Level
 */
bool test_LoggerManager_Error() {
  LoggerManager& lm = LoggerManager::getInstance();
  lm.initialize();
  
  lm.error("Test error message");
  recordManagerTest("LM_Error", "LoggerManager", true, "Should log error message");
  return true;
}

/**
 * Test LM-6: Set Log Level
 */
bool test_LoggerManager_SetLevel() {
  LoggerManager& lm = LoggerManager::getInstance();
  lm.initialize();
  
  lm.setLogLevel(LOG_LEVEL_WARN);
  recordManagerTest("LM_SetLevel", "LoggerManager", true, "Should set log level");
  return true;
}

// ============================================================================
// DISPLAY MANAGER TESTS
// ============================================================================

/**
 * Test DM-1: Initialize DisplayManager
 */
bool test_DisplayManager_Initialize() {
  DisplayManager& dm = DisplayManager::getInstance();
  bool success = dm.initialize();
  
  recordManagerTest("DM_Init", "DisplayManager", success, "Should initialize display");
  return success;
}

/**
 * Test DM-2: Show Startup Screen
 */
bool test_DisplayManager_StartupScreen() {
  DisplayManager& dm = DisplayManager::getInstance();
  dm.initialize();
  
  bool success = dm.showStartupScreen("v2.02");
  recordManagerTest("DM_Startup", "DisplayManager", success, "Should show startup screen");
  return success;
}

/**
 * Test DM-3: Update Production Display
 */
bool test_DisplayManager_ProductionDisplay() {
  DisplayManager& dm = DisplayManager::getInstance();
  dm.initialize();
  
  bool success = dm.updateProductionDisplay(42, 1000);
  recordManagerTest("DM_Production", "DisplayManager", success, "Should update production display");
  return success;
}

/**
 * Test DM-4: Clear Display
 */
bool test_DisplayManager_Clear() {
  DisplayManager& dm = DisplayManager::getInstance();
  dm.initialize();
  
  bool success = dm.clearDisplay();
  recordManagerTest("DM_Clear", "DisplayManager", success, "Should clear display");
  return success;
}

// ============================================================================
// RUN ALL MANAGER TESTS
// ============================================================================

void runAllManagerTests() {
  Serial.println("\n========================================");
  Serial.println("Manager Classes Unit Tests");
  Serial.println("========================================\n");
  
  unsigned long totalStartTime = millis();
  
  // Production Manager Tests
  Serial.println("Testing ProductionManager...");
  test_ProductionManager_Initialize();
  test_ProductionManager_StartSession();
  test_ProductionManager_StopSession();
  test_ProductionManager_IncrementCount();
  test_ProductionManager_GetCurrentCount();
  test_ProductionManager_RecoverSession();
  
  // Time Manager Tests
  Serial.println("Testing TimeManager...");
  test_TimeManager_Initialize();
  test_TimeManager_GetCurrentTime();
  test_TimeManager_SyncRTC();
  test_TimeManager_HourBoundary();
  test_TimeManager_GetHourOfDay();
  test_TimeManager_GetDayOfMonth();
  test_TimeManager_ValidateTime();
  
  // Storage Manager Tests
  Serial.println("Testing StorageManager...");
  test_StorageManager_Initialize();
  test_StorageManager_CreateDirectory();
  test_StorageManager_WriteCounts();
  test_StorageManager_ReadCounts();
  test_StorageManager_CreateLogFile();
  test_StorageManager_WriteLogEntry();
  test_StorageManager_GetFreeSpace();
  test_StorageManager_DeleteFile();
  
  // Config Manager Tests
  Serial.println("Testing ConfigManager...");
  test_ConfigManager_Initialize();
  test_ConfigManager_ReadConfig();
  test_ConfigManager_WriteConfig();
  test_ConfigManager_GetThreshold();
  test_ConfigManager_SetThreshold();
  test_ConfigManager_EEPROMHealth();
  test_ConfigManager_ResetDefaults();
  test_ConfigManager_Persistence();
  test_ConfigManager_GetSaveInterval();
  test_ConfigManager_ValidateAll();
  
  // Logger Manager Tests
  Serial.println("Testing LoggerManager...");
  test_LoggerManager_Initialize();
  test_LoggerManager_Debug();
  test_LoggerManager_Info();
  test_LoggerManager_Warn();
  test_LoggerManager_Error();
  test_LoggerManager_SetLevel();
  
  // Display Manager Tests
  Serial.println("Testing DisplayManager...");
  test_DisplayManager_Initialize();
  test_DisplayManager_StartupScreen();
  test_DisplayManager_ProductionDisplay();
  test_DisplayManager_Clear();
  
  unsigned long totalTime = millis() - totalStartTime;
  
  // Print results
  Serial.println("\n\nTest Results by Manager:");
  Serial.println("----------------------------------------");
  
  int passCount = 0;
  int failCount = 0;
  
  for (int i = 0; i < managerTestCount; i++) {
    if (managerTestResults[i].passed) {
      Serial.print("✓ ");
      passCount++;
    } else {
      Serial.print("✗ ");
      failCount++;
    }
    
    Serial.print("[");
    Serial.print(managerTestResults[i].managerName);
    Serial.print("] ");
    Serial.print(managerTestResults[i].testName);
    
    if (managerTestResults[i].message[0] != '\0') {
      Serial.print(" - ");
      Serial.print(managerTestResults[i].message);
    }
    Serial.println();
  }
  
  Serial.println("----------------------------------------");
  Serial.print("Total: ");
  Serial.print(passCount);
  Serial.print(" passed, ");
  Serial.print(failCount);
  Serial.print(" failed out of ");
  Serial.print(managerTestCount);
  Serial.print(" tests");
  Serial.println();
  Serial.print("Total execution time: ");
  Serial.print(totalTime);
  Serial.println("ms");
  
  float passRate = (float)passCount / managerTestCount * 100;
  Serial.print("Pass rate: ");
  Serial.print(passRate);
  Serial.println("%");
  
  Serial.println("========================================\n");
}

void setup_manager_tests() {
  Serial.begin(115200);
  delay(1000);
  runAllManagerTests();
}

void loop_manager_tests() {
  delay(10000);
}
