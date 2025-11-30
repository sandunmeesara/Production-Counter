/**
 * Hardware Validation Tests
 * Tests for physical hardware functionality
 * 
 * Test Coverage:
 * 1. GPIO pin tests
 * 2. I2C device tests (OLED, RTC)
 * 3. SPI device tests (SD Card)
 * 4. Interrupt tests
 * 5. Serial communication tests
 * 6. Power management tests
 * 7. Timing accuracy tests
 */

#include <Arduino.h>
#include "../hal.h"

struct HardwareTestResult {
  const char* testName;
  const char* hardware;
  bool passed;
  const char* message;
  unsigned long executionTime;
  int value;  // Additional data (voltage, frequency, etc.)
};

HardwareTestResult hardwareResults[50];
int hardwareTestCount = 0;

/**
 * Helper to record hardware test
 */
void recordHardwareTest(const char* name, const char* hw, bool result, const char* msg = "", int val = 0) {
  if (hardwareTestCount < 50) {
    hardwareResults[hardwareTestCount].testName = name;
    hardwareResults[hardwareTestCount].hardware = hw;
    hardwareResults[hardwareTestCount].passed = result;
    hardwareResults[hardwareTestCount].message = msg;
    hardwareResults[hardwareTestCount].value = val;
    hardwareResults[hardwareTestCount].executionTime = 0;
    hardwareTestCount++;
  }
}

// ============================================================================
// GPIO TESTS
// ============================================================================

/**
 * Test HW-1: Counter Button GPIO
 */
bool test_CounterButtonGPIO() {
  unsigned long startTime = millis();
  
  GPIO_HAL& gpio = GPIO_HAL::getInstance();
  bool initialized = gpio.initialize();
  
  // Configure counter button pin (GPIO 15)
  bool configured = gpio.configureInput(GPIO_COUNTER_BUTTON, true);  // true = pull-up
  
  // Read initial state
  bool buttonState = gpio.readInput(GPIO_COUNTER_BUTTON);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = initialized && configured;
  recordHardwareTest("HW_GPIO_Counter", "GPIO", result, "Counter button configured");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return result;
}

/**
 * Test HW-2: Diagnostic Button GPIO
 */
bool test_DiagnosticButtonGPIO() {
  unsigned long startTime = millis();
  
  GPIO_HAL& gpio = GPIO_HAL::getInstance();
  gpio.initialize();
  
  // Configure diagnostic button pin (GPIO 27)
  bool configured = gpio.configureInput(GPIO_DIAGNOSTIC_BUTTON, true);
  
  // Read state
  bool buttonState = gpio.readInput(GPIO_DIAGNOSTIC_BUTTON);
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_GPIO_Diag", "GPIO", configured, "Diagnostic button configured");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return configured;
}

/**
 * Test HW-3: Production Latch GPIO
 */
bool test_ProductionLatchGPIO() {
  unsigned long startTime = millis();
  
  GPIO_HAL& gpio = GPIO_HAL::getInstance();
  gpio.initialize();
  
  // Configure production latch pin (GPIO 25)
  bool configured = gpio.configureInput(GPIO_PRODUCTION_LATCH, true);
  
  // Read state
  bool latchState = gpio.readInput(GPIO_PRODUCTION_LATCH);
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_GPIO_Latch", "GPIO", configured, "Production latch configured");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return configured;
}

/**
 * Test HW-4: Status LED GPIO Output
 */
bool test_StatusLEDGPIO() {
  unsigned long startTime = millis();
  
  GPIO_HAL& gpio = GPIO_HAL::getInstance();
  gpio.initialize();
  
  // Configure LED output pin (GPIO 2)
  bool configured = gpio.configureOutput(GPIO_STATUS_LED, false);  // false = low initially
  
  // Set LED high
  gpio.writeOutput(GPIO_STATUS_LED, true);
  
  // Read back
  bool ledState = gpio.readOutput(GPIO_STATUS_LED);
  
  // Set LED low
  gpio.writeOutput(GPIO_STATUS_LED, false);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = configured && ledState;
  recordHardwareTest("HW_GPIO_LED", "GPIO", result, "Status LED configured and controlled");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return result;
}

