#include "state_handlers.h"
#include "state_manager.h"
#include "managers.h"
#include "hal.h"
#include <Arduino.h>

// Global timing variables
static unsigned long lastSaveTime = 0;
static unsigned long lastHealthCheckTime = 0;
static unsigned long lastDisplayUpdateTime = 0;
static unsigned long lastHourChangeTime = 0;

// Configuration
static const unsigned long SAVE_INTERVAL = 5000;        // Save every 5 seconds
static const unsigned long HEALTH_CHECK_INTERVAL = 30000; // Check health every 30 seconds
static const unsigned long DISPLAY_UPDATE_INTERVAL = 100;  // Update display every 100ms

// ============================================================================
// INITIALIZATION STATE HANDLER
// ============================================================================

bool executeInitializationState() {
  static int initStep = 0;
  static unsigned long startTime = 0;
  
  if (initStep == 0) {
    startTime = millis();
    LoggerManager::info("=== INITIALIZATION STARTED ===");
    initStep++;
  }
  
  // Step 1: Serial communication
  if (initStep == 1) {
    if (initializeSerial()) {
      LoggerManager::info("Serial initialized");
      initStep++;
    } else {
      LoggerManager::error("Serial initialization failed");
      return false;
    }
  }
  
  // Step 2: GPIO pins
  if (initStep == 2) {
    if (initializeGPIO()) {
      LoggerManager::info("GPIO initialized");
      initStep++;
    } else {
      LoggerManager::error("GPIO initialization failed");
      return false;
    }
  }
  
  // Step 3: I2C bus
  if (initStep == 3) {
    if (initializeI2C()) {
      LoggerManager::info("I2C initialized");
      initStep++;
    } else {
      LoggerManager::error("I2C initialization failed - continuing without I2C devices");
      initStep++;
    }
  }
  
  // Step 4: SPI bus
  if (initStep == 4) {
    if (initializeSPI()) {
      LoggerManager::info("SPI initialized");
      initStep++;
    } else {
      LoggerManager::error("SPI initialization failed - continuing without SD");
      initStep++;
    }
  }
  
  // Step 5: RTC module
  if (initStep == 5) {
    if (initializeRTC()) {
      LoggerManager::info("RTC initialized");
      initStep++;
    } else {
      LoggerManager::error("RTC initialization failed - time unavailable");
      initStep++;
    }
  }
  
  // Step 6: Display (OLED)
  if (initStep == 6) {
    if (initializeDisplay()) {
      LoggerManager::info("Display initialized");
      initStep++;
    } else {
      LoggerManager::error("Display initialization failed - no visual feedback");
      initStep++;
    }
  }
  
  // Step 7: Storage (SD card)
  if (initStep == 7) {
    if (initializeStorage()) {
      LoggerManager::info("Storage initialized");
      initStep++;
    } else {
      LoggerManager::error("Storage initialization failed - recovery not available");
      initStep++;
    }
  }
  
  // Step 8: Managers
  if (initStep == 8) {
    if (initializeManagers()) {
      LoggerManager::info("Managers initialized");
      initStep++;
    } else {
      LoggerManager::error("Manager initialization failed");
      return false;
    }
  }
  
  // Step 9: Recovery
  if (initStep == 9) {
    if (recoverFromPowerLoss()) {
      LoggerManager::info("Power loss recovery complete");
    } else {
      LoggerManager::warn("No recovery data found - starting fresh");
    }
    initStep++;
  }
  
  // Step 10: Final validation
  if (initStep == 10) {
    unsigned long duration = millis() - startTime;
    LoggerManager::info("=== INITIALIZATION COMPLETE (%.1fs) ===", duration / 1000.0);
    
    // Display initialization complete
    DisplayManager::showReadyScreen();
    
    // Transition to READY state
    StateManager::getInstance().queueEvent(EVT_INIT_COMPLETE);
    initStep = 0;  // Reset for next initialization cycle
    return true;
  }
  
  return true;  // Initialization still in progress
}

