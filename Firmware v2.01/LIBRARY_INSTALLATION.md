# Library Requirements & Installation Guide

## Overview
This document lists all required libraries for the refactored Production Counter System using 16x4 LCD display.

---

## Required Libraries

### 1. **Core Libraries (Built-in with ESP32)**
- **Wire** - I2C communication
- **SPI** - SPI communication for SD card
- **SD** - SD card file system
- **EEPROM** - Persistent storage

No installation needed - included with ESP32 board support

### 2. **LiquidCrystal_I2C**
I2C interface library for 16x4 LCD panels with PCF8574 backpack

**Installation:**
1. Arduino IDE → Sketch → Include Library → Manage Libraries
2. Search: `LiquidCrystal I2C`
3. Install: **LiquidCrystal I2C by Frank de Brabander**
4. Version: Latest (tested with v1.1.2)

**Alternative names to search:**
- "LiquidCrystal_I2C"
- "LCD I2C"
- "16x4 LCD"

### 3. **RTClib**
Real-Time Clock library for DS3231 module

**Installation:**
1. Arduino IDE → Sketch → Include Library → Manage Libraries
2. Search: `RTClib`
3. Install: **RTClib by Adafruit**
4. Version: Latest (tested with v2.1.1)

### 4. **ESP32 Board Support**
Arduino core for ESP32

**Installation:**
1. Arduino IDE → File → Preferences
2. In "Additional Boards Manager URLs" add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Arduino IDE → Tools → Board → Board Manager
4. Search: `esp32`
5. Install: **esp32 by Espressif Systems**
6. Version: Latest (tested with v2.0.11)

---

## Installation Steps - Complete Guide

### Step 1: Install ESP32 Board Support
```
Arduino IDE → Preferences
Add URL: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
OK

Tools → Board → Board Manager
Search: esp32
Install: "esp32 by Espressif Systems"
Close
```

### Step 2: Install LiquidCrystal_I2C Library
```
Sketch → Include Library → Manage Libraries
Search: LiquidCrystal I2C
Install: "LiquidCrystal I2C by Frank de Brabander"
Close
```

### Step 3: Install RTClib
```
Sketch → Include Library → Manage Libraries
Search: RTClib
Install: "RTClib by Adafruit"
Close
```

### Step 4: Verify Installation
Copy this code to Arduino IDE and compile:
```cpp
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <SD.h>

LiquidCrystal_I2C lcd(0x27, 16, 4);
RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  Serial.println("Libraries loaded!");
}

void loop() {}
```

If it compiles without errors, all libraries are installed correctly.

---

## Board Selection

### In Arduino IDE:

**Select Board:**
```
Tools → Board → ESP32 Arduino → ESP32 Dev Module
```

**Select Port:**
```
Tools → Port → (Your ESP32 COM port, e.g., COM3)
```

**Select Upload Speed:**
```
Tools → Upload Speed → 115200
```

### Recommended Board Settings:
```
Board:           ESP32 Dev Module
Upload Speed:    115200
CPU Frequency:   240 MHz
Flash Frequency: 80 MHz
Flash Mode:      QIO
Flash Size:      4MB (32Mb)
Partition Scheme: Default 4MB with spiffs
Core Debug Level: None
PSRAM:           Disabled
```

---

## Library Includes Reference

### What Each Library Provides:

```cpp
#include <Wire.h>
// Purpose: I2C communication
// Provides: I2C master/slave functionality
// Used for: LCD and RTC communication
```

```cpp
#include <LiquidCrystal_I2C.h>
// Purpose: I2C LCD control library
// Provides: LCD 16x4 character display functions
// Main functions:
//   - lcd.init()
//   - lcd.backlight()
//   - lcd.print()
//   - lcd.setCursor()
//   - lcd.clear()
```

```cpp
#include <RTClib.h>
// Purpose: DS3231 Real-Time Clock control
// Provides: DateTime and RTC functions
// Main functions:
//   - rtc.now()
//   - rtc.adjust()
//   - rtc.begin()
```

```cpp
#include <SPI.h>
// Purpose: SPI communication
// Provides: SPI bus control
// Used for: SD card communication
```

```cpp
#include <SD.h>
// Purpose: SD card file system
// Provides: File read/write operations
// Main functions:
//   - SD.begin()
//   - SD.open()
//   - SD.exists()
//   - SD.remove()
```

```cpp
#include <EEPROM.h>
// Purpose: Persistent storage on ESP32
// Provides: Non-volatile memory operations
// Main functions:
//   - EEPROM.write()
//   - EEPROM.read()
//   - EEPROM.commit()
```

---