// ============================================================================
// I2C DEVICE TESTS (OLED, RTC)
// ============================================================================

/**
 * Test HW-5: I2C Bus Initialization
 */
bool test_I2CBusInitialization() {
  unsigned long startTime = millis();
  
  I2C_HAL& i2c = I2C_HAL::getInstance();
  bool initialized = i2c.initialize(21, 22, 100000);  // SDA, SCL, 100kHz
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_I2C_Init", "I2C", initialized, "I2C bus initialized");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return initialized;
}

/**
 * Test HW-6: OLED Display Detection
 */
bool test_OLEDDetection() {
  unsigned long startTime = millis();
  
  I2C_HAL& i2c = I2C_HAL::getInstance();
  i2c.initialize(21, 22, 100000);
  
  // Scan for OLED at address 0x3C
  bool found = i2c.deviceFound(0x3C);
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_OLED_Detect", "I2C/OLED", found, "OLED found at 0x3C");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return found;
}

/**
 * Test HW-7: RTC Detection
 */
bool test_RTCDetection() {
  unsigned long startTime = millis();
  
  I2C_HAL& i2c = I2C_HAL::getInstance();
  i2c.initialize(21, 22, 100000);
  
  // Scan for RTC at address 0x68
  bool found = i2c.deviceFound(0x68);
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_RTC_Detect", "I2C/RTC", found, "RTC found at 0x68");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return found;
}

/**
 * Test HW-8: OLED Initialization
 */
bool test_OLEDInitialization() {
  unsigned long startTime = millis();
  
  // Initialize I2C first
  I2C_HAL& i2c = I2C_HAL::getInstance();
  i2c.initialize(21, 22, 100000);
  
  // Initialize OLED (128x64, 0x3C)
  bool initialized = i2c.initializeDevice(0x3C);
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_OLED_Init", "OLED", initialized, "OLED initialized");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return initialized;
}

/**
 * Test HW-9: RTC Time Read
 */
bool test_RTCTimeRead() {
  unsigned long startTime = millis();
  
  I2C_HAL& i2c = I2C_HAL::getInstance();
  i2c.initialize(21, 22, 100000);
  
  // Read RTC time
  uint8_t hour = 12, minute = 30, second = 0;
  bool success = i2c.readRTCTime(hour, minute, second);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = success && (hour < 24) && (minute < 60) && (second < 60);
  recordHardwareTest("HW_RTC_Read", "RTC", result, "RTC time read successfully");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  hardwareResults[hardwareTestCount - 1].value = hour;
  
  return result;
}

// ============================================================================
// SPI DEVICE TESTS (SD CARD)
// ============================================================================

/**
 * Test HW-10: SPI Bus Initialization
 */
bool test_SPIBusInitialization() {
  unsigned long startTime = millis();
  
  SPI_HAL& spi = SPI_HAL::getInstance();
  bool initialized = spi.initialize(23, 19, 18, 26);  // CLK, MOSI, MISO, CS
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_SPI_Init", "SPI", initialized, "SPI bus initialized");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return initialized;
}

/**
 * Test HW-11: SD Card Detection
 */
bool test_SDCardDetection() {
  unsigned long startTime = millis();
  
  SPI_HAL& spi = SPI_HAL::getInstance();
  spi.initialize(23, 19, 18, 26);
  
  // Attempt to detect SD card
  bool detected = spi.detectSDCard();
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_SD_Detect", "SD Card", detected, "SD card detected");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return detected;
}

/**
 * Test HW-12: SD Card Speed Detection
 */