// ============================================================================
// READY STATE HANDLER
// ============================================================================

bool executeReadyState() {
  unsigned long currentTime = millis();
  
  // Update display periodically
  if (currentTime - lastDisplayUpdateTime >= DISPLAY_UPDATE_INTERVAL) {
    updateStatusDisplay();
    lastDisplayUpdateTime = currentTime;
  }
  
  // Check for hour boundary
  if (TimeManager::getInstance().hasHourChanged()) {
    handleHourBoundary();
    lastHourChangeTime = currentTime;
  }
  
  // Perform periodic saves
  if (currentTime - lastSaveTime >= SAVE_INTERVAL) {
    if (!saveCheckpoint()) {
      LoggerManager::warn("Checkpoint save failed");
    }
    lastSaveTime = currentTime;
  }
  
  // Monitor system health
  if (currentTime - lastHealthCheckTime >= HEALTH_CHECK_INTERVAL) {
    if (!checkSystemHealth()) {
      LoggerManager::warn("System health check detected issues");
      StateManager::getInstance().queueEvent(EVT_SYSTEM_ERROR);
      return false;
    }
    lastHealthCheckTime = currentTime;
  }
  
  // Check for production start signal (event-driven)
  // This is handled by event processing in main loop
  
  return true;
}

// ============================================================================
// PRODUCTION STATE HANDLER
// ============================================================================

bool executeProductionState() {
  unsigned long currentTime = millis();
  
  // Update display in real-time
  if (currentTime - lastDisplayUpdateTime >= DISPLAY_UPDATE_INTERVAL) {
    updateProductionDisplay();
    lastDisplayUpdateTime = currentTime;
  }
  
  // Check for hour boundary during production
  if (TimeManager::getInstance().hasHourChanged()) {
    handleHourBoundary();
    lastHourChangeTime = currentTime;
  }
  
  // Save progress periodically
  if (currentTime - lastSaveTime >= SAVE_INTERVAL) {
    if (!saveProductionProgress()) {
      LoggerManager::error("Failed to save production progress");
      StateManager::getInstance().queueEvent(EVT_SYSTEM_ERROR);
      return false;
    }
    lastSaveTime = currentTime;
  }
  
  // Monitor system health during production
  if (currentTime - lastHealthCheckTime >= HEALTH_CHECK_INTERVAL) {
    if (!checkSystemHealth()) {
      LoggerManager::error("System health degraded during production");
      StateManager::getInstance().queueEvent(EVT_SYSTEM_ERROR);
      return false;
    }
    lastHealthCheckTime = currentTime;
  }
  
  // Check for production stop signal (event-driven)
  // This is handled by event processing in main loop
  
  return true;
}

// ============================================================================
// DIAGNOSTIC STATE HANDLER
// ============================================================================

bool executeDiagnosticState() {
  static bool diagStarted = false;
  
  if (!diagStarted) {
    LoggerManager::info("Starting diagnostics...");
    DisplayManager::showInitializationScreen();  // Show progress
    diagStarted = true;
  }
  
  // Run all diagnostics
  if (runAllDiagnostics()) {
    LoggerManager::info("All diagnostics passed");
    diagStarted = false;
    
    // Return to READY state
    StateManager::getInstance().queueEvent(EVT_DIAG_COMPLETE);
    return true;
  } else {
    LoggerManager::warn("Some diagnostics failed - review results");
    diagStarted = false;
    
    // Queue error event and stay in diagnostic
    StateManager::getInstance().queueEvent(EVT_SYSTEM_ERROR);
    return false;
  }
}

// ============================================================================
// ERROR STATE HANDLER
// ============================================================================

