# Professional Embedded Systems Development Best Practices

## For Your Production Counter Firmware

---

## 1. Code Organization & Architecture

### Current Project Structure
```
Firmware v2.02/
├── code_v3.cpp              (Main firmware - 2100+ lines)
├── README.md                (System overview)
├── SYSTEM_USAGE_GUIDE.md    (User documentation)
├── FSM_ARCHITECTURE.md      (Design patterns)
├── FSM_REFACTORING_ROADMAP.md (Implementation plan)
├── COUNT_UPDATE_FLOW.md     (Data flow documentation)
└── [SD Card] Files created at runtime
```

### Recommended Professional Structure
```
esp32-production-counter/
├── firmware/
│   ├── src/
│   │   ├── main.cpp                 (Entry point)
│   │   ├── states/
│   │   │   ├── state_manager.h      (FSM core)
│   │   │   ├── state_ready.cpp      (Ready state logic)
│   │   │   ├── state_running.cpp    (Running state)
│   │   │   └── state_error.cpp      (Error handling)
│   │   ├── managers/
│   │   │   ├── production_manager.h
│   │   │   ├── time_manager.h
│   │   │   ├── storage_manager.h
│   │   │   └── display_manager.h
│   │   ├── drivers/
│   │   │   ├── oled_driver.h
│   │   │   ├── rtc_driver.h
│   │   │   └── sd_driver.h
│   │   ├── hal/
│   │   │   ├── gpio.h               (GPIO abstraction)
│   │   │   ├── timer.h              (Timer abstraction)
│   │   │   └── uart.h               (Serial abstraction)
│   │   ├── config/
│   │   │   ├── pinconfig.h          (All pin definitions)
│   │   │   ├── hwconfig.h           (Hardware settings)
│   │   │   └── fwconfig.h           (Firmware settings)
│   │   ├── utils/
│   │   │   ├── logger.h             (Logging system)
│   │   │   ├── crc.h                (Data validation)
│   │   │   └── utils.h              (Common utilities)
│   │   └── isr/
│   │       └── interrupts.cpp       (All ISRs)
│   ├── platformio.ini               (Build configuration)
│   └── CMakeLists.txt               (Alternative build)
├── docs/
│   ├── ARCHITECTURE.md              (Design documents)
│   ├── FSM_DESIGN.md                (State machine details)
│   ├── API_REFERENCE.md             (Class/function docs)
│   └── HARDWARE_GUIDE.md            (Electrical specs)
├── test/
│   ├── unit/
│   │   ├── test_state_manager.cpp
│   │   ├── test_production_mgr.cpp
│   │   └── test_storage_mgr.cpp
│   ├── integration/
│   │   ├── test_fsm_transitions.cpp
│   │   └── test_data_flow.cpp
│   └── CMakeLists.txt
├── tools/
│   ├── sdcard_simulator/            (PC tool for testing)
│   ├── log_analyzer/                (Parse device logs)
│   └── data_visualizer/             (Chart production data)
├── .gitignore
├── README.md                        (Project overview)
├── CHANGELOG.md                     (Version history)
└── LICENSE

```

---

## 2. Professional Code Patterns

### A. Hardware Abstraction Layer (HAL)

```cpp
// hal/gpio.h
#pragma once

class GPIO {
public:
  enum Pin {
    COUNTER_BTN = 15,
    DIAG_BTN = 27,
    LATCH_BTN = 25,
    SD_CS = 26,
  };
  
  enum Mode {
    INPUT_PULLUP,
    OUTPUT,
    INPUT,
  };
  
  static void init(Pin pin, Mode mode);
  static bool read(Pin pin);
  static void write(Pin pin, bool value);
  static void attachInterrupt(Pin pin, void(*handler)(), int mode);
};
```

### B. Logging System (Essential for Debugging)

```cpp
// utils/logger.h
#pragma once

enum LogLevel {
  LOG_DEBUG = 0,
  LOG_INFO = 1,
  LOG_WARN = 2,
  LOG_ERROR = 3,
  LOG_FATAL = 4,
};

class Logger {
private:
  static LogLevel minLevel;
  static bool sdLogging;
  
public:
  static void init(LogLevel level = LOG_INFO, bool logToSD = true);
  
  static void debug(const char* format, ...);
  static void info(const char* format, ...);
  static void warn(const char* format, ...);
  static void error(const char* format, ...);
  static void fatal(const char* format, ...);
  
private:
  static void log(LogLevel level, const char* format, va_list args);
};

// Usage:
Logger::info("Production started at %02d:%02d:%02d", h, m, s);
Logger::error("SD card error: %d", errorCode);
Logger::debug("Count: %d, Cumulative: %d", currentCount, cumulativeCount);
```