## Dependency Tree

```
Production Counter Firmware
│
├─ Wire.h
│  └─ Used by: LiquidCrystal_I2C, RTClib
│
├─ LiquidCrystal_I2C.h
│  └─ Depends on: Wire.h
│
├─ RTClib.h
│  └─ Depends on: Wire.h
│
├─ SPI.h
│  └─ Used by: SD.h
│
├─ SD.h
│  └─ Depends on: SPI.h
│
└─ EEPROM.h
   └─ No dependencies
```

---

## Troubleshooting Installation

### "LiquidCrystal_I2C not found"
**Solution:**
1. Verify library is installed: Sketch → Include Library
2. Verify spelling: `LiquidCrystal_I2C` (note underscore, not dash)
3. Check library version is compatible
4. Restart Arduino IDE

### "RTClib not found"
**Solution:**
1. Verify Adafruit RTClib is installed (not another RTC library)
2. Check in: Sketch → Include Library (should be listed)
3. Ensure you didn't install an old version
4. Update to latest version

### "ESP32 board not found"
**Solution:**
1. Check board URL is correct in Preferences
2. Go to Tools → Board → Board Manager
3. Search for "esp32"
4. Click Install for "esp32 by Espressif Systems"
5. Restart Arduino IDE

### Compilation errors with includes
**Solution:**
```
Sketch → Include Library → Manage Libraries
Type "esp32 core" and verify it's installed
If not, install "esp32 by Espressif Systems"
Select the board again: Tools → Board → ESP32 Arduino → ESP32 Dev Module
```

---

## Version Compatibility

### Tested Versions:

| Library | Version | Status |
|---------|---------|--------|
| LiquidCrystal_I2C | 1.1.2 | ✓ Tested |
| RTClib | 2.1.1 | ✓ Tested |
| ESP32 Board | 2.0.11 | ✓ Tested |
| Arduino IDE | 1.8.19+ | ✓ Supported |
| Arduino IDE | 2.0+ | ✓ Supported |

### Minimum Versions:

| Library | Minimum |
|---------|---------|
| LiquidCrystal_I2C | 1.1.0 |
| RTClib | 2.0.0 |
| ESP32 Board | 2.0.0 |
| Arduino IDE | 1.8.16 |

---

## Quick Install Script (Windows PowerShell)

If you prefer command-line installation, you can use the Arduino CLI:

```powershell
# Install ESP32 board
arduino-cli core install esp32:esp32

# Install libraries
arduino-cli lib install "LiquidCrystal_I2C"
arduino-cli lib install "RTClib"

# List installed
arduino-cli lib list
```

---

## Manual Library Installation (Advanced)

If automatic installation fails:

### 1. Download Libraries
- [LiquidCrystal_I2C](https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library)
- [RTClib](https://github.com/adafruit/RTClib)

### 2. Extract to Arduino Libraries Folder
```
Windows: C:\Users\YourUsername\Documents\Arduino\libraries\
Linux: ~/Arduino/libraries/
Mac: ~/Documents/Arduino/libraries/
```

### 3. Folder Structure
```
libraries/
├── LiquidCrystal_I2C/
│   ├── LiquidCrystal_I2C.h
│   ├── LiquidCrystal_I2C.cpp
│   └── ...
│
└── RTClib/
    ├── RTClib.h
    ├── RTClib.cpp
    └── ...
```

### 4. Restart Arduino IDE

---

## Verification Checklist

After installation, verify:

- [ ] Arduino IDE opens without errors
- [ ] Tools → Board shows "ESP32 Dev Module" available
- [ ] Tools → Port shows your COM port
- [ ] Sketch → Include Library shows all three libraries
- [ ] Sample code compiles without errors
- [ ] ESP32 can be programmed (blinks LED test)

---

## Support & Documentation

### Official Resources:
- [LiquidCrystal_I2C GitHub](https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library)
- [RTClib Documentation](https://adafruit.com/product/3295)
- [ESP32 Documentation](https://docs.espressif.com/)

### Debugging:
If you encounter issues:
1. Check Arduino IDE → Tools → Board Manager is correct
2. Verify Tools → Port is selected
3. Check serial output for error messages
4. Review library installation using "Manage Libraries"
5. Restart Arduino IDE if needed

---

## Next Steps

After successful installation:
1. Open `Code_v2.cpp` in Arduino IDE
2. Select Board: ESP32 Dev Module
3. Select Port: (your ESP32 port)
4. Click Upload
5. Check Serial Monitor (115200 baud) for output

---

**Last Updated:** November 6, 2025
**System:** ESP32 Production Counter v2.0 (LCD Edition)