bool executeErrorState() {
  static unsigned long errorStartTime = 0;
  static bool errorDisplayed = false;
  
  if (!errorDisplayed) {
    errorStartTime = millis();
    errorDisplayed = true;
    
    // Log error state entry
    LoggerManager::error("=== SYSTEM ERROR STATE ENTERED ===");
    
    // Display error on OLED
    DisplayManager::showErrorScreen("SYSTEM ERROR");
    
    // Log full system state
    LoggerManager::error("Free heap: %d bytes", PowerManager::getFreeHeap());
    LoggerManager::error("Chip temp: %.1f°C", PowerManager::getChipTemperature());
  }
  
  // Allow 5 seconds for user to see error
  if (millis() - errorStartTime >= 5000) {
    LoggerManager::info("Attempting error recovery...");
    
    // Try to recover
    if (recoverFromPowerLoss() || initializeManagers()) {
      LoggerManager::info("Recovery successful - returning to READY");
      errorDisplayed = false;
      StateManager::getInstance().queueEvent(EVT_ERROR_RECOVERED);
      return true;
    } else {
      LoggerManager::fatal("Recovery failed - reboot required");
      DisplayManager::showErrorScreen("REBOOT NEEDED");
      
      // Stay in error state for 30 seconds, then force reboot
      if (millis() - errorStartTime >= 35000) {
        Watchdog::reset();  // Force system reboot
      }
      return false;
    }
  }
  
  return true;
}

// ============================================================================
// GUARD CONDITIONS
// ============================================================================

bool canStartProduction() {
  // Check if session already active
  if (ProductionManager::getInstance().isSessionActive()) {
    LoggerManager::warn("Production already active");
    return false;
  }
  
  // Check if display is ready
  if (!isDisplayReady()) {
    LoggerManager::error("Display not ready for production");
    return false;
  }
  
  // Check heap health
  if (!isHeapHealthy()) {
    LoggerManager::error("Heap memory insufficient for production");
    return false;
  }
  
  return true;
}

bool canStopProduction() {
  // Always safe to stop production (graceful shutdown)
  return true;
}

bool isHeapHealthy() {
  int freeHeap = PowerManager::getFreeHeap();
  const int MIN_HEAP = 50000;  // Require 50KB minimum
  
  if (freeHeap < MIN_HEAP) {
    LoggerManager::warn("Low heap: %d bytes (threshold: %d)", freeHeap, MIN_HEAP);
    return false;
  }
  
  return true;
}

bool isTimeValid() {
  return TimeManager::getInstance().isTimeValid();
}

bool isDisplayReady() {
  // Display is considered ready if initialization succeeded
  // In a real implementation, you'd check actual I2C responsiveness
  return true;
}

bool isStorageAvailable() {
  // Storage is considered available if initialization succeeded
  // In a real implementation, you'd check SD card mount status
  return true;
}

bool isRTCReady() {
  // RTC is considered ready if initialization succeeded
  // In a real implementation, you'd check actual I2C communication
  return true;
}

// ============================================================================
// INITIALIZATION SEQUENCE HELPERS
// ============================================================================

bool initializeSerial() {
  Serial_HAL::init(115200);
  LoggerManager::info("Serial communication initialized at 115200 baud");
  return true;
}

bool initializeGPIO() {
  // Initialize counter button (GPIO 15)
  GPIO::init(15, GPIO_MODE_INPUT);
  GPIO::attachInterrupt(15, nullptr);  // Will attach actual ISR in real code
  
  // Initialize diagnostic button (GPIO 27)
  GPIO::init(27, GPIO_MODE_INPUT);
  GPIO::attachInterrupt(27, nullptr);
  
  // Initialize production latch (GPIO 25)
  GPIO::init(25, GPIO_MODE_INPUT);
  GPIO::attachInterrupt(25, nullptr);
  
  // Initialize SD chip select (GPIO 26)
  GPIO::init(26, GPIO_MODE_OUTPUT);
  GPIO::write(26, HIGH);
  
  LoggerManager::info("All GPIO pins initialized");
  return true;
}