### C. Error Handling Pattern

```cpp
// Common result type for operations
enum OperationResult {
  RESULT_OK,
  RESULT_ERROR_SD_CARD,
  RESULT_ERROR_RTC,
  RESULT_ERROR_INVALID_PARAM,
  RESULT_ERROR_TIMEOUT,
  RESULT_ERROR_MEMORY,
};

// Use Result type instead of void
OperationResult saveCount(const char* filename, int value) {
  if (!sdAvailable) {
    Logger::error("SD card not available");
    return RESULT_ERROR_SD_CARD;
  }
  
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Logger::error("Failed to open: %s", filename);
    return RESULT_ERROR_SD_CARD;
  }
  
  file.println(value);
  bool success = file.flush();
  file.close();
  
  if (!success) {
    Logger::warn("Failed to flush file");
    return RESULT_ERROR_SD_CARD;
  }
  
  Logger::debug("Saved to %s: %d", filename, value);
  return RESULT_OK;
}
```

### D. Configuration Management

```cpp
// config/settings.h
#pragma once

struct SystemSettings {
  // Timing
  uint32_t saveInterval = 5000;        // ms
  uint32_t debounceDelay = 50;         // ms
  uint32_t displayRefreshRate = 100;   // ms
  uint32_t statusDisplayDuration = 3000; // ms
  
  // Limits
  uint16_t maxCount = 9999;
  uint8_t maxStartupRetries = 3;
  uint32_t sdInitTimeout = 5000;       // ms
  
  // Features
  bool enableSDLogging = true;
  bool enablePowerRecovery = true;
  bool enableDiagnostics = true;
  
  // Validate settings
  bool isValid() const {
    return saveInterval >= 1000 && saveInterval <= 60000 &&
           debounceDelay >= 10 && debounceDelay <= 500 &&
           maxCount >= 100 && maxCount <= 99999;
  }
};
```

---

## 3. Testing Strategy

### Unit Testing Example

```cpp
// test/unit/test_production_mgr.cpp
#include <gtest/gtest.h>
#include "managers/production_manager.h"

class ProductionManagerTest : public ::testing::Test {
protected:
  ProductionManager manager;
  
  void SetUp() override {
    manager.reset();
  }
};

TEST_F(ProductionManagerTest, StartSessionInitializesCorrectly) {
  manager.startSession();
  
  EXPECT_TRUE(manager.isSessionActive());
  EXPECT_EQ(manager.getSessionCount(), 0);
  EXPECT_NE(manager.getStartTime(), 0);
}

TEST_F(ProductionManagerTest, CountIncrementsCorrectly) {
  manager.startSession();
  
  for (int i = 0; i < 100; i++) {
    manager.incrementCount();
  }
  
  EXPECT_EQ(manager.getSessionCount(), 100);
}

TEST_F(ProductionManagerTest, StopSessionCalculatesCorrectly) {
  manager.startSession();
  
  for (int i = 0; i < 50; i++) {
    manager.incrementCount();
  }
  
  manager.stopSession();
  
  EXPECT_FALSE(manager.isSessionActive());
  EXPECT_EQ(manager.getSessionCount(), 50);
}
```

### Integration Testing

```cpp
// test/integration/test_fsm_flow.cpp
TEST(FSMIntegration, CompleteProductionSession) {
  // Simulate complete workflow
  system.startup();
  EXPECT_TRUE(system.isReady());
  
  // Start production
  system.pressLatchButton();
  EXPECT_TRUE(system.isProducing());
  
  // Count some items
  for (int i = 0; i < 100; i++) {
    system.pressCounterButton();
  }
  EXPECT_EQ(system.getCurrentCount(), 100);
  
  // Hour change
  system.simulateHourChange();
  EXPECT_EQ(system.getCumulativeCount(), 100);
  
  // Stop production
  system.releaseLatchButton();
  EXPECT_FALSE(system.isProducing());
  
  // Verify files saved
  EXPECT_TRUE(fs.fileExists(PRODUCTION_FILE));
  EXPECT_TRUE(fs.fileExists(DAILY_LOG_FILE));
}
```

---

## 4. Performance Optimization

