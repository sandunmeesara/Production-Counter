# Production Counter System - Refactoring Guide

## Overview
The ESP32 Production Counter has been refactored from OLED 128x64 display to a 16x4 LCD panel with production latching button support.

---

## Key Changes

### 1. **Display Hardware**
| Aspect | Old (OLED) | New (LCD) |
|--------|-----------|----------|
| Display Type | OLED 128x64 | LCD 16x4 |
| Interface | I2C (0x3C) | I2C (0x27/0x3F) |
| Libraries | Adafruit_SSD1306, Adafruit_GFX | LiquidCrystal_I2C |
| Pin Configuration | I2C only | I2C only |

**LCD Address Notes:**
- Default: `0x27` (most common)
- Alternative: `0x3F` or `0x20` depending on backpack type
- Adjust `LCD_ADDRESS` definition if needed

### 2. **New Production Control Pin**
- **GPIO 25**: Production Latching Button (to GND)
- When toggled ON: Creates production log file with start timestamp
- When toggled OFF: Updates log file with end timestamp and count
- Button debounce: 500ms

### 3. **Production Logging System**

#### Log File Creation
When production starts, a file is created with format:
```
/PROD_YYYYMMDD_HHMMSS.txt
```

**Log File Contents:**
```
=== PRODUCTION LOG ===
Started: YYYY-MM-DD HH:MM:SS
Ended: YYYY-MM-DD HH:MM:SS
Count: 12345
```

#### New Global Variables
```cpp
volatile bool productionActive = false;
volatile bool productionStateChanged = false;
DateTime productionStartTime;
int productionSessionCount = 0;
```

#### New Functions
- `createProductionLogFile(DateTime startTime, bool isStart)`
  - Creates log file when production starts
  
- `updateProductionLogFile(DateTime startTime, DateTime endTime, int sessionCount)`
  - Updates log file when production stops
  
- `printDateTime(DateTime dt)`
  - Utility function for printing timestamps

---

## LCD Display Layout

The 16x4 LCD displays information across 4 rows:

```
Row 0: [PROD] HH:MM:SS   (or [IDLE] HH:MM:SS)
Row 1: Count: 00123
Row 2: Session: 00456     (or Hour: 00789 if idle)
Row 3: S:HH:MM:SS         (or Status message if idle)
```

### Display Modes

**When Production is ACTIVE:**
```
[PROD] 14:32:15
Count: 00145
Session: 00145
S:14:25:30
```

**When Production is IDLE:**
```
[IDLE] 14:32:15
Count: 00145
Hour: 00156
Ready
```

---

## Pin Configuration

### GPIO Assignments
| Pin | Function | Type | Notes |
|-----|----------|------|-------|
| 15 | Counter Button | Input | Main counting interrupt |
| 25 | Production Latch | Input | Toggle production ON/OFF |
| 27 | Diagnostic Button | Input | Enter diagnostic mode |
| 18 | SD SPI Clock | SPI | VSPI (SPI3) |
| 19 | SD SPI MISO | SPI | VSPI (SPI3) |
| 23 | SD SPI MOSI | SPI | VSPI (SPI3) |
| 26 | SD Chip Select | Output | VSPI (SPI3) |
| 21 | I2C SDA | I2C | For LCD & RTC |
| 22 | I2C SCL | I2C | For LCD & RTC |

---

## Hardware Requirements

### Required Components
1. **ESP32 Development Board**
2. **16x4 LCD Panel with PCF8574 I2C Backpack**
   - Backpack address: 0x27 or 0x3F
   - SDA/SCL pull-ups (usually included on backpack)
3. **DS3231 RTC Module**
   - I2C interface
   - Battery-backed real-time clock
4. **SD Card Module**
   - VSPI compatible
   - Voltage level shifting recommended (ESP32 is 3.3V)
5. **Push Buttons** (x3)
   - Counter button (GPIO 15 to GND)
   - Production latch button (GPIO 25 to GND)
   - Diagnostic button (GPIO 27 to GND)
6. **Power Supply**
   - 5V for ESP32 and SD module
   - 3.3V for I2C devices (LCD, RTC)

---

## Library Installation

### Arduino IDE Library Manager
Install these libraries:
1. **Wire** (included with ESP32 board support)
2. **LiquidCrystal_I2C** (by Frank de Brabander)
3. **RTClib** (by Adafruit)
4. **SD** (included with ESP32 board support)
5. **EEPROM** (included with ESP32 board support)

### ESP32 Board Support
- Install "ESP32 by Espressif Systems" from Board Manager

---

## Wiring Diagram