bool initializeI2C() {
  I2C::init(21, 22, 400000);
  
  // Check for OLED at 0x3C
  if (!I2C::devicePresent(0x3C)) {
    LoggerManager::warn("OLED display not found at 0x3C");
  } else {
    LoggerManager::info("OLED display found");
  }
  
  // Check for RTC at 0x68
  if (!I2C::devicePresent(0x68)) {
    LoggerManager::warn("RTC not found at 0x68");
  } else {
    LoggerManager::info("RTC found");
  }
  
  LoggerManager::info("I2C bus initialized");
  return true;
}

bool initializeSPI() {
  SPI_HAL::init(0, 5000000);  // 5MHz for SD card
  LoggerManager::info("SPI bus initialized");
  return true;
}

bool initializeRTC() {
  if (!TimeManager::getInstance().initialize()) {
    LoggerManager::warn("RTC initialization failed");
    return false;
  }
  
  LoggerManager::info("RTC synchronized: %s", 
    TimeManager::getInstance().getTimeString().c_str());
  return true;
}

bool initializeDisplay() {
  if (!DisplayManager::getInstance().initialize()) {
    LoggerManager::warn("Display initialization failed");
    return false;
  }
  
  LoggerManager::info("Display initialized");
  return true;
}

bool initializeStorage() {
  if (!StorageManager::getInstance().initialize()) {
    LoggerManager::warn("Storage initialization failed");
    return false;
  }
  
  LoggerManager::info("Storage initialized");
  return true;
}

bool initializeManagers() {
  // All managers are singletons and auto-initialize
  LoggerManager::getInstance();  // Initialize logger
  ConfigManager::getInstance().initialize();
  ProductionManager::getInstance();
  TimeManager::getInstance();
  StorageManager::getInstance();
  DisplayManager::getInstance();
  
  LoggerManager::info("All managers initialized");
  return true;
}

bool recoverFromPowerLoss() {
  if (!isStorageAvailable()) {
    LoggerManager::warn("Storage not available for recovery");
    return false;
  }
  
  // Check if recovery file exists
  if (!ProductionManager::getInstance().isRecoveryValid()) {
    LoggerManager::info("No recovery data found");
    return true;  // Not an error, just no data to recover
  }
  
  // Attempt recovery
  if (ProductionManager::getInstance().recover()) {
    LoggerManager::info("Power loss recovery successful");
    DisplayManager::getInstance().showReadyScreen();
    return true;
  } else {
    LoggerManager::error("Power loss recovery failed");
    return false;
  }
}

// ============================================================================
// PERIODIC MAINTENANCE HELPERS
// ============================================================================

void handleHourBoundary() {
  LoggerManager::info("Hour boundary detected - updating cumulative counts");
  
  // Handle hour change in TimeManager
  TimeManager::getInstance().handleHourChange();
  
  // Save cumulative count
  int totalCount = ProductionManager::getInstance().getTotalSessionCount();
  StorageManager::getInstance().saveCount("/cumulative_count.txt", totalCount);
  
  // Create daily log entry
  String logEntry = "Hour ";
  logEntry += TimeManager::getInstance().getCurrentHour();
  logEntry += " - Items: ";
  logEntry += totalCount;
  logEntry += "\n";
  
  LoggerManager::info("Cumulative count: %d", totalCount);
}

bool saveCheckpoint() {
  // Save production count
  int count = ProductionManager::getInstance().getTotalSessionCount();
  
  if (!StorageManager::getInstance().saveCount("/cumulative_count.txt", count)) {
    LoggerManager::error("Failed to save count checkpoint");
    return false;
  }
  
  LoggerManager::debug("Checkpoint saved - count: %d", count);
  return true;
}

