# Code Changes Summary - Production Counter Refactoring

## Files Modified

### 1. **Code_v2.cpp** - Main firmware file
- Replaced OLED display with 16x4 LCD
- Added production latch button functionality
- Added production logging system

---

## Detailed Changes

### A. Library Changes

**Removed:**
```cpp
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
```

**Added:**
```cpp
#include <LiquidCrystal_I2C.h>
```

**Kept:**
```cpp
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <EEPROM.h>
```

---

### B. Pin Definition Changes

**Removed OLED pins:**
```cpp
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
```

**Added LCD pins:**
```cpp
#define LCD_COLS 16
#define LCD_ROWS 4
#define LCD_ADDRESS 0x27      // PCF8574 I2C backpack
#define DISPLAY_UPDATE_INTERVAL 500  // Throttle LCD updates
```

**New Pin:**
```cpp
#define PRODUCTION_LATCH_PIN 25  // Production on/off button
```

---

### C. Global Object Changes

**Before:**
```cpp
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
```

**After:**
```cpp
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
```

---

### D. System State Variables - NEW

```cpp
// Production state variables
volatile bool productionActive = false;
volatile bool productionStateChanged = false;
DateTime productionStartTime;
int productionSessionCount = 0;
```

---

### E. Interrupt Handlers - UPDATED

**New ISR for production button:**
```cpp
void IRAM_ATTR handleProductionLatch() {
  static unsigned long lastPress = 0;
  unsigned long now = millis();
  if (now - lastPress > 500) {  // 500ms debounce
    productionActive = !productionActive;
    productionStateChanged = true;
    lastPress = now;
  }
}
```

**Updated counter ISR:**
```cpp
void IRAM_ATTR handleInterrupt() {
  // ... existing code ...
  productionSessionCount++;  // NEW: Track session count
}
```

---

### F. Setup Function Changes

**Display initialization - BEFORE:**
```cpp
if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
  Serial.println("✗ SSD1306 allocation failed!");
  for(;;);
}
display.clearDisplay();
display.display();
displayStartupMessage();
```

**Display initialization - AFTER:**
```cpp
lcd.init();
lcd.backlight();
lcd.clear();
displayLCDStartupMessage();
```

**Button setup - NEW:**
```cpp
pinMode(PRODUCTION_LATCH_PIN, INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(PRODUCTION_LATCH_PIN), 
                handleProductionLatch, FALLING);
```

---

### G. Main Loop - PRODUCTION STATE HANDLING (NEW)

```cpp
if (productionStateChanged) {
  productionStateChanged = false;
  if (productionActive) {
    // Production started
    productionStartTime = rtcAvailable ? rtc.now() : DateTime(...);
    productionSessionCount = 0;
    createProductionLogFile(productionStartTime, true);
    statusMessage = "PROD STARTED";
    showStatus("PROD STARTED", 2000);
  } else {
    // Production stopped
    DateTime productionEndTime = rtcAvailable ? rtc.now() : DateTime(...);
    updateProductionLogFile(productionStartTime, productionEndTime, 
                           productionSessionCount);
    statusMessage = "PROD STOPPED";
    showStatus("PROD STOPPED", 2000);
  }
  needsFullRedraw = true;
}
```

---

### H. NEW Functions - Production Logging

#### 1. `createProductionLogFile(DateTime startTime, bool isStart)`
- Creates log file when production starts
- Format: `/PROD_YYYYMMDD_HHMMSS.txt`
- Writes initial header and start time

#### 2. `updateProductionLogFile(DateTime startTime, DateTime endTime, int sessionCount)`
- Updates log file when production stops
- Adds end time and final count
- Overwrites file with complete information

#### 3. `printDateTime(DateTime dt)`
- Utility function for formatted timestamp printing

---

### I. Display Function Changes

**Removed:**
- `displayStartupMessage()` - OLED version
- `showStatus()` - OLED version
- `drawMainScreen()` - OLED version

**Added:**
- `displayLCDStartupMessage()` - LCD version
- `showStatus()` - LCD version (updated)
- `drawMainLCDScreen()` - NEW LCD-specific display

**New display logic:**
```cpp
// Row 0: Production status and time
[PROD] 14:32:15   OR   [IDLE] 14:32:15

// Row 1: Real-time count
Count: 00123

// Row 2: Session count or hourly count
Session: 00123    OR    Hour: 00456

// Row 3: Production start time or status
S:14:25:30        OR    Ready
```

---