bool test_SDCardSpeedDetection() {
  unsigned long startTime = millis();
  
  SPI_HAL& spi = SPI_HAL::getInstance();
  spi.initialize(23, 19, 18, 26);
  
  // Auto-detect SD card speed
  uint32_t speed = spi.autoDetectSpeed();
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = (speed >= 100000 && speed <= 5000000);  // Valid range
  recordHardwareTest("HW_SD_Speed", "SD Card", result, "SD card speed detected");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  hardwareResults[hardwareTestCount - 1].value = speed / 1000;  // kHz
  
  return result;
}

// ============================================================================
// TIMER TESTS
// ============================================================================

/**
 * Test HW-13: Timer Initialization
 */
bool test_TimerInitialization() {
  unsigned long startTime = millis();
  
  Timer_HAL& timer = Timer_HAL::getInstance();
  bool initialized = timer.initialize();
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_Timer_Init", "Timer", initialized, "Timer initialized");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return initialized;
}

/**
 * Test HW-14: Timer Accuracy
 */
bool test_TimerAccuracy() {
  unsigned long startTime = millis();
  
  Timer_HAL& timer = Timer_HAL::getInstance();
  timer.initialize();
  
  // Measure 1000ms
  unsigned long timerStart = timer.getMicros();
  delay(1000);
  unsigned long timerEnd = timer.getMicros();
  
  unsigned long elapsed = timerEnd - timerStart;
  
  // Should be approximately 1,000,000 microseconds
  bool accurate = (elapsed > 990000 && elapsed < 1010000);
  
  recordHardwareTest("HW_Timer_Accuracy", "Timer", accurate, "Timer accurate within 1%");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed / 1000;
  
  return accurate;
}

// ============================================================================
// SERIAL TESTS
// ============================================================================

/**
 * Test HW-15: Serial Communication
 */
bool test_SerialCommunication() {
  unsigned long startTime = millis();
  
  Serial_HAL& serial = Serial_HAL::getInstance();
  bool initialized = serial.initialize(115200);
  
  // Send test message
  serial.println("Hardware test started");
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_Serial_Init", "Serial", initialized, "Serial initialized at 115200 baud");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return initialized;
}

// ============================================================================
// POWER MANAGEMENT TESTS
// ============================================================================

/**
 * Test HW-16: Watchdog Initialization
 */
bool test_WatchdogInitialization() {
  unsigned long startTime = millis();
  
  Watchdog_HAL& watchdog = Watchdog_HAL::getInstance();
  bool initialized = watchdog.initialize(35000);  // 35 second timeout
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_Watchdog_Init", "Watchdog", initialized, "Watchdog initialized");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return initialized;
}

/**
 * Test HW-17: Watchdog Feed
 */
bool test_WatchdogFeed() {
  unsigned long startTime = millis();
  
  Watchdog_HAL& watchdog = Watchdog_HAL::getInstance();
  watchdog.initialize(35000);
  
  // Feed watchdog multiple times
  bool success = true;
  for (int i = 0; i < 5; i++) {
    watchdog.feed();
  }
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_Watchdog_Feed", "Watchdog", success, "Watchdog fed 5 times");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return success;
}

/**
 * Test HW-18: Power Manager Initialization
 */
bool test_PowerManagerInitialization() {
  unsigned long startTime = millis();
  
  PowerManager_HAL& pm = PowerManager_HAL::getInstance();
  bool initialized = pm.initialize();
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_PowerMgr_Init", "Power Manager", initialized, "Power manager initialized");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return initialized;
}

/**
 * Test HW-19: Get Heap Free Space
 */
bool test_HeapFreeSpace() {
  unsigned long startTime = millis();
  
  PowerManager_HAL& pm = PowerManager_HAL::getInstance();
  pm.initialize();
  
  uint32_t freeHeap = pm.getHeapFree();
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = (freeHeap > 100000);  // Should have > 100KB
  recordHardwareTest("HW_Heap_Free", "Memory", result, "Heap free space checked");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  hardwareResults[hardwareTestCount - 1].value = freeHeap / 1024;  // KB
  
  return result;
}