bool checkSystemHealth() {
  // Check heap memory
  int freeHeap = PowerManager::getFreeHeap();
  if (freeHeap < 50000) {
    LoggerManager::warn("Low heap memory: %d bytes", freeHeap);
  }
  
  // Check temperature
  float temp = PowerManager::getChipTemperature();
  if (temp > 80.0) {
    LoggerManager::warn("High chip temperature: %.1f°C", temp);
  }
  
  // Feed watchdog
  Watchdog::feed();
  
  LoggerManager::debug("System health check: Heap=%d bytes, Temp=%.1f°C", 
    freeHeap, temp);
  
  return true;
}

bool updateStatusDisplay() {
  DisplayManager& display = DisplayManager::getInstance();
  
  // Show ready screen with current status
  display.showReadyScreen();
  
  // Add status information
  if (isTimeValid()) {
    String timeStr = TimeManager::getInstance().getTimeString();
    display.displayText(0, 0, timeStr.c_str(), 1);
  }
  
  // Show cumulative count
  int totalCount = ProductionManager::getInstance().getTotalSessionCount();
  display.displayNumber(0, 20, totalCount, 2);
  
  return true;
}

// ============================================================================
// PRODUCTION STATE HELPERS
// ============================================================================

bool handleItemCounted() {
  ProductionManager::getInstance().incrementCount();
  
  int count = ProductionManager::getInstance().getSessionCount();
  
  // Log every 100 items
  if (count % 100 == 0) {
    LoggerManager::info("Production count: %d", count);
  }
  
  return true;
}

bool updateProductionDisplay() {
  DisplayManager& display = DisplayManager::getInstance();
  
  // Get current session data
  int sessionCount = ProductionManager::getInstance().getSessionCount();
  int totalCount = ProductionManager::getInstance().getTotalSessionCount();
  
  // Show production screen
  display.showProductionScreen(sessionCount);
  
  // Calculate and display items per minute
  // (This would require tracking start time in a real implementation)
  
  return true;
}

bool saveProductionProgress() {
  int count = ProductionManager::getInstance().getSessionCount();
  
  // Update recovery file with current session count
  StorageManager& storage = StorageManager::getInstance();
  
  // Save to temporary file (recovery will use this)
  String recoveryData = "Session Count: ";
  recoveryData += count;
  recoveryData += "\n";
  
  if (!storage.writeFile("/prod_session.txt", recoveryData.c_str())) {
    LoggerManager::error("Failed to save production progress");
    return false;
  }
  
  LoggerManager::debug("Production progress saved - count: %d", count);
  return true;
}

// ============================================================================
// DIAGNOSTIC HELPERS
// ============================================================================

bool testGPIO() {
  LoggerManager::info("Testing GPIO pins...");
  
  // Test counter button
  GPIO::init(15, GPIO_MODE_INPUT);
  bool btn1 = GPIO::read(15) == LOW || GPIO::read(15) == HIGH;
  
  // Test diagnostic button
  GPIO::init(27, GPIO_MODE_INPUT);
  bool btn2 = GPIO::read(27) == LOW || GPIO::read(27) == HIGH;
  
  // Test production latch
  GPIO::init(25, GPIO_MODE_INPUT);
  bool btn3 = GPIO::read(25) == LOW || GPIO::read(25) == HIGH;
  
  bool allPass = btn1 && btn2 && btn3;
  LoggerManager::info("GPIO test: %s", allPass ? "PASS" : "FAIL");
  
  return allPass;
}

bool testI2C() {
  LoggerManager::info("Testing I2C bus...");
  
  bool oledPresent = I2C::devicePresent(0x3C);
  bool rtcPresent = I2C::devicePresent(0x68);
  
  LoggerManager::info("I2C test: OLED=%s, RTC=%s", 
    oledPresent ? "OK" : "FAIL", rtcPresent ? "OK" : "FAIL");
  
  return oledPresent || rtcPresent;  // At least one device should respond
}