### Memory Profiling

```cpp
// Get heap statistics
uint32_t getFreeHeap() {
  return ESP.getFreeHeap();
}

uint32_t getLargestFreeBlock() {
  return ESP.getMaxAllocHeap();
}

// Log memory usage periodically
void monitorMemory() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 60000) { // Every minute
    Logger::info("Heap: %d bytes free (largest block: %d)",
                 getFreeHeap(), getLargestFreeBlock());
    lastCheck = millis();
  }
}
```

### Stack Usage

```cpp
// Check stack in ISRs
void IRAM_ATTR handleInterrupt() {
  // ISR code here
  // Keep ISRs minimal and fast
  
  // Minimum stack check
  if (uxTaskGetStackHighWaterMark(NULL) < 256) {
    Logger::warn("ISR stack low!");
  }
}
```

### CPU Usage

```cpp
// Measure loop frequency
void measureLoopFrequency() {
  static unsigned long lastMeasure = 0;
  static int loopCount = 0;
  
  loopCount++;
  unsigned long now = millis();
  
  if (now - lastMeasure >= 1000) {
    Logger::info("Loop frequency: %d Hz", loopCount);
    loopCount = 0;
    lastMeasure = now;
  }
}
```

---

## 5. Version Control & CI/CD

### Git Workflow

```bash
# Create feature branch
git checkout -b feature/fsm-refactoring

# Commit with clear messages
git commit -m "refactor: implement StateManager class with guard conditions"

# Keep commits atomic and logical
git commit -m "test: add unit tests for ProductionManager"

# Merge with squash for clean history
git checkout main
git merge --squash feature/fsm-refactoring
git commit -m "refactor: FSM-based architecture implementation"
```

### Semantic Versioning

```
Current: v2.02 (Major.Minor.Patch)

v2.02 → v2.03 (Bug fixes, minor improvements)
v2.02 → v2.10 (New features, backward compatible)
v2.02 → v3.00 (Breaking changes, major refactor)

Changes log in CHANGELOG.md:
- Feature: Added power loss recovery
- Fix: Corrected hour file naming
- Refactor: Implemented FSM architecture
- Docs: Updated API documentation
```

---

## 6. Documentation Standards

### Code Comments

```cpp
// BAD: Obvious comment
int count = 0;  // Initialize count

// GOOD: Explains WHY, not WHAT
// Reset count at hour boundary to track hourly production
int count = 0;

// GOOD: Complex logic explanation
// countThisHour = (current - baseline) prevents double-counting
// when hour changes during active production session
int countThisHour = currentCount - countAtHourStart;
```

### Function Documentation

```cpp
/**
 * Save production session to SD card
 * 
 * Creates a timestamped production file with format:
 * Production_YYYY-MM-DD_HHhMMm-HHhMMm.txt
 * 
 * @param[in] startTime  Session start datetime
 * @param[in] stopTime   Session stop datetime  
 * @param[in] count      Total units counted
 * 
 * @return OperationResult::RESULT_OK on success
 *         OperationResult::RESULT_ERROR_SD_CARD on failure
 * 
 * @note Overwrites existing file with same timestamp
 * @note Requires sdAvailable == true
 * 
 * @see saveCount()
 * @see updateDailyLog()
 */
OperationResult saveProductionSession(DateTime startTime, 
                                      DateTime stopTime, 
                                      int count);
```

---

## 7. Safety & Reliability

### Watchdog Timer

```cpp
// Configure watchdog to detect hangs
void initWatchdog() {
  esp_task_wdt_init(30, true); // 30 second timeout
  esp_task_wdt_add(NULL);      // Subscribe current task
}

// Feed watchdog in critical loops
void mainLoop() {
  while (true) {
    esp_task_wdt_reset(); // Reset watchdog timer
    
    // Main logic here
    handleEvents();
    updateState();
    executeState();
    
    delay(10);
  }
}
```

### Assertion for Development

```cpp
// Use assertions to catch logic errors early
#define ASSERT(condition, message) \
  do { \
    if (!(condition)) { \
      Logger::fatal("ASSERT FAILED: %s at %s:%d", \
                    message, __FILE__, __LINE__); \
      while(1); /* Hang for debugging */ \
    } \
  } while(0)

// Usage
ASSERT(rtcAvailable, "RTC must be available");
ASSERT(currentCount >= 0 && currentCount <= MAX_COUNT, 
       "Count out of range");
```