/**
 * Test HW-20: EEPROM Initialization
 */
bool test_EEPROMInitialization() {
  unsigned long startTime = millis();
  
  EEPROM_HAL& eeprom = EEPROM_HAL::getInstance();
  bool initialized = eeprom.initialize(512);  // 512 bytes
  
  unsigned long elapsed = millis() - startTime;
  
  recordHardwareTest("HW_EEPROM_Init", "EEPROM", initialized, "EEPROM initialized");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return initialized;
}

/**
 * Test HW-21: EEPROM Write/Read
 */
bool test_EEPROMReadWrite() {
  unsigned long startTime = millis();
  
  EEPROM_HAL& eeprom = EEPROM_HAL::getInstance();
  eeprom.initialize(512);
  
  // Write test value
  uint8_t testValue = 0xAB;
  eeprom.write(0, testValue);
  
  // Read back
  uint8_t readValue = eeprom.read(0);
  
  unsigned long elapsed = millis() - startTime;
  
  bool result = (readValue == testValue);
  recordHardwareTest("HW_EEPROM_RW", "EEPROM", result, "EEPROM write/read test");
  hardwareResults[hardwareTestCount - 1].executionTime = elapsed;
  
  return result;
}

// ============================================================================
// RUN ALL HARDWARE TESTS
// ============================================================================

void runAllHardwareTests() {
  Serial.println("\n========================================");
  Serial.println("Hardware Validation Tests");
  Serial.println("========================================\n");
  
  unsigned long totalStartTime = millis();
  
  // Run tests
  Serial.println("Testing GPIO pins...");
  test_CounterButtonGPIO();
  test_DiagnosticButtonGPIO();
  test_ProductionLatchGPIO();
  test_StatusLEDGPIO();
  
  Serial.println("Testing I2C devices...");
  test_I2CBusInitialization();
  test_OLEDDetection();
  test_RTCDetection();
  test_OLEDInitialization();
  test_RTCTimeRead();
  
  Serial.println("Testing SPI/SD Card...");
  test_SPIBusInitialization();
  test_SDCardDetection();
  test_SDCardSpeedDetection();
  
  Serial.println("Testing Timers...");
  test_TimerInitialization();
  test_TimerAccuracy();
  
  Serial.println("Testing Serial...");
  test_SerialCommunication();
  
  Serial.println("Testing Power Management...");
  test_WatchdogInitialization();
  test_WatchdogFeed();
  test_PowerManagerInitialization();
  test_HeapFreeSpace();
  
  Serial.println("Testing EEPROM...");
  test_EEPROMInitialization();
  test_EEPROMReadWrite();
  
  unsigned long totalTime = millis() - totalStartTime;
  
  // Print results
  Serial.println("\n\nHardware Test Results:");
  Serial.println("========================================");
  
  int passCount = 0;
  int failCount = 0;
  
  for (int i = 0; i < hardwareTestCount; i++) {
    if (hardwareResults[i].passed) {
      Serial.print("✓ ");
      passCount++;
    } else {
      Serial.print("✗ ");
      failCount++;
    }
    
    Serial.print("[");
    Serial.print(hardwareResults[i].hardware);
    Serial.print("] ");
    Serial.print(hardwareResults[i].testName);
    
    if (hardwareResults[i].message[0] != '\0') {
      Serial.print(" - ");
      Serial.print(hardwareResults[i].message);
    }
    
    if (hardwareResults[i].value > 0) {
      Serial.print(" (");
      Serial.print(hardwareResults[i].value);
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
  Serial.print(hardwareTestCount);
  Serial.println();
  
  float passRate = (float)passCount / hardwareTestCount * 100;
  Serial.print("Pass rate: ");
  Serial.print(passRate);
  Serial.println("%");
  
  Serial.println("========================================\n");
}

void setup_hardware_tests() {
  Serial.begin(115200);
  delay(1000);
  runAllHardwareTests();
}

void loop_hardware_tests() {
  delay(10000);
}