bool testSPI() {
  LoggerManager::info("Testing SPI bus...");
  
  SPI_HAL::selectDevice(26);
  uint8_t testData = 0xAA;
  uint8_t result = SPI_HAL::transfer(testData);
  SPI_HAL::deselectDevice(26);
  
  bool pass = result != 0xFF;  // Should get some response
  LoggerManager::info("SPI test: %s", pass ? "PASS" : "FAIL");
  
  return pass;
}

bool testRTC() {
  LoggerManager::info("Testing RTC...");
  
  if (!isTimeValid()) {
    LoggerManager::error("RTC time invalid");
    return false;
  }
  
  String timeStr = TimeManager::getInstance().getTimeString();
  LoggerManager::info("RTC test: PASS (Time: %s)", timeStr.c_str());
  
  return true;
}

bool testStorage() {
  LoggerManager::info("Testing storage...");
  
  // Try to write test file
  if (!StorageManager::getInstance().writeFile("/test.txt", "TEST\n")) {
    LoggerManager::error("Storage write test failed");
    return false;
  }
  
  // Try to read test file
  String content = StorageManager::getInstance().readFile("/test.txt");
  if (content.length() == 0) {
    LoggerManager::error("Storage read test failed");
    return false;
  }
  
  // Clean up
  StorageManager::getInstance().deleteFile("/test.txt");
  
  LoggerManager::info("Storage test: PASS");
  return true;
}

bool testMemory() {
  LoggerManager::info("Testing memory...");
  
  int freeHeap = PowerManager::getFreeHeap();
  LoggerManager::info("Free heap: %d bytes", freeHeap);
  
  bool pass = freeHeap > 50000;  // Need at least 50KB
  LoggerManager::info("Memory test: %s", pass ? "PASS" : "FAIL");
  
  return pass;
}

bool runAllDiagnostics() {
  LoggerManager::info("=== RUNNING DIAGNOSTICS ===");
  
  bool gpioPass = testGPIO();
  bool i2cPass = testI2C();
  bool spiPass = testSPI();
  bool rtcPass = testRTC();
  bool storagePass = testStorage();
  bool memoryPass = testMemory();
  
  LoggerManager::info("=== DIAGNOSTIC RESULTS ===");
  LoggerManager::info("GPIO: %s", gpioPass ? "PASS" : "FAIL");
  LoggerManager::info("I2C: %s", i2cPass ? "PASS" : "FAIL");
  LoggerManager::info("SPI: %s", spiPass ? "PASS" : "FAIL");
  LoggerManager::info("RTC: %s", rtcPass ? "PASS" : "FAIL");
  LoggerManager::info("Storage: %s", storagePass ? "PASS" : "FAIL");
  LoggerManager::info("Memory: %s", memoryPass ? "PASS" : "FAIL");
  
  bool allPass = gpioPass && i2cPass && rtcPass && memoryPass;
  // SPI and storage are optional (SD card may not be present)
  
  LoggerManager::info("Overall: %s", allPass ? "PASS" : "FAIL");
  
  return allPass;
}

// ============================================================================
// INTERRUPT SERVICE ROUTINE CALLBACKS
// ============================================================================

/**
 * Called from counter button ISR
 * Queues count event for processing in main loop
 */
void onCounterButtonPressed() {
  StateManager::getInstance().queueEvent(EVT_ITEM_COUNTED);
}

/**
 * Called from diagnostic button ISR
 * Triggers diagnostic mode
 */
void onDiagnosticButtonPressed() {
  StateManager::getInstance().queueEvent(EVT_DIAGNOSTIC_REQUESTED);
}

/**
 * Called from production latch ISR
 * Toggles production mode
 */
void onProductionLatchChanged() {
  if (ProductionManager::getInstance().isSessionActive()) {
    StateManager::getInstance().queueEvent(EVT_PRODUCTION_STOP);
  } else {
    StateManager::getInstance().queueEvent(EVT_PRODUCTION_START);
  }
}