---

## 8. Manufacturing & Deployment

### Firmware Versioning

```cpp
// Define in config/version.h
#define FIRMWARE_VERSION_MAJOR 2
#define FIRMWARE_VERSION_MINOR 3
#define FIRMWARE_VERSION_PATCH 0
#define FIRMWARE_BUILD_NUMBER 1234
#define FIRMWARE_BUILD_DATE "2025-11-15"

// Stringify helper
#define STR(x) #x
#define XSTR(x) STR(x)

const char* firmwareVersion = 
  XSTR(FIRMWARE_VERSION_MAJOR) "." 
  XSTR(FIRMWARE_VERSION_MINOR) "." 
  XSTR(FIRMWARE_VERSION_PATCH);
```

### Production Checklist

```
□ Code review completed
□ All unit tests passing
□ All integration tests passing
□ Memory profiling completed
□ Stack usage verified
□ CPU usage optimized
□ Documentation updated
□ Changelog updated
□ No compiler warnings
□ No static analysis warnings
□ Code formatted consistently
□ Git history clean
□ Release notes written
□ Hardware tested
□ User documentation updated
```

---

## 9. Maintenance & Support

### Diagnostic Interface

```cpp
// Serial command for detailed diagnostics
void executeDiagnostics() {
  Serial.println("\n=== SYSTEM DIAGNOSTICS ===");
  
  // Hardware status
  Serial.print("OLED: ");
  Serial.println(rtcAvailable ? "✓" : "✗");
  
  Serial.print("RTC: ");
  Serial.println(rtcAvailable ? "✓" : "✗");
  
  Serial.print("SD Card: ");
  Serial.println(sdAvailable ? "✓" : "✗");
  
  // Memory status
  Serial.print("Heap Free: ");
  Serial.print(getFreeHeap());
  Serial.println(" bytes");
  
  // Timing status
  Serial.print("Loop Frequency: ");
  Serial.print(loopFrequency);
  Serial.println(" Hz");
  
  // Production status
  Serial.print("Session Active: ");
  Serial.println(productionActive ? "YES" : "NO");
  
  Serial.print("Current Count: ");
  Serial.println(currentCount);
  
  // File system status
  Serial.print("Files on SD: ");
  Serial.println(countFilesOnSD());
}
```

### Remote Monitoring (Future)

```cpp
// Template for WiFi logging (Phase 2)
class RemoteLogger {
private:
  WiFiClient client;
  const char* serverIP;
  uint16_t serverPort;
  
public:
  void logEvent(const char* eventType, const char* data) {
    if (!client.connected()) {
      if (!connect()) {
        Logger::warn("Cannot connect to remote server");
        return;
      }
    }
    
    // Send JSON formatted data
    // {
    //   "type": "production_count",
    //   "timestamp": 1234567890,
    //   "value": 123,
    //   "device_id": "ESP32-001"
    // }
  }
};
```

---

## 10. Checklist for Professional Production Code

- [ ] **Architecture**: FSM-based with clear state transitions
- [ ] **Code Organization**: Modular, layered, well-structured
- [ ] **Error Handling**: Comprehensive with recovery paths
- [ ] **Testing**: Unit tests, integration tests, error cases
- [ ] **Documentation**: API docs, design docs, user guides
- [ ] **Logging**: Detailed diagnostic output
- [ ] **Configuration**: Externalized, validated settings
- [ ] **Performance**: Optimized memory and CPU usage
- [ ] **Safety**: Watchdog, assertions, bounds checking
- [ ] **Version Control**: Clean history, tagged releases
- [ ] **Deployment**: Reproducible builds, versioning
- [ ] **Monitoring**: Health checks, telemetry
- [ ] **Maintenance**: Diagnostic tools, support docs

---

## Next Actions

### Immediate (Week 1)
1. ✅ Review current architecture (DONE)
2. Create detailed FSM diagrams
3. Plan modular refactoring
4. Set up test framework

### Short Term (Weeks 2-4)
1. Implement StateManager class
2. Create modular subsystems
3. Write comprehensive tests
4. Refactor main loop

### Medium Term (Month 2-3)
1. Optimize performance
2. Add remote monitoring
3. Create manufacturing tools
4. Full documentation

### Long Term
1. Version 3.0 with advanced features
2. Multi-device coordination
3. Cloud dashboard integration
4. Predictive maintenance

---

**I'm ready to implement any of these improvements. Which would you like to start with?**