```
ESP32 Pin Layout:
┌─────────────────────────────────┐
│ GND  21(SDA)  22(SCL)           │
│ 3.3V 18(SCK)  19(MISO)  23(MOSI)│
│ 5V   26(CS)   27(DIAG)  25(PROD)│
│      15(COUNT)                  │
└─────────────────────────────────┘

I2C Devices (SDA=21, SCL=22):
├── LCD 16x4 (Addr: 0x27)
└── RTC DS3231 (Addr: 0x68)

SPI Devices (VSPI/SPI3):
└── SD Card Module

Push Buttons:
├── GPIO 15 → Counter (to GND)
├── GPIO 25 → Production Latch (to GND)
└── GPIO 27 → Diagnostic (to GND)
```

---

## Serial Commands

The system still supports all previous serial commands:

### Display Functions
```
INFO              - Show debug menu
SET,1,<value>     - Change save interval (1000-60000 ms)
SET,2,<value>     - Change debounce delay (10-500 ms)
SET,3,<value>     - Change max count (100-99999)
SET,4,<value>     - Change status display time (1000-30000 ms)
DEBOUNCE,<value>  - Quick set debounce (10-500 ms)
SAVE              - Save settings to EEPROM
RESET             - Reset current count to 0
RESETPARAM        - Reset all parameters to defaults
TIME,YYYY,MM,DD,HH,MM,SS - Set RTC time
```

### Example: Set RTC Time
```
TIME,2025,11,06,14,32,15
```

---

## Functionality Summary

### Core Features
✓ Real-time production counting via interrupt on GPIO 15
✓ Production session logging with timestamps
✓ LCD 16x4 display with dual-mode display (production/idle)
✓ Hourly count tracking and logging
✓ SD card logging (production sessions + hourly logs)
✓ RTC (Real-Time Clock) time synchronization
✓ Diagnostic mode for testing components
✓ EEPROM-based parameter storage
✓ Serial command interface

### Production Session Workflow
1. **Production OFF** → Display shows [IDLE] mode
2. **Press Production Latch Button** → Production ON
   - Creates log file: `PROD_YYYYMMDD_HHMMSS.txt`
   - LCD shows [PROD] mode
   - Session count resets to 0
   - Real-time count continues accumulating
3. **Items Counted** → Session count increments
4. **Press Production Latch Button** → Production OFF
   - Updates log file with end time and count
   - LCD shows [IDLE] mode
   - Session information preserved in log file
5. **Log file saved** on SD card

---

## File Structure on SD Card

```
SD_CARD_ROOT/
├── count.txt              (current session count)
├── hourly_count.txt       (last hour's count)
├── cumulative_count.txt   (total cumulative count)
├── PROD_20251106_143030.txt   (production session 1)
├── PROD_20251106_153045.txt   (production session 2)
└── YYYYMMDD_HHMM[AM|PM].txt   (hourly logs)
```

---

## Troubleshooting

### LCD Not Displaying
1. Check I2C address (default 0x27, try 0x3F)
2. Verify SDA/SCL connections
3. Check LCD contrast/brightness potentiometer
4. Verify 5V power to LCD backpack

### Production Button Not Responding
1. Verify GPIO 25 connection to GND
2. Check debounce delay setting (default 500ms)
3. Test with diagnostic mode

### SD Card Not Working
1. Format SD card to FAT32
2. Check CS pin (GPIO 26) connection
3. Try slower SPI speed (system tries 400kHz → 1MHz → 5MHz)
4. Verify voltage levels (3.3V signals)

### RTC Time Loss
1. Check battery on DS3231 module
2. Verify I2C connection (SDA 21, SCL 22)
3. Set time via serial command after power cycle

---

## Modifications from Previous Version

### Removed
- OLED display support
- OLED-specific functions (displayStartupMessage, drawMainScreen with OLED graphics)
- Adafruit graphics library dependencies

### Added
- 16x4 LCD support via LiquidCrystal_I2C
- Production latch button (GPIO 25)
- Production session logging functions
- Production state tracking variables
- Production-aware display modes

### Updated
- Main display function: `drawMainLCDScreen()` (replaces `drawMainScreen()`)
- Startup message: `displayLCDStartupMessage()`
- Diagnostic mode display functions
- Main loop production state handling
- Status display function for LCD

---

## LED Indicators (Optional)

You can add LED indicators by modifying the code:
- Red LED: Production Active (GPIO XX)
- Green LED: System Ready (GPIO XX)

---

## Development Notes

- Code tested on ESP32-DEVKIT-V1
- LCD library: LiquidCrystal_I2C for I2C interface
- All interrupts are debounced to prevent false triggers
- Production state changes are tracked with flags for reliable state management
- Display updates are throttled (500ms minimum) to prevent flicker

---

## Future Enhancements

Potential improvements:
1. LCD backlight auto-off timer
2. SD card activity indicator
3. Buzzer/alarm on session start/end
4. Wifi logging to cloud service
5. Web interface for remote monitoring
6. SD card data export via USB
7. Multi-language support

---

## Support

For issues or questions:
1. Check the troubleshooting section above
2. Verify all connections match the wiring diagram
3. Test individual components in diagnostic mode
4. Check serial output for error messages
5. Review SD card for log files and error logs