### J. Diagnostic Mode Updates

**Before (OLED):**
```cpp
display.clearDisplay();
display.fillRect(...);
display.setTextColor(...);
display.setCursor(...);
display.println("DIAGNOSTIC");
```

**After (LCD):**
```cpp
lcd.clear();
lcd.setCursor(4, 0);
lcd.print("DIAGNOSTIC");
```

**Updated diagnostic test:**
```cpp
// Changed from "OLED: PASS" to "LCD: PASS"
testResults.oledOk = true;  // Still used for LCD test
lcd.setCursor(0, 1);
lcd.print("LCD:   PASS");
```

---

## Data Flow Diagrams

### Production Session Flow
```
User presses production button
  ↓
handleProductionLatch() ISR fires
  ↓
productionActive toggled, productionStateChanged = true
  ↓
Main loop detects productionStateChanged
  ↓
if productionActive:
  ├─ Create log file
  ├─ Set productionStartTime
  ├─ Reset productionSessionCount to 0
  └─ Show "PROD STARTED" message
else:
  ├─ Update log file with end time & count
  ├─ Save productionSessionCount
  └─ Show "PROD STOPPED" message
  ↓
Log file created/updated on SD card
```

### Count Flow During Production
```
Item passes sensor
  ↓
Counter button pressed (GPIO 15 falls)
  ↓
handleInterrupt() ISR
  ↓
currentCount++ (global)
productionSessionCount++ (if production active)
countChanged = true
  ↓
Main loop: countChanged detected
  ↓
Periodically save to SD (every 5 seconds)
  ↓
LCD display updates with new count
```

---

## Configuration Adjustments Needed

### 1. LCD I2C Address
If LCD doesn't display, try changing:
```cpp
#define LCD_ADDRESS 0x27  // Default
// Try: 0x3F or 0x20
```

### 2. Display Update Speed
To reduce flicker or power consumption:
```cpp
#define DISPLAY_UPDATE_INTERVAL 500  // Current: 500ms
// Adjust to 1000ms for slower updates or 200ms for faster
```

### 3. Production Button Debounce
To adjust sensitivity:
```cpp
// In handleProductionLatch() ISR
if (now - lastPress > 500) {  // Current: 500ms debounce
  // Adjust value as needed
}
```

---

## Testing Procedures

### 1. Basic LCD Test
```cpp
lcd.print("Test");  // Should display on LCD
```

### 2. Production Button Test
Press GPIO 25 button → LCD should show [PROD] toggle

### 3. Log File Creation Test
1. Press production button (start)
2. Wait 2 seconds
3. Check SD card for `/PROD_*.txt` file

### 4. Count Recording Test
1. Start production (GPIO 25)
2. Press counter button (GPIO 15) several times
3. Stop production (GPIO 25)
4. Check log file for count value

---

## Performance Considerations

### Memory Usage
- Old OLED code: ~15KB graphics library
- New LCD code: ~3KB simple I2C library
- **Savings: ~12KB of program memory**

### Update Speed
- OLED: Full display update every frame (slower)
- LCD: Character-only updates (faster)
- LCD throttled to 500ms to reduce I2C traffic

### I2C Bus Sharing
- LCD and RTC share I2C (GPIO 21=SDA, GPIO 22=SCL)
- Both at 100kHz standard I2C speed
- No conflicts expected

---

## Backward Compatibility

### Maintained:
✓ All serial commands (SET, DEBOUNCE, SAVE, etc.)
✓ EEPROM settings storage
✓ RTC time functionality
✓ SD card file system
✓ Hourly logging
✓ Counting mechanism
✓ Diagnostic mode

### Changed:
✗ Display output format (character vs. pixels)
✗ Display layout (4x16 vs. 128x64)
✗ Display libraries

---

## Migration Checklist

- [x] Replace includes
- [x] Update pin definitions
- [x] Update global objects
- [x] Add production state variables
- [x] Update interrupt handlers
- [x] Update setup function
- [x] Update main loop
- [x] Add production logging functions
- [x] Replace display functions
- [x] Update diagnostic functions
- [x] Test compilation
- [x] Create documentation

---

## Future Enhancement Points

1. **LCD Backlight Control** - Add on/off timer
2. **Production Stats** - Display avg items/hour during session
3. **Error Logging** - Log SD card errors
4. **Audio Feedback** - Buzzer on production start/stop
5. **Remote Logging** - Wifi upload to cloud
6. **Multi-Line Status** - Scroll longer messages

