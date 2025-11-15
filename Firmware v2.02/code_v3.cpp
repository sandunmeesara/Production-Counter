/*
 * ESP32 Hourly Counter System - OLED 128x64 VERSION
 * Features: Interrupt counting, hourly logging, OLED display, diagnostics
 * Libraries: Adafruit SSD1306, RTClib, SD, Adafruit GFX
 * 
 * OLED Display: 128x64 pixels, I2C interface
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <EEPROM.h>

// ========================================
// PIN DEFINITIONS
// ========================================
#define INTERRUPT_PIN 15      // Counter button (to GND)
#define DIAGNOSTIC_PIN 27     // Diagnostic button (to GND)
#define LATCHING_PIN 25       // Production latching button (to GND)

// OLED Display - I2C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1         // Reset pin (-1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C   // I2C address (0x3C for most 128x64 OLEDs)

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
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
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

// ========================================
// PRODUCTION SESSION VARIABLES
// ========================================
volatile bool productionActive = false;
DateTime productionStartTime;
DateTime productionStopTime;
int productionStartCount = 0;
int productionCount = 0;
volatile bool productionStatusChanged = false;
volatile unsigned long lastLatchingButtonTime = 0;
volatile unsigned long lastDiagnosticButtonTime = 0;

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
const char* PRODUCTION_STATE_FILE = "/prod_session.txt";  // Stores active session for recovery

// ========================================
// HELPER FUNCTION DECLARATIONS
// ========================================
void printDivider();
void printHeader(const char* title);
String normalizeFilePath(const char* filename);
void centerDisplayText(int textSize, int y, const char* text);
void formatTimeString(char* buffer, DateTime dt, bool includeSeconds);
void saveProductionState();
void restoreProductionState();
void clearProductionState();

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
  
  if (currentTime - lastInterruptTime > cachedDebounceDelay) {
    // Only count during production session
    if (productionActive && currentCount < MAX_COUNT) {
      currentCount++;
      countChanged = true;
    }
    lastInterruptTime = currentTime;
  }
}

void IRAM_ATTR handleDiagnosticButton() {
  unsigned long now = millis();
  if (now - lastDiagnosticButtonTime > 200) {
    diagnosticRequested = true;
    lastDiagnosticButtonTime = now;
  }
}

void IRAM_ATTR handleLatchingButton() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastLatchingButtonTime > 100) {
    productionStatusChanged = true;
    lastLatchingButtonTime = currentTime;
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
// HELPER FUNCTIONS
// ========================================

void printDivider() {
  Serial.println("─────────────────────────────────────────────");
}

void printHeader(const char* title) {
  Serial.println();
  Serial.println("╔════════════════════════════════════════╗");
  Serial.print("║      ");
  Serial.print(title);
  // Padding for alignment
  int titleLen = strlen(title);
  for (int i = 0; i < 30 - titleLen; i++) {
    Serial.print(" ");
  }
  Serial.println("║");
  Serial.println("╚════════════════════════════════════════╝\n");
}

String normalizeFilePath(const char* filename) {
  String result = String(filename);
  if (!result.startsWith("/")) {
    result = "/" + result;
  }
  return result;
}

void centerDisplayText(int textSize, int y, const char* text) {
  display.setTextSize(textSize);
  display.setTextColor(SSD1306_WHITE);
  
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int xPos = (SCREEN_WIDTH - w) / 2;
  
  display.setCursor(xPos, y);
  display.println(text);
}

void formatTimeString(char* buffer, DateTime dt, bool includeSeconds) {
  if (includeSeconds) {
    snprintf(buffer, 20, "%04d-%02d-%02d %02d:%02d:%02d", 
             dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
  } else {
    snprintf(buffer, 17, "%04d-%02d-%02d %02d:%02d", 
             dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute());
  }
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
  
  // Initialize I2C for OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("✓ I2C initialized");
  
  // Initialize OLED Display
  Serial.println("\n--- OLED Display Initialization ---");
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("✗ SSD1306 allocation failed!");
    for(;;); // Don't proceed, loop forever
  }
  
  Serial.println("✓ OLED Display initialized");
  display.clearDisplay();
  display.display();
  
  displayStartupMessage();
  
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

  // --- Recover/initialize production state after reboot ---
  // Read the latching (production) button to decide whether to
  // restore the last count or reset to zero on startup.
  pinMode(LATCHING_PIN, INPUT_PULLUP); // ensure we can read the pin
  bool latchPressed = (digitalRead(LATCHING_PIN) == LOW); // LOW = pressed

  if (latchPressed) {
    // If the button is pressed during boot, assume production is active
    // and restore the last saved count so the display shows the previous value.
    productionActive = true;
    productionStartCount = 0; // show full saved count on screen
    productionCount = 0;
    if (rtcAvailable) {
      productionStartTime = rtc.now();
    } else {
      // Fallback time (compile-time date) if RTC isn't available
      productionStartTime = DateTime(2025, 11, 12, 0, 0, 0);
    }

    Serial.println("✓ Reboot: Latching pin pressed - resuming production (restored last count)");
    showStatus("Production Resumed", 1500);
    needsFullRedraw = true;
  } else {
    // If the button is NOT pressed, reset the count to zero on startup
    productionActive = false;
    noInterrupts();
    currentCount = 0;
    countChanged = true;
    interrupts();

    // Persist the reset count if SD is available
    if (sdAvailable) {
      writeCountToFile(COUNT_FILE, 0);
    }

    productionStartCount = 0;
    productionCount = 0;
    Serial.println("✓ Reboot: Latching pin not pressed - count reset to 0");
    showStatus("Count Reset", 1200);
    needsFullRedraw = true;
  }
  
  // Try to recover production session if power was lost unexpectedly
  // (This overrides the latch button logic if a saved session is found)
  restoreProductionState();
  
  // Setup interrupts
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleInterrupt, FALLING);
  
  pinMode(DIAGNOSTIC_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DIAGNOSTIC_PIN), handleDiagnosticButton, FALLING);
  
  pinMode(LATCHING_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(LATCHING_PIN), handleLatchingButton, CHANGE);
  
  Serial.println("\n✓ Interrupts configured");
  
  if (rtcAvailable) {
    DateTime now = rtc.now();
    lastHour = now.hour();
  } else {
    lastHour = 0;
  }
  
  needsFullRedraw = true;
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║          ✓✓✓ SYSTEM READY ✓✓✓          ║");
  Serial.println("║    OLED Display Configuration:         ║");
  Serial.println("║    • 128x64 pixels                     ║");
  Serial.println("║    • I2C interface (0x3C)              ║");
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
    
    // Handle production button state change
    if (productionStatusChanged) {
      productionStatusChanged = false;
      delay(50);  // Allow pin to settle after interrupt
      
      // Read the actual button state (LOW = pressed/ON, HIGH = released/OFF)
      bool buttonPressed = (digitalRead(LATCHING_PIN) == LOW);
      
      if (buttonPressed) {
        // Button is being held DOWN → START production
        startProduction();
      } else {
        // Button is released → STOP production
        stopProduction();
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
      
      // Also update production state file if production is active (for recovery)
      if (productionActive) {
        saveProductionState();
      }
    }
    
    // Update display
    if (countChanged || needsFullRedraw) {
      drawMainScreen();
      needsFullRedraw = false;
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
  
  // IMPORTANT: Only reset currentCount if production is NOT active
  // If production is active, we need to preserve the count differential
  if (!productionActive) {
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
  } else {
    // Production is active - just update cumulative without resetting
    Serial.println("⚠ Hour changed during production - production count preserved");
    needsFullRedraw = true;
  }
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
// SD CARD FILE MANAGEMENT FUNCTIONS
// ========================================

void listAllFiles() {
  if (!sdAvailable) {
    Serial.println("✗ SD Card not available");
    return;
  }
  
  printHeader("FILES ON SD CARD");
  
  SD_BEGIN();
  
  File root = SD.open("/");
  if (!root) {
    Serial.println("✗ Cannot open root directory");
    SD_END();
    return;
  }
  
  if (!root.isDirectory()) {
    Serial.println("✗ Root is not a directory");
    root.close();
    SD_END();
    return;
  }
  
  int fileCount = 0;
  File file = root.openNextFile();
  
  Serial.println("Index | Filename                          | Size (bytes)");
  Serial.println("─────┼──────────────────────────────────┼──────────────");
  
  while (file) {
    if (!file.isDirectory()) {
      fileCount++;
      Serial.print("  ");
      Serial.print(fileCount);
      Serial.print("   | ");
      
      const char* filename = file.name();
      Serial.print(filename);
      
      int filenameLen = strlen(filename);
      int padding = 35 - filenameLen;
      for (int i = 0; i < padding; i++) {
        Serial.print(" ");
      }
      
      Serial.print("| ");
      Serial.println(file.size());
    }
    file.close();
    file = root.openNextFile();
  }
  
  file.close();
  root.close();
  SD_END();
  
  printDivider();
  Serial.print("✓ Total files: ");
  Serial.println(fileCount);
  Serial.println();
}

void searchFiles(const char* searchPattern) {
  if (!sdAvailable) {
    Serial.println("✗ SD Card not available");
    return;
  }
  
  String patternUpper = String(searchPattern);
  patternUpper.toUpperCase();
  
  printHeader("SEARCHING FILES");
  
  SD_BEGIN();
  
  File root = SD.open("/");
  if (!root) {
    Serial.println("✗ Cannot open root directory");
    SD_END();
    return;
  }
  
  int matchCount = 0;
  File file = root.openNextFile();
  
  Serial.println("Matching Files:");
  
  while (file) {
    if (!file.isDirectory()) {
      const char* filename = file.name();
      String filenameUpper = String(filename);
      filenameUpper.toUpperCase();
      
      if (filenameUpper.indexOf(patternUpper) >= 0) {
        matchCount++;
        Serial.print("  ");
        Serial.print(matchCount);
        Serial.print(". ");
        Serial.print(filename);
        Serial.print(" (");
        Serial.print(file.size());
        Serial.println(" bytes)");
      }
    }
    file.close();
    file = root.openNextFile();
  }
  
  file.close();
  root.close();
  SD_END();
  
  printDivider();
  Serial.print("✓ Found: ");
  Serial.print(matchCount);
  Serial.println(" file(s)");
  Serial.println();
}

void readFile(const char* filename) {
  if (!sdAvailable) {
    Serial.println("✗ SD Card not available");
    return;
  }
  
  if (filename == NULL || strlen(filename) == 0) {
    Serial.println("✗ Invalid filename");
    return;
  }
  
  printHeader("READING FILE");
  
  SD_BEGIN();
  
  // Check if file exists
  if (!SD.exists(filename)) {
    Serial.print("✗ File not found: ");
    Serial.println(filename);
    SD_END();
    return;
  }
  
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.print("✗ Cannot open file: ");
    Serial.println(filename);
    SD_END();
    return;
  }
  
  // Get file size
  long fileSize = file.size();
  Serial.print("File size: ");
  Serial.print(fileSize);
  Serial.println(" bytes");
  printDivider();
  
  // Read and display file content
  long bytesRead = 0;
  int lineNumber = 1;
  
  while (file.available()) {
    String line = file.readStringUntil('\n');
    
    // Print line number for reference
    Serial.print(lineNumber);
    Serial.print(" | ");
    Serial.println(line);
    
    bytesRead += line.length() + 1;
    lineNumber++;
  }
  
  printDivider();
  Serial.print("✓ Read ");
  Serial.print(bytesRead);
  Serial.print(" bytes in ");
  Serial.print(lineNumber - 1);
  Serial.println(" lines");
  
  file.close();
  SD_END();
  
  Serial.println();
}

void readProductionFiles() {
  if (!sdAvailable) {
    Serial.println("✗ SD Card not available");
    return;
  }
  
  printHeader("PRODUCTION SESSION FILES");
  
  SD_BEGIN();
  
  File root = SD.open("/");
  if (!root) {
    Serial.println("✗ Cannot open root directory");
    SD_END();
    return;
  }
  
  int fileCount = 0;
  File file = root.openNextFile();
  
  Serial.println("Production Files Found:");
  
  while (file) {
    if (!file.isDirectory()) {
      const char* filename = file.name();
      
      // Check if file starts with "Production_"
      if (strncmp(filename, "Production_", 11) == 0) {
        fileCount++;
        Serial.print("  ");
        Serial.print(fileCount);
        Serial.print(". ");
        Serial.print(filename);
        Serial.print(" (");
        Serial.print(file.size());
        Serial.println(" bytes)");
      }
    }
    file.close();
    file = root.openNextFile();
  }
  
  file.close();
  root.close();
  SD_END();
  
  printDivider();
  Serial.print("✓ Total production files: ");
  Serial.println(fileCount);
  Serial.println();
}

void deleteFile(const char* filename) {
  if (!sdAvailable) {
    Serial.println("✗ SD Card not available");
    return;
  }
  
  if (filename == NULL || strlen(filename) == 0) {
    Serial.println("✗ Invalid filename");
    return;
  }
  
  SD_BEGIN();
  
  if (!SD.exists(filename)) {
    Serial.print("✗ File not found: ");
    Serial.println(filename);
    SD_END();
    return;
  }
  
  if (SD.remove(filename)) {
    Serial.print("✓ File deleted: ");
    Serial.println(filename);
  } else {
    Serial.print("✗ Failed to delete file: ");
    Serial.println(filename);
  }
  
  SD_END();
}

// ========================================
// PRODUCTION SESSION FUNCTIONS
// ========================================
void startProduction() {
  if (!rtcAvailable) {
    Serial.println("✗ Cannot start production: RTC not available");
    showStatus("RTC ERROR", 2000);
    return;
  }
  
  productionActive = true;
  productionStartTime = rtc.now();
  productionStartCount = currentCount;
  productionCount = 0;
  
  Serial.println("\n>>> PRODUCTION STARTED <<<");
  Serial.print("Start Time: ");
  char timeStr[20];
  formatTimeString(timeStr, productionStartTime, true);
  Serial.println(timeStr);
  
  showStatus("Production Started", 2000);
  needsFullRedraw = true;
  
  // Save state for recovery in case of power loss
  saveProductionState();
}

void stopProduction() {
  if (!productionActive) {
    return;
  }
  
  productionActive = false;
  productionStopTime = rtcAvailable ? rtc.now() : productionStartTime;
  
  noInterrupts();
  productionCount = currentCount - productionStartCount;
  interrupts();
  
  Serial.println("\n>>> PRODUCTION STOPPED <<<");
  Serial.print("Stop Time: ");
  char timeStr[20];
  formatTimeString(timeStr, productionStopTime, true);
  Serial.println(timeStr);
  Serial.print("Production Count: "); Serial.println(productionCount);
  
  showStatus("Production Stopped", 2000);
  
  // Save production session to file and clear state file
  if (sdAvailable) {
    saveProductionSession();
    clearProductionState();  // Session completed, no need to recover
  }
  
  needsFullRedraw = true;
}

void saveProductionSession() {
  // Create filename with production start and stop times
  char filename[64];
  snprintf(filename, sizeof(filename), "/Production_%04d%02d%02d_%02d%02d%02d_to_%02d%02d%02d.txt",
           productionStartTime.year(), productionStartTime.month(), productionStartTime.day(),
           productionStartTime.hour(), productionStartTime.minute(), productionStartTime.second(),
           productionStopTime.hour(), productionStopTime.minute(), productionStopTime.second());
  
  SD_BEGIN();
  
  // Delete if exists
  if (SD.exists(filename)) {
    SD.remove(filename);
  }
  
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    SD_END();
    Serial.print("✗ Failed to create production file: "); Serial.println(filename);
    return;
  }
  
  // Write production session data
  file.println("=== PRODUCTION SESSION ===");
  
  file.print("Production Started: ");
  char startStr[20];
  formatTimeString(startStr, productionStartTime, true);
  file.println(startStr);
  
  file.print("Production Stopped: ");
  char stopStr[20];
  formatTimeString(stopStr, productionStopTime, true);
  file.println(stopStr);
  
  file.print("Production Count: ");
  file.println(productionCount);
  
  file.flush();
  file.close();
  SD_END();
  
  Serial.print("✓ Production session saved: "); Serial.println(filename);
  
  // Also save hourly production count to separate file
  saveHourlyProductionCount();
}

void saveHourlyProductionCount() {
  // Create hourly production summary file
  char filename[64];
  DateTime now = rtcAvailable ? rtc.now() : productionStartTime;
  
  // Filename: HourlyProduction_YYYYMMDD.txt
  snprintf(filename, sizeof(filename), "/HourlyProduction_%04d%02d%02d.txt",
           now.year(), now.month(), now.day());
  
  SD_BEGIN();
  
  File file = SD.open(filename, FILE_APPEND);
  if (!file) {
    SD_END();
    Serial.print("✗ Failed to save hourly production count: "); Serial.println(filename);
    return;
  }
  
  // Append production session info to hourly file
  file.println("---");
  file.print("Session: ");
  char startStr[20];
  formatTimeString(startStr, productionStartTime, false);
  file.print(startStr);
  file.print(" to ");
  char stopStr[20];
  formatTimeString(stopStr, productionStopTime, false);
  file.println(stopStr);
  file.print("Count: ");
  file.println(productionCount);
  
  file.flush();
  file.close();
  SD_END();
  
  Serial.print("✓ Hourly production count saved to: "); Serial.println(filename);
}

// ========================================
// PRODUCTION SESSION STATE RECOVERY
// ========================================
// This saves the active production session to a state file.
// If power is lost during production, the session can be recovered on reboot.
void saveProductionState() {
  if (!sdAvailable) {
    return; // Silent fail if SD not available (data will be in memory)
  }
  
  if (!productionActive) {
    // Production not active, clear any saved state
    clearProductionState();
    return;
  }
  
  // Production is active - save current state
  SD_BEGIN();
  
  File file = SD.open(PRODUCTION_STATE_FILE, FILE_WRITE);
  if (!file) {
    SD_END();
    return;
  }
  
  // Format: currentCount|productionStartCount|year|month|day|hour|minute|second
  file.print(currentCount); file.println();
  file.print(productionStartCount); file.println();
  file.print(productionStartTime.year()); file.println();
  file.print(productionStartTime.month()); file.println();
  file.print(productionStartTime.day()); file.println();
  file.print(productionStartTime.hour()); file.println();
  file.print(productionStartTime.minute()); file.println();
  file.print(productionStartTime.second()); file.println();
  
  file.flush();
  file.close();
  SD_END();
}

// This restores a production session from the state file if it exists.
// Called during setup to recover from unexpected power loss.
void restoreProductionState() {
  if (!sdAvailable) {
    Serial.println("⚠ Cannot check for production recovery: SD card not available");
    return;
  }
  
  SD_BEGIN();
  
  if (!SD.exists(PRODUCTION_STATE_FILE)) {
    SD_END();
    return; // No saved state, normal startup
  }
  
  File file = SD.open(PRODUCTION_STATE_FILE, FILE_READ);
  if (!file) {
    SD_END();
    return;
  }
  
  // Read saved state
  int savedCurrentCount = file.parseInt();
  int savedStartCount = file.parseInt();
  int year = file.parseInt();
  int month = file.parseInt();
  int day = file.parseInt();
  int hour = file.parseInt();
  int minute = file.parseInt();
  int second = file.parseInt();
  
  file.close();
  SD_END();
  
  // Validate the data
  if (year >= 2020 && year <= 2100 && month >= 1 && month <= 12 && 
      day >= 1 && day <= 31 && hour <= 23 && minute <= 59 && second <= 59) {
    
    // Restore the production session
    productionActive = true;
    currentCount = savedCurrentCount;
    productionStartCount = savedStartCount;
    productionStartTime = DateTime(year, month, day, hour, minute, second);
    productionCount = currentCount - productionStartCount;
    
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   PRODUCTION SESSION RECOVERED FROM    ║");
    Serial.println("║       UNEXPECTED POWER LOSS            ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.print("✓ Restored count: "); Serial.println(productionCount);
    Serial.print("✓ Session start: ");
    char timeStr[20];
    formatTimeString(timeStr, productionStartTime, true);
    Serial.println(timeStr);
    
    showStatus("Production Recovered!", 2000);
    needsFullRedraw = true;
  } else {
    Serial.println("⚠ Corrupted production state file, skipping recovery");
  }
}

// Clear the production state file
void clearProductionState() {
  if (!sdAvailable) {
    return;
  }
  
  SD_BEGIN();
  if (SD.exists(PRODUCTION_STATE_FILE)) {
    SD.remove(PRODUCTION_STATE_FILE);
  }
  SD_END();
}

// ========================================
// OLED DISPLAY FUNCTIONS
// ========================================
void displayStartupMessage() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 20);
  display.println("COUNTER");
  display.setTextSize(1);
  display.setCursor(25, 45);
  display.println("Starting...");
  display.display();
  delay(1000);
}

void showStatus(const char* message, int duration) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 28);
  
  // Center text
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 28);
  
  display.println(message);
  display.display();
  delay(duration);
}

void drawMainScreen() {
  display.clearDisplay();
  
  // Get current time to display (always use latest RTC time)
  DateTime displayTime = rtcAvailable ? rtc.now() : productionStartTime;
  
  if (productionActive) {
    // ===== PRODUCTION ACTIVE DISPLAY =====
    // Status at top
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 0);
    display.println("Pro. Started");
    
    // EXTRA BIG production count (center) - Text size 5
    noInterrupts();
    int count = currentCount - productionStartCount;
    interrupts();
    
    if (count < 0) count = 0;
    
    char countStr[10];
    snprintf(countStr, sizeof(countStr), "%d", count);
    centerDisplayText(5, 12, countStr);
    
    // Time at bottom (HH.MM AM/PM format with space) - Show CURRENT time
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    int displayHour = getDisplay12Hour(displayTime.hour());
    const char* ampm = getAmPm(displayTime.hour());
    
    char timeStr[12];
    snprintf(timeStr, sizeof(timeStr), "%02d.%02d %s", 
             displayHour, displayTime.minute(), ampm);
    
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 56);
    display.println(timeStr);
    
  } else {
    // ===== PRODUCTION STOPPED DISPLAY =====
    // Status at top
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 0);
    display.println("Pro. Stopped");
    
    // EXTRA BIG count (last production count) - Text size 5
    char countStr[10];
    snprintf(countStr, sizeof(countStr), "%d", productionCount);
    centerDisplayText(5, 12, countStr);
    
    // Time at bottom (HH.MM AM/PM format with space) - Show CURRENT time
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    int displayHour = getDisplay12Hour(displayTime.hour());
    const char* ampm = getAmPm(displayTime.hour());
    
    char timeStr[12];
    snprintf(timeStr, sizeof(timeStr), "%02d.%02d %s", 
             displayHour, displayTime.minute(), ampm);
    
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 56);
    display.println(timeStr);
  }
  
  // Display everything
  display.display();
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
  // Format expected: TIME,YYYY,MM,DD,HH,MM,SS
  if (!input.startsWith("TIME,")) {
    return false;
  }
  
  String timeData = input.substring(5); // Skip "TIME,"
  int commaCount = 0;
  for (int i = 0; i < timeData.length(); i++) {
    if (timeData[i] == ',') commaCount++;
  }
  
  // Need exactly 5 commas for 6 values (YYYY,MM,DD,HH,MM,SS)
  if (commaCount != 5) {
    Serial.println("✗ Invalid time format. Expected: TIME,YYYY,MM,DD,HH,MM,SS");
    Serial.println("  Example: TIME,2025,11,15,14,30,45");
    return true;
  }
  
  int values[6] = {0};
  int index = 0;
  int value = 0;
  
  for (int i = 0; i < timeData.length(); i++) {
    char c = timeData[i];
    if (c >= '0' && c <= '9') {
      value = value * 10 + (c - '0');
    } else if (c == ',') {
      if (index < 6) {
        values[index++] = value;
        value = 0;
      }
    }
  }
  
  // Don't forget the last value after final comma
  if (index < 6) {
    values[index++] = value;
  }
  
  // Validate all 6 values parsed
  if (index != 6) {
    Serial.println("✗ Failed to parse all 6 time values");
    return true;
  }
  
  int year = values[0];
  int month = values[1];
  int day = values[2];
  int hour = values[3];
  int minute = values[4];
  int second = values[5];
  
  // Validate ranges
  if (year < 2020 || year > 2100) {
    Serial.print("✗ Year out of range: "); Serial.println(year);
    return true;
  }
  if (month < 1 || month > 12) {
    Serial.print("✗ Month out of range: "); Serial.println(month);
    return true;
  }
  if (day < 1 || day > 31) {
    Serial.print("✗ Day out of range: "); Serial.println(day);
    return true;
  }
  if (hour < 0 || hour > 23) {
    Serial.print("✗ Hour out of range: "); Serial.println(hour);
    return true;
  }
  if (minute < 0 || minute > 59) {
    Serial.print("✗ Minute out of range: "); Serial.println(minute);
    return true;
  }
  if (second < 0 || second > 59) {
    Serial.print("✗ Second out of range: "); Serial.println(second);
    return true;
  }
  
  // All valid - set the RTC
  DateTime newTime(year, month, day, hour, minute, second);
  rtc.adjust(newTime);
  
  Serial.print("✓ RTC time set to: ");
  Serial.print(year); Serial.print("-");
  Serial.print(month); Serial.print("-");
  Serial.print(day); Serial.print(" ");
  Serial.print(hour); Serial.print(":");
  Serial.print(minute); Serial.print(":");
  Serial.println(second);
  
  return true;
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
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║          PARAMETER COMMANDS            ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("  SET,1,<value>  - Change Save Interval (ms) [1000-60000]");
  Serial.println("  SET,2,<value>  - Change Debounce Delay (ms) [10-500]");
  Serial.println("  SET,3,<value>  - Change Max Count [100-99999]");
  Serial.println("  SET,4,<value>  - Change Status Display Time (ms) [1000-30000]");
  Serial.println("  DEBOUNCE,<value> - Quick set debounce delay (ms) [10-500]");
  Serial.println("  SAVE           - Save all settings to EEPROM");
  Serial.println("  RESET          - Reset Current Count to 0");
  Serial.println("  RESETPARAM     - Reset all parameters to defaults");
  Serial.println("  TIME,YYYY,MM,DD,HH,MM,SS - Set RTC time");
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║         FILE MANAGEMENT COMMANDS       ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("  LS             - List all files on SD card");
  Serial.println("  PROD           - List production session files");
  Serial.println("  SEARCH,<text>  - Search files by name/pattern");
  Serial.println("  READ,<filename> - Read file contents");
  Serial.println("  DEL,<filename> - Delete a file");
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║           OTHER COMMANDS               ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("  INFO           - Show this menu again");
  Serial.println("  STATUS         - Check SD card status");
  Serial.println("  REINIT         - Re-initialize SD card\n");
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
  
  // Check if SD card is available for file commands
  if (input == "LS" || input == "PROD" || input.startsWith("SEARCH,") || 
      input.startsWith("READ,") || input.startsWith("DEL,")) {
    if (!sdAvailable) {
      Serial.println("✗ SD Card not available!");
      Serial.println("  Check SD card connection and try again.");
      Serial.println("  Tip: Press GPIO 27 for diagnostics to test SD card");
      return true;
    }
  }
  
  // Check SD status
  if (input == "STATUS") {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║          SYSTEM STATUS CHECK          ║");
    Serial.println("╚════════════════════════════════════════╝\n");
    
    Serial.print("OLED Display:   ✓ OK (Connected)\n");
    Serial.print("RTC Module:     ");
    Serial.println(rtcAvailable ? "✓ OK" : "✗ NOT CONNECTED");
    Serial.print("SD Card:        ");
    Serial.println(sdAvailable ? "✓ READY" : "✗ NOT READY");
    
    if (sdAvailable) {
      SD_BEGIN();
      uint64_t cardSize = SD.cardSize() / (1024 * 1024);
      Serial.print("SD Card Size:   ");
      Serial.print(cardSize);
      Serial.println(" MB");
      SD_END();
    }
    Serial.println();
    return true;
  }
  
  // Re-initialize SD card
  if (input == "REINIT") {
    Serial.println("\nAttempting SD card re-initialization...");
    SD.end();
    delay(500);
    digitalWrite(SD_CS_PIN, HIGH);
    delay(500);
    
    sdAvailable = initializeSD();
    
    if (sdAvailable) {
      Serial.println("✓ SD card successfully re-initialized!");
      initializeFiles();
    } else {
      Serial.println("✗ SD card re-initialization FAILED");
    }
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
  
  // ===== FILE MANAGEMENT COMMANDS =====
  
  // List all files
  if (input == "LS") {
    listAllFiles();
    return true;
  }
  
  // List production files
  if (input == "PROD") {
    readProductionFiles();
    return true;
  }
  
  // Search files: SEARCH,pattern
  if (input.startsWith("SEARCH,")) {
    String pattern = input.substring(7); // Skip "SEARCH,"
    if (pattern.length() > 0) {
      searchFiles(pattern.c_str());
      return true;
    } else {
      Serial.println("✗ Invalid search pattern");
      Serial.println("  Usage: SEARCH,<pattern>");
      return true;
    }
  }
  
  // Read file: READ,filename
  if (input.startsWith("READ,")) {
    String filename = input.substring(5); // Skip "READ,"
    if (filename.length() > 0) {
      // Add leading slash if not present
      if (!filename.startsWith("/")) {
        filename = "/" + filename;
      }
      readFile(filename.c_str());
      return true;
    } else {
      Serial.println("✗ Invalid filename");
      Serial.println("  Usage: READ,<filename>");
      Serial.println("  Example: READ,Production_20251107_143025_to_143530.txt");
      return true;
    }
  }
  
  // Delete file: DEL,filename
  if (input.startsWith("DEL,")) {
    String filename = input.substring(4); // Skip "DEL,"
    if (filename.length() > 0) {
      // Add leading slash if not present
      if (!filename.startsWith("/")) {
        filename = "/" + filename;
      }
      
      Serial.print("⚠ Are you sure you want to delete: ");
      Serial.println(filename);
      Serial.println("  (This action cannot be undone)");
      
      // For safety, require confirmation by checking if file exists first
      if (SD.exists(filename.c_str())) {
        deleteFile(filename.c_str());
      } else {
        Serial.println("✗ File not found");
      }
      return true;
    } else {
      Serial.println("✗ Invalid filename");
      Serial.println("  Usage: DEL,<filename>");
      return true;
    }
  }
  
  return false;
}

// ========================================
// DIAGNOSTIC MODE
// ========================================
void enterDiagnosticMode() {
  currentMode = MODE_DIAGNOSTIC;
  Serial.println("\n>>> DIAGNOSTIC MODE <<<");
  
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(20, 2);
  display.println("DIAGNOSTIC");
  display.display();
  
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 20);
  display.println("Testing...");
  display.display();
  delay(500);
  
  runDiagnosticTests();
  displayDiagnosticResults();
  
  display.setCursor(15, 56);
  display.setTextSize(1);
  display.println("Press to exit");
  display.display();
  
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
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(20, 2);
  display.println("DIAGNOSTIC");
  display.setTextColor(SSD1306_WHITE);
  
  int y = 20;
  
  // Test OLED (already working if we can see this)
  testResults.oledOk = true;
  display.setCursor(5, y);
  display.print("OLED: PASS");
  display.display();
  y += 12;
  delay(300);
  
  // Test SD Card
  display.setCursor(5, y);
  testResults.sdOk = testSDCard();
  display.print("SD:   ");
  display.print(testResults.sdOk ? "PASS" : "FAIL");
  display.display();
  y += 12;
  delay(300);
  
  // Test RTC
  display.setCursor(5, y);
  testResults.rtcOk = testRTCModule();
  display.print("RTC:  ");
  display.print(testResults.rtcOk ? "PASS" : "FAIL");
  display.display();
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
  
  display.setCursor(5, 50);
  display.print("Score: ");
  display.print(pass);
  display.print("/3 (");
  display.print((pass * 100) / 3);
  display.print("%)");
  display.display();
}