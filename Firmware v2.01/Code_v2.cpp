/*
 * ESP32 Production Counter System - 16x4 LCD VERSION
 * Features: Interrupt counting, production logging, 16x4 LCD display, diagnostics
 * Libraries: LiquidCrystal_I2C, RTClib, SD, EEPROM
 * 
 * LCD Display: 16x4 characters, I2C interface (PCF8574 backpack)
 * Production Control: Latching button on GPIO 25
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <EEPROM.h>

// ========================================
// PIN DEFINITIONS
// ========================================
#define INTERRUPT_PIN 15      // Counter button (to GND)
#define DIAGNOSTIC_PIN 27     // Diagnostic button (to GND)
#define PRODUCTION_LATCH_PIN 25  // Production latching button (to GND)

// LCD Display - I2C (16x4)
#define LCD_COLS 16
#define LCD_ROWS 4
#define LCD_ADDRESS 0x27      // I2C address for PCF8574 backpack (adjust if needed: 0x3F, 0x27, etc.)

// I2C pins (default for ESP32)
#define I2C_SDA 21
#define I2C_SCL 22

// SD Card - VSPI (SPI3)
#define SD_CS_PIN 26          // SD Card Chip Select
#define SD_SCK 18             // VSPI Clock (GPIO 18)
#define SD_MISO 19            // VSPI MISO (GPIO 19)
#define SD_MOSI 23            // VSPI MOSI (GPIO 23)

// ========================================
// GLOBAL OBJECTS
// ========================================
SPIClass spiSD(VSPI);     // VSPI (SPI3) for SD Card
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
RTC_DS3231 rtc;

// ========================================
// SYSTEM STATE VARIABLES
// ========================================
enum OperatingMode { MODE_NORMAL, MODE_DIAGNOSTIC };
OperatingMode currentMode = MODE_NORMAL;

volatile int currentCount = 0;
volatile bool countChanged = false;
int hourlyCount = 0;
int cumulativeCount = 0;
int lastHour = -1;
bool rtcAvailable = false;
bool sdAvailable = false;

// Production state variables
volatile bool productionActive = false;
volatile bool productionStateChanged = false;
DateTime productionStartTime;
int productionSessionCount = 0;

unsigned long lastSaveTime = 0;
#define SAVE_INTERVAL 5000

// ========================================
// DEBOUNCING CONFIGURATION
// ========================================
#define DEBOUNCE_DELAY 50
#define MAX_COUNT 9999

// ========================================
// DISPLAY STATE CACHING
// ========================================
static bool needsStatusUpdate = false;
static bool needsFullRedraw = true;
static unsigned long lastDisplayUpdate = 0;
#define DISPLAY_UPDATE_INTERVAL 500  // Update LCD every 500ms

// ========================================
// INTERRUPT PIN VARIABLES
// ========================================
volatile unsigned long lastInterruptTime = 0;
volatile unsigned long cachedDebounceDelay = DEBOUNCE_DELAY;

// ========================================
// SD CARD MACROS
// ========================================
#define SD_BEGIN() digitalWrite(SD_CS_PIN, LOW)
#define SD_END() digitalWrite(SD_CS_PIN, HIGH)

const char* COUNT_FILE = "/count.txt";
const char* HOURLY_FILE = "/hourly_count.txt";
const char* CUMULATIVE_FILE = "/cumulative_count.txt";

// ========================================
// STATUS SYSTEM DEFINES
// ========================================
#define STATUS_DISPLAY_DURATION 3000

// ========================================
// EEPROM CONFIGURATION
// ========================================
#define EEPROM_SIZE 512
#define EEPROM_ADDR_SAVE_INTERVAL 0
#define EEPROM_ADDR_DEBOUNCE_DELAY 4
#define EEPROM_ADDR_MAX_COUNT 8
#define EEPROM_ADDR_STATUS_DURATION 12
#define EEPROM_MAGIC_NUMBER 0xAB  // Magic number to verify EEPROM data
#define EEPROM_ADDR_MAGIC 16

// ========================================
// RUNTIME PARAMETERS
// ========================================
struct RuntimeParams {
  unsigned long saveInterval = SAVE_INTERVAL;
  unsigned long debounceDelay = DEBOUNCE_DELAY;
  int maxCount = MAX_COUNT;
  unsigned long statusDisplayDuration = STATUS_DISPLAY_DURATION;
};
RuntimeParams runtimeParams;

// ========================================
// STATUS SYSTEM
// ========================================
enum Status {
  STATUS_IDLE,
  STATUS_COUNTING,
  STATUS_HOUR_COMPLETE,
  STATUS_INITIALIZING,
  STATUS_DIAGNOSTIC
};
Status currentStatus = STATUS_INITIALIZING;
String statusMessage = "";
unsigned long statusDisplayTime = 0;

// ========================================
// DIAGNOSTIC VARIABLES
// ========================================
volatile bool diagnosticRequested = false;
struct TestResults {
  bool oledOk = false;
  bool sdOk = false;
  bool rtcOk = false;
};
TestResults testResults;

// ========================================
// INTERRUPT SERVICE ROUTINES
// ========================================
void IRAM_ATTR handleInterrupt() {
  unsigned long currentTime = millis();
  unsigned long debounce = cachedDebounceDelay;
  
  if (currentTime - lastInterruptTime > debounce) {
    if (currentCount < MAX_COUNT) {
      currentCount++;
      countChanged = true;
      productionSessionCount++;
    }
    lastInterruptTime = currentTime;
  }
}

void IRAM_ATTR handleDiagnosticButton() {
  static unsigned long lastPress = 0;
  unsigned long now = millis();
  if (now - lastPress > 200) {
    diagnosticRequested = true;
    lastPress = now;
  }
}

void IRAM_ATTR handleProductionLatch() {
  static unsigned long lastPress = 0;
  unsigned long now = millis();
  if (now - lastPress > 500) {  // Debounce latch button
    productionActive = !productionActive;
    productionStateChanged = true;
    lastPress = now;
  }
}

// ========================================
// EEPROM MANAGEMENT FUNCTIONS
// ========================================
void saveSettingsToEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  
  // Write each parameter
  EEPROM.writeULong(EEPROM_ADDR_SAVE_INTERVAL, runtimeParams.saveInterval);
  EEPROM.writeULong(EEPROM_ADDR_DEBOUNCE_DELAY, runtimeParams.debounceDelay);
  EEPROM.writeInt(EEPROM_ADDR_MAX_COUNT, runtimeParams.maxCount);
  EEPROM.writeULong(EEPROM_ADDR_STATUS_DURATION, runtimeParams.statusDisplayDuration);
  
  // Write magic number to verify data is valid
  EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC_NUMBER);
  
  // Commit changes to flash
  EEPROM.commit();
  EEPROM.end();
  
  Serial.println("✓ Settings saved to EEPROM");
}

void loadSettingsFromEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  
  // Check if EEPROM has valid data
  uint8_t magic = EEPROM.read(EEPROM_ADDR_MAGIC);
  
  if (magic == EEPROM_MAGIC_NUMBER) {
    // Load saved parameters
    runtimeParams.saveInterval = EEPROM.readULong(EEPROM_ADDR_SAVE_INTERVAL);
    runtimeParams.debounceDelay = EEPROM.readULong(EEPROM_ADDR_DEBOUNCE_DELAY);
    runtimeParams.maxCount = EEPROM.readInt(EEPROM_ADDR_MAX_COUNT);
    runtimeParams.statusDisplayDuration = EEPROM.readULong(EEPROM_ADDR_STATUS_DURATION);
    
    Serial.println("✓ Settings loaded from EEPROM");
    Serial.print("  Save Interval: "); Serial.print(runtimeParams.saveInterval); Serial.println(" ms");
    Serial.print("  Debounce Delay: "); Serial.print(runtimeParams.debounceDelay); Serial.println(" ms");
    Serial.print("  Max Count: "); Serial.println(runtimeParams.maxCount);
    Serial.print("  Status Display Time: "); Serial.print(runtimeParams.statusDisplayDuration); Serial.println(" ms");
  } else {
    Serial.println("⚠ No saved settings found in EEPROM (using defaults)");
    saveSettingsToEEPROM(); // Save defaults for next time
  }
  
  EEPROM.end();
}

void resetSettingsToDefault() {
  runtimeParams.saveInterval = SAVE_INTERVAL;
  runtimeParams.debounceDelay = DEBOUNCE_DELAY;
  runtimeParams.maxCount = MAX_COUNT;
  runtimeParams.statusDisplayDuration = STATUS_DISPLAY_DURATION;
  
  saveSettingsToEEPROM();
  Serial.println("✓ Settings reset to defaults and saved");
}

// ========================================
// SETUP
// ========================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   ESP32 COUNTER - OLED 128x64 VERSION ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // Load settings from EEPROM
  Serial.println("--- Loading Settings from EEPROM ---");
  loadSettingsFromEEPROM();
  cachedDebounceDelay = runtimeParams.debounceDelay;
  
  // Initialize I2C for LCD
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("✓ I2C initialized");
  
  // Initialize LCD Display
  Serial.println("\n--- LCD Display Initialization ---");
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  Serial.println("✓ LCD Display initialized (16x4)");
  
  displayLCDStartupMessage();
  
  // Configure SD CS pin
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(100);
  
  Serial.println("✓ CS pin configured");
  
  // Initialize VSPI for SD Card
  Serial.println("\n--- VSPI (SPI3) for SD Card ---");
  Serial.print("  SCK:  GPIO "); Serial.println(SD_SCK);
  Serial.print("  MOSI: GPIO "); Serial.println(SD_MOSI);
  Serial.print("  MISO: GPIO "); Serial.println(SD_MISO);
  Serial.print("  CS:   GPIO "); Serial.println(SD_CS_PIN);
  
  spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS_PIN);
  Serial.println("✓ VSPI bus initialized");
  
  delay(100);
  
  // Initialize SD Card
  Serial.println("\n--- SD Card Initialization ---");
  digitalWrite(SD_CS_PIN, HIGH);
  delay(500);
  
  Serial.println("Attempting SD initialization on VSPI...");
  sdAvailable = initializeSD();
  
  if (!sdAvailable) {
    Serial.println("✗ SD Card failed - continuing without SD");
    showStatus("SD ERROR", 2000);
  } else {
    Serial.println("✓ SD Card ready");
    showStatus("SD OK", 1000);
  }
  
  // Initialize RTC
  Serial.println("\n--- RTC Initialization ---");
  showStatus("Init RTC...", 500);
  
  if (!rtc.begin()) {
    Serial.println("✗ RTC not responding");
    rtcAvailable = false;
  } else {
    rtcAvailable = true;
    Serial.println("✓ RTC responding");
    
    if (rtc.lostPower()) {
      Serial.println("⚠ RTC lost power!");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      Serial.println("⚠ Using compile time");
    }
    
    DateTime currentTime = rtc.now();
    Serial.print("✓ RTC time: ");
    Serial.print(currentTime.year()); Serial.print("-");
    Serial.print(currentTime.month()); Serial.print("-");
    Serial.print(currentTime.day()); Serial.print(" ");
    Serial.print(currentTime.hour()); Serial.print(":");
    Serial.print(currentTime.minute());
    Serial.println();
  }
  
  showStatus(rtcAvailable ? "RTC OK" : "RTC FAIL", 1000);
  
  // Initialize files
  if (sdAvailable) {
    Serial.println("\n--- File System Check ---");
    showStatus("Checking...", 500);
    initializeFiles();
    
    currentCount = readCountFromFile(COUNT_FILE);
    hourlyCount = readCountFromFile(HOURLY_FILE);
    cumulativeCount = readCountFromFile(CUMULATIVE_FILE);
    Serial.print("✓ Loaded counts: current=");
    Serial.print(currentCount);
    Serial.print(", hourly=");
    Serial.print(hourlyCount);
    Serial.print(", cumulative=");
    Serial.println(cumulativeCount);
  }
  
  // Setup interrupts
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleInterrupt, FALLING);
  
  pinMode(DIAGNOSTIC_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DIAGNOSTIC_PIN), handleDiagnosticButton, FALLING);
  
  // Setup Production Latch Button
  pinMode(PRODUCTION_LATCH_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PRODUCTION_LATCH_PIN), handleProductionLatch, FALLING);
  
  Serial.println("\n✓ Interrupts configured");
  Serial.print("  Counter pin:     GPIO "); Serial.println(INTERRUPT_PIN);
  Serial.print("  Diagnostic pin:  GPIO "); Serial.println(DIAGNOSTIC_PIN);
  Serial.print("  Production pin:  GPIO "); Serial.println(PRODUCTION_LATCH_PIN);
  
  if (rtcAvailable) {
    DateTime now = rtc.now();
    lastHour = now.hour();
  } else {
    lastHour = 0;
  }
  
  needsFullRedraw = true;
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║          ✓✓✓ SYSTEM READY ✓✓✓          ║");
  Serial.println("║    LCD Display Configuration:          ║");
  Serial.println("║    • 16x4 characters                   ║");
  Serial.println("║    • I2C interface (0x27/0x3F)         ║");
  Serial.println("║    • Production latch on GPIO 25       ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("\nPress GPIO 27 for diagnostics");
  Serial.println("Type 'INFO' in Serial for DEBUG MENU\n");
  
  debugMenu();
}

// ========================================
// MAIN LOOP
// ========================================
void loop() {
  if (diagnosticRequested) {
    diagnosticRequested = false;
    delay(300);
    enterDiagnosticMode();
    return;
  }
  
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() > 0) {
      input.toUpperCase();
      
      if (!processDebugCommand(input)) {
        checkAndSetTimeFromSerial(input);
      }
    }
    return;
  }
  
  if (currentMode == MODE_NORMAL) {
    unsigned long now = millis();
    
    cachedDebounceDelay = runtimeParams.debounceDelay;
    
    // Handle production state changes
    if (productionStateChanged) {
      productionStateChanged = false;
      if (productionActive) {
        // Production started
        productionStartTime = rtcAvailable ? rtc.now() : DateTime(2025, 1, 1, 0, 0, 0);
        productionSessionCount = 0;
        createProductionLogFile(productionStartTime, true);
        statusMessage = "PROD STARTED";
        showStatus("PROD STARTED", 2000);
      } else {
        // Production stopped
        DateTime productionEndTime = rtcAvailable ? rtc.now() : DateTime(2025, 1, 1, 0, 0, 0);
        updateProductionLogFile(productionStartTime, productionEndTime, productionSessionCount);
        statusMessage = "PROD STOPPED";
        showStatus("PROD STOPPED", 2000);
      }
      needsFullRedraw = true;
    }
    
    // Update time display
    static unsigned long lastTimeUpdate = 0;
    if (now - lastTimeUpdate >= 1000) {
      if (rtcAvailable) {
        DateTime rtcNow = rtc.now();
        
        if (rtcNow.hour() != lastHour) {
          handleHourChange(rtcNow);
          lastHour = rtcNow.hour();
        }
      }
      needsFullRedraw = true;
      lastTimeUpdate = now;
    }
    
    // Save count periodically
    if (countChanged && sdAvailable && (now - lastSaveTime > runtimeParams.saveInterval)) {
      noInterrupts();
      int countToSave = currentCount;
      countChanged = false;
      interrupts();
      
      writeCountToFile(COUNT_FILE, countToSave);
      needsStatusUpdate = true;
      lastSaveTime = now;
    }
    
    // Update display with throttling
    if ((countChanged || needsFullRedraw || productionStateChanged) && 
        (now - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL)) {
      drawMainLCDScreen();
      needsFullRedraw = false;
      lastDisplayUpdate = now;
    }
    
    // Clear temporary status
    if (currentStatus != STATUS_IDLE && 
        currentStatus != STATUS_COUNTING &&
        now - statusDisplayTime > runtimeParams.statusDisplayDuration) {
      currentStatus = STATUS_IDLE;
      statusMessage = "Ready";
      needsFullRedraw = true;
    }
  }
  
  delay(10);
}

// ========================================
// PRODUCTION LOGGING FUNCTIONS
// ========================================
void createProductionLogFile(DateTime startTime, bool isStart) {
  if (!sdAvailable) {
    Serial.println("⚠ SD Card not available, cannot create production log");
    return;
  }
  
  char filename[48];
  snprintf(filename, sizeof(filename), "/PROD_%04d%02d%02d_%02d%02d%02d.txt",
           startTime.year(), startTime.month(), startTime.day(),
           startTime.hour(), startTime.minute(), startTime.second());
  
  SD_BEGIN();
  File file = SD.open(filename, FILE_WRITE);
  
  if (!file) {
    SD_END();
    Serial.print("✗ Failed to create production log: ");
    Serial.println(filename);
    return;
  }
  
  file.println("=== PRODUCTION LOG ===");
  file.print("Started: ");
  file.print(startTime.year()); file.print("-");
  file.print(startTime.month()); file.print("-");
  file.print(startTime.day()); file.print(" ");
  file.print(startTime.hour()); file.print(":");
  file.print(startTime.minute()); file.print(":");
  file.println(startTime.second());
  
  file.println("End: [WAITING]");
  file.println("Count: 0");
  
  file.flush();
  file.close();
  SD_END();
  
  Serial.print("✓ Production log created: ");
  Serial.println(filename);
}

void updateProductionLogFile(DateTime startTime, DateTime endTime, int sessionCount) {
  if (!sdAvailable) {
    Serial.println("⚠ SD Card not available, cannot update production log");
    return;
  }
  
  char filename[48];
  snprintf(filename, sizeof(filename), "/PROD_%04d%02d%02d_%02d%02d%02d.txt",
           startTime.year(), startTime.month(), startTime.day(),
           startTime.hour(), startTime.minute(), startTime.second());
  
  SD_BEGIN();
  
  // Read the file
  if (!SD.exists(filename)) {
    SD_END();
    Serial.print("⚠ Production log file not found: ");
    Serial.println(filename);
    return;
  }
  
  File file = SD.open(filename, FILE_READ);
  String content = "";
  while (file.available()) {
    content += (char)file.read();
  }
  file.close();
  
  // Create new content with updated end time and count
  String newContent = "=== PRODUCTION LOG ===\n";
  newContent += "Started: ";
  newContent += String(startTime.year()); newContent += "-";
  newContent += String(startTime.month()); newContent += "-";
  newContent += String(startTime.day()); newContent += " ";
  newContent += String(startTime.hour()); newContent += ":";
  newContent += String(startTime.minute()); newContent += ":";
  newContent += String(startTime.second()); newContent += "\n";
  
  newContent += "Ended: ";
  newContent += String(endTime.year()); newContent += "-";
  newContent += String(endTime.month()); newContent += "-";
  newContent += String(endTime.day()); newContent += " ";
  newContent += String(endTime.hour()); newContent += ":";
  newContent += String(endTime.minute()); newContent += ":";
  newContent += String(endTime.second()); newContent += "\n";
  
  newContent += "Count: ";
  newContent += String(sessionCount); newContent += "\n";
  
  // Write updated content
  SD.remove(filename);
  file = SD.open(filename, FILE_WRITE);
  if (file) {
    file.print(newContent);
    file.flush();
    file.close();
    
    Serial.print("✓ Production log updated: ");
    Serial.println(filename);
    Serial.print("  Start: "); printDateTime(startTime);
    Serial.print("  End: ");   printDateTime(endTime);
    Serial.print("  Count: "); Serial.println(sessionCount);
  }
  
  SD_END();
}

void printDateTime(DateTime dt) {
  Serial.print(dt.year()); Serial.print("-");
  Serial.print(dt.month()); Serial.print("-");
  Serial.print(dt.day()); Serial.print(" ");
  Serial.print(dt.hour()); Serial.print(":");
  Serial.print(dt.minute()); Serial.print(":");
  Serial.println(dt.second());
}

// ========================================
// SD CARD FUNCTIONS
// ========================================
bool initializeSD() {
  Serial.println("\n=== SD Card Initialization ===");
  
  digitalWrite(SD_CS_PIN, HIGH);
  delay(200);
  
  Serial.println("Step 1: CS pin secured");
  Serial.print("  SD_CS (GPIO "); Serial.print(SD_CS_PIN); Serial.println("): HIGH");
  
  uint32_t speeds[] = {400000, 1000000, 5000000};
  const char* speedDesc[] = {"400kHz (SLOW/SAFE)", "1MHz (MEDIUM)", "5MHz (FAST)"};
  
  for (int i = 0; i < 3; i++) {
    if (i > 0) {
      Serial.print("\n  Retrying with next speed...");
      SD.end();
      delay(300);
      digitalWrite(SD_CS_PIN, HIGH);
      delay(300);
    }
    
    Serial.print("\n  Attempt "); Serial.print(i + 1); 
    Serial.print(": "); Serial.println(speedDesc[i]);
    
    digitalWrite(SD_CS_PIN, HIGH);
    delayMicroseconds(500);
    
    Serial.print("    Calling SD.begin()...");
    if (!SD.begin(SD_CS_PIN, spiSD, speeds[i])) {
      Serial.println(" FAILED to initialize");
      continue;
    }
    
    Serial.println(" SUCCESS!");
    
    uint8_t cardType = SD.cardType();
    Serial.print("    Card Type: ");
    
    if (cardType == CARD_NONE) {
      Serial.println("NONE (no card inserted)");
      SD.end();
      continue;
    } else if (cardType == CARD_SD) {
      Serial.println("SD");
    } else if (cardType == CARD_SDHC) {
      Serial.println("SDHC");
    } else {
      Serial.println("UNKNOWN");
    }
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.print("    Card Size: ");
    Serial.print(cardSize);
    Serial.println(" MB");
    
    Serial.print("    Testing write capability...");
    File testFile = SD.open("/INIT_TEST.txt", FILE_WRITE);
    if (testFile) {
      testFile.println("Init OK");
      testFile.close();
      Serial.println(" ✓ Write OK");
      
      SD.remove("/INIT_TEST.txt");
    } else {
      Serial.println(" ✗ FAILED");
      SD.end();
      continue;
    }
    
    Serial.print("\n  ✓✓✓ SD Card initialized at ");
    Serial.print(speeds[i] / 1000);
    Serial.println(" kHz ✓✓✓");
    
    digitalWrite(SD_CS_PIN, HIGH);
    delay(100);
    return true;
  }
  
  Serial.println("\n  ✗✗✗ SD Card initialization FAILED on all speeds ✗✗✗");
  SD.end();
  digitalWrite(SD_CS_PIN, HIGH);
  delay(100);
  
  return false;
}

void initializeFiles() {
  Serial.println("\n--- Initializing SD Files ---");
  
  SD_BEGIN();
  
  if (!SD.exists(COUNT_FILE)) {
    Serial.print("Creating "); Serial.println(COUNT_FILE);
    writeCountToFile(COUNT_FILE, 0);
    
    if (SD.exists(COUNT_FILE)) {
      Serial.print("  ✓ "); Serial.println(COUNT_FILE);
    } else {
      Serial.print("  ✗ Failed to create "); Serial.println(COUNT_FILE);
    }
  } else {
    Serial.print("✓ "); Serial.print(COUNT_FILE); Serial.println(" exists");
  }
  
  if (!SD.exists(HOURLY_FILE)) {
    Serial.print("Creating "); Serial.println(HOURLY_FILE);
    writeCountToFile(HOURLY_FILE, 0);
    
    if (SD.exists(HOURLY_FILE)) {
      Serial.print("  ✓ "); Serial.println(HOURLY_FILE);
    } else {
      Serial.print("  ✗ Failed to create "); Serial.println(HOURLY_FILE);
    }
  } else {
    Serial.print("✓ "); Serial.print(HOURLY_FILE); Serial.println(" exists");
  }
  
  if (!SD.exists(CUMULATIVE_FILE)) {
    Serial.print("Creating "); Serial.println(CUMULATIVE_FILE);
    writeCountToFile(CUMULATIVE_FILE, 0);
    
    if (SD.exists(CUMULATIVE_FILE)) {
      Serial.print("  ✓ "); Serial.println(CUMULATIVE_FILE);
    } else {
      Serial.print("  ✗ Failed to create "); Serial.println(CUMULATIVE_FILE);
    }
  } else {
    Serial.print("✓ "); Serial.print(CUMULATIVE_FILE); Serial.println(" exists");
  }
  
  SD_END();
  Serial.println("File initialization complete");
}

int readCountFromFile(const char* filename) {
  SD_BEGIN();
  
  if (!SD.exists(filename)) {
    Serial.print("⚠ File not found: "); Serial.println(filename);
    SD_END();
    return 0;
  }
  
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.print("✗ Failed to open: "); Serial.println(filename);
    SD_END();
    return 0;
  }
  
  char buffer[10] = {0};
  int bytesRead = file.read((uint8_t*)buffer, sizeof(buffer) - 1);
  file.close();
  SD_END();
  
  if (bytesRead <= 0) {
    Serial.print("⚠ Empty file: "); Serial.println(filename);
    return 0;
  }
  
  int count = atoi(buffer);
  if (count < 0 || count > MAX_COUNT) {
    Serial.print("⚠ Invalid count in "); 
    Serial.print(filename); 
    Serial.print(": "); 
    Serial.print(count);
    Serial.println(" (reset to 0)");
    count = 0;
  }
  
  return count;
}

void writeCountToFile(const char* filename, int count) {
  if (count < 0 || count > MAX_COUNT) {
    Serial.print("✗ Invalid count value: "); Serial.println(count);
    return;
  }
  
  SD_BEGIN();
  
  // Delete old file first to ensure clean write
  if (SD.exists(filename)) {
    SD.remove(filename);
  }
  
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.print("✗ Cannot open file for writing: "); Serial.println(filename);
    SD_END();
    return;
  }
  
  file.println(count);
  file.flush();
  file.close();
  SD_END();
}

void handleHourChange(DateTime now) {
  Serial.println("\n>>> Hour Changed <<<");
  
  noInterrupts();
  int finalCount = currentCount;
  currentCount = 0;
  countChanged = false;
  interrupts();
  
  hourlyCount = finalCount;
  
  // Add hourly count to cumulative count
  cumulativeCount += hourlyCount;
  
  if (sdAvailable) {
    writeCountToFile(COUNT_FILE, 0);
    writeCountToFile(HOURLY_FILE, hourlyCount);
    writeCountToFile(CUMULATIVE_FILE, cumulativeCount);
    createHourlyLogFile(now, finalCount, cumulativeCount);
  }
  
  needsFullRedraw = true;
  showStatus("Hour Logged", 2000);
  
  Serial.print("✓ Hour logged: "); Serial.print(finalCount);
  Serial.print(" | Cumulative: "); Serial.println(cumulativeCount);
}

void createHourlyLogFile(DateTime dt, int count, int cumulative) {
  char filename[32];
  int h = getDisplay12Hour(dt.hour());
  
  snprintf(filename, sizeof(filename), "/%04d_%02d_%02d_%02d_%02d%s.txt",
           dt.year(), dt.month(), dt.day(), h, dt.minute(),
           getAmPm(dt.hour()));
  
  SD_BEGIN();
  
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    SD_END();
    Serial.print("✗ Failed to create log: "); Serial.println(filename);
    return;
  }
  
  file.print("Time: ");
  file.print(dt.year()); file.print("-");
  file.print(dt.month()); file.print("-");
  file.print(dt.day()); file.print(" ");
  file.print(dt.hour()); file.print(":");
  file.println(dt.minute());
  file.print("Hour Count: ");
  file.println(count);
  file.print("Cumulative: ");
  file.println(cumulative);
  
  file.flush();
  file.close();
  SD_END();
  
  Serial.print("✓ Log: "); Serial.println(filename);
}

// ========================================
// LCD DISPLAY FUNCTIONS
// ========================================
void displayLCDStartupMessage() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("PRODUCTION");
  lcd.setCursor(5, 1);
  lcd.print("COUNTER");
  lcd.setCursor(4, 3);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();
}

void showStatus(const char* message, int duration) {
  lcd.clear();
  lcd.setCursor(0, 1);
  
  // Center message on LCD
  int msgLen = strlen(message);
  int startCol = (LCD_COLS - msgLen) / 2;
  if (startCol < 0) startCol = 0;
  
  lcd.setCursor(startCol, 1);
  lcd.print(message);
  delay(duration);
  lcd.clear();
  needsFullRedraw = true;
}

void drawMainLCDScreen() {
  // Get current time
  DateTime now = rtcAvailable ? rtc.now() : DateTime(2025, 1, 1, 12, 0, 0);
  
  noInterrupts();
  int count = currentCount;
  interrupts();
  
  lcd.clear();
  
  // Row 0: Time and Production Status
  char row0[17];
  if (productionActive) {
    snprintf(row0, sizeof(row0), "[PROD] %02d:%02d:%02d", 
             now.hour(), now.minute(), now.second());
  } else {
    snprintf(row0, sizeof(row0), "[IDLE] %02d:%02d:%02d", 
             now.hour(), now.minute(), now.second());
  }
  lcd.setCursor(0, 0);
  lcd.print(row0);
  
  // Row 1: Current Count
  char row1[17];
  snprintf(row1, sizeof(row1), "Count: %05d", count);
  lcd.setCursor(0, 1);
  lcd.print(row1);
  
  // Row 2: Session count (if production active) or hourly count
  char row2[17];
  if (productionActive) {
    snprintf(row2, sizeof(row2), "Session: %05d", productionSessionCount);
  } else {
    snprintf(row2, sizeof(row2), "Hour: %05d", hourlyCount);
  }
  lcd.setCursor(0, 2);
  lcd.print(row2);
  
  // Row 3: Production start time or status message
  char row3[17];
  if (productionActive) {
    snprintf(row3, sizeof(row3), "S:%02d:%02d:%02d", 
             productionStartTime.hour(), 
             productionStartTime.minute(), 
             productionStartTime.second());
  } else {
    strncpy(row3, statusMessage.c_str(), 16);
    row3[16] = '\0';
  }
  lcd.setCursor(0, 3);
  lcd.print(row3);
}

// ========================================
// TIME SETTING
// ========================================
inline int getDisplay12Hour(int hour) {
  int h = hour % 12;
  return (h == 0) ? 12 : h;
}

inline const char* getAmPm(int hour) {
  return (hour >= 12) ? "PM" : "AM";
}

bool checkAndSetTimeFromSerial(String input) {
  int commaCount = 0;
  for (int i = 0; i < input.length(); i++) {
    if (input[i] == ',') commaCount++;
  }
  
  if (commaCount != 5) {
    return false;
  }
  
  int values[6] = {0};
  int index = 0;
  int value = 0;
  
  for (int i = 0; i < input.length() && index < 6; i++) {
    char c = input[i];
    if (c >= '0' && c <= '9') {
      value = value * 10 + (c - '0');
    } else if (c == ',') {
      values[index++] = value;
      value = 0;
    }
  }
  
  // Don't forget last value
  if (index < 6) {
    values[index] = value;
  }
  
  if (index == 5 && value >= 0 && value <= 59) {
    if (values[0] >= 2020 && values[0] <= 2100 &&
        values[1] >= 1 && values[1] <= 12 &&
        values[2] >= 1 && values[2] <= 31 &&
        values[3] >= 0 && values[3] <= 23 &&
        values[4] >= 0 && values[4] <= 59) {
      
      DateTime newTime(values[0], values[1], values[2], values[3], values[4], value);
      rtc.adjust(newTime);
      
      Serial.print("✓ RTC set to: ");
      Serial.print(values[0]); Serial.print("-");
      Serial.print(values[1]); Serial.print("-");
      Serial.print(values[2]); Serial.print(" ");
      Serial.print(values[3]); Serial.print(":");
      Serial.print(values[4]); Serial.print(":");
      Serial.println(value);
      return true;
    } else {
      Serial.println("✗ Invalid values. Expected: YYYY,MM,DD,HH,MM,SS");
      return true;
    }
  }
  
  return false;
}

// ========================================
// DEBUG MENU
// ========================================
void debugMenu() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║        DEBUG MENU - PARAMETERS         ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  Serial.println("Current Settings:");
  Serial.print("  1. Save Interval:        "); Serial.print(runtimeParams.saveInterval); Serial.println(" ms");
  Serial.print("  2. Debounce Delay:       "); Serial.print(runtimeParams.debounceDelay); Serial.println(" ms");
  Serial.print("  3. Max Count:            "); Serial.println(runtimeParams.maxCount);
  Serial.print("  4. Status Display Time:  "); Serial.print(runtimeParams.statusDisplayDuration); Serial.println(" ms");
  
  Serial.println("\nCount Status:");
  Serial.print("  Current Count:           "); Serial.println(currentCount);
  Serial.print("  Hourly Count:            "); Serial.println(hourlyCount);
  Serial.print("  Cumulative Count:        "); Serial.println(cumulativeCount);
  
  Serial.println("\nCommands:");
  Serial.println("  SET,1,<value>  - Change Save Interval (ms) [1000-60000]");
  Serial.println("  SET,2,<value>  - Change Debounce Delay (ms) [10-500]");
  Serial.println("  SET,3,<value>  - Change Max Count [100-99999]");
  Serial.println("  SET,4,<value>  - Change Status Display Time (ms) [1000-30000]");
  Serial.println("  DEBOUNCE,<value> - Quick set debounce delay (ms) [10-500]");
  Serial.println("  SAVE           - Save all settings to EEPROM");
  Serial.println("  RESET          - Reset Current Count to 0");
  Serial.println("  RESETPARAM     - Reset all parameters to defaults");
  Serial.println("  TIME,YYYY,MM,DD,HH,MM,SS - Set RTC time");
  Serial.println("  INFO           - Show this menu again\n");
}

bool processDebugCommand(String input) {
  if (input == "INFO") {
    debugMenu();
    return true;
  }
  
  if (input == "SAVE") {
    saveSettingsToEEPROM();
    Serial.println("⚠ Note: Settings will persist after power cycle");
    return true;
  }
  
  if (input == "RESETPARAM") {
    resetSettingsToDefault();
    Serial.println("⚠ All parameters have been reset to defaults");
    return true;
  }
  
  if (input == "RESET") {
    noInterrupts();
    currentCount = 0;
    countChanged = true;
    interrupts();
    needsFullRedraw = true;
    Serial.println("✓ Current count reset to 0");
    return true;
  }
  
  // Quick debounce command: DEBOUNCE,<value>
  if (input.startsWith("DEBOUNCE,")) {
    String valueStr = input.substring(9); // Skip "DEBOUNCE,"
    long newValue = valueStr.toInt();
    
    if (newValue >= 10 && newValue <= 500) {
      runtimeParams.debounceDelay = newValue;
      cachedDebounceDelay = newValue;
      Serial.print("✓ Debounce Delay updated to: "); Serial.print(newValue); Serial.println(" ms");
      Serial.print("  (Will take effect on next interrupt)");
      Serial.println("  Use 'SAVE' command to persist settings");
      return true;
    } else {
      Serial.println("✗ Invalid debounce value. Range: 10-500 ms");
      Serial.print("  Current debounce: "); Serial.print(runtimeParams.debounceDelay); Serial.println(" ms");
      return true;
    }
  }
  
  if (input.startsWith("SET,")) {
    int firstComma = input.indexOf(',');
    int secondComma = input.indexOf(',', firstComma + 1);
    
    if (firstComma != -1 && secondComma != -1) {
      String paramStr = input.substring(firstComma + 1, secondComma);
      String newValueStr = input.substring(secondComma + 1);
      
      int param = paramStr.toInt();
      long newValue = newValueStr.toInt();
      
      switch (param) {
        case 1:
          if (newValue >= 1000 && newValue <= 60000) {
            runtimeParams.saveInterval = newValue;
            Serial.print("✓ Save Interval: "); Serial.print(newValue); Serial.println(" ms");
            Serial.println("  Use 'SAVE' command to persist settings");
          } else {
            Serial.println("✗ Value out of range (1000-60000)");
          }
          return true;
          
        case 2:
          if (newValue >= 10 && newValue <= 500) {
            runtimeParams.debounceDelay = newValue;
            cachedDebounceDelay = newValue;
            Serial.print("✓ Debounce Delay: "); Serial.print(newValue); Serial.println(" ms");
            Serial.println("  (Will take effect on next interrupt)");
            Serial.println("  Use 'SAVE' command to persist settings");
          } else {
            Serial.println("✗ Value out of range (10-500)");
          }
          return true;
          
        case 3:
          if (newValue >= 100 && newValue <= 99999) {
            runtimeParams.maxCount = newValue;
            Serial.print("✓ Max Count: "); Serial.println(newValue);
            Serial.println("  Use 'SAVE' command to persist settings");
          } else {
            Serial.println("✗ Value out of range (100-99999)");
          }
          return true;
          
        case 4:
          if (newValue >= 1000 && newValue <= 30000) {
            runtimeParams.statusDisplayDuration = newValue;
            Serial.print("✓ Status Display Time: "); Serial.print(newValue); Serial.println(" ms");
            Serial.println("  Use 'SAVE' command to persist settings");
          } else {
            Serial.println("✗ Value out of range (1000-30000)");
          }
          return true;
      }
    }
    return true;
  }
  
  return false;
}

// ========================================
// DIAGNOSTIC MODE
// ========================================
void enterDiagnosticMode() {
  currentMode = MODE_DIAGNOSTIC;
  Serial.println("\n>>> DIAGNOSTIC MODE <<<");
  
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("DIAGNOSTIC");
  lcd.setCursor(5, 1);
  lcd.print("Testing...");
  delay(500);
  
  runDiagnosticTests();
  displayDiagnosticResults();
  
  lcd.setCursor(3, 3);
  lcd.print("Press to exit");
  
  diagnosticRequested = false;
  while (!diagnosticRequested) delay(100);
  diagnosticRequested = false;
  
  currentMode = MODE_NORMAL;
  if (sdAvailable) {
    currentCount = readCountFromFile(COUNT_FILE);
    hourlyCount = readCountFromFile(HOURLY_FILE);
    cumulativeCount = readCountFromFile(CUMULATIVE_FILE);
  }
  needsFullRedraw = true;
  statusMessage = "Ready";
  
  Serial.println(">>> Normal mode <<<\n");
}

void runDiagnosticTests() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("DIAGNOSTIC");
  
  // Test LCD (already working if we can see this)
  testResults.oledOk = true;
  lcd.setCursor(0, 1);
  lcd.print("LCD:   PASS");
  delay(300);
  
  // Test SD Card
  testResults.sdOk = testSDCard();
  lcd.setCursor(0, 2);
  lcd.print("SD:    ");
  lcd.print(testResults.sdOk ? "PASS" : "FAIL");
  delay(300);
  
  // Test RTC
  testResults.rtcOk = testRTCModule();
  lcd.setCursor(0, 3);
  lcd.print("RTC:   ");
  lcd.print(testResults.rtcOk ? "PASS" : "FAIL");
  delay(300);
}

bool testSDCard() {
  digitalWrite(SD_CS_PIN, HIGH);
  delay(50);
  
  if (SD.begin(SD_CS_PIN, spiSD)) {
    SD.end();
    digitalWrite(SD_CS_PIN, HIGH);
    return true;
  }
  return false;
}

bool testRTCModule() {
  if (!rtc.begin()) return false;
  DateTime now = rtc.now();
  return (now.year() >= 2020 && now.year() <= 2100);
}

void displayDiagnosticResults() {
  int pass = (testResults.oledOk ? 1 : 0) + 
             (testResults.sdOk ? 1 : 0) + 
             (testResults.rtcOk ? 1 : 0);
  
  lcd.setCursor(0, 0);
  lcd.print("Score: ");
  lcd.print(pass);
  lcd.print("/3 (");
  lcd.print((pass * 100) / 3);
  lcd.print("%)");
}