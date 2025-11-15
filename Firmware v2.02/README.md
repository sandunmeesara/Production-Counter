# ESP32 Production Counter System - Complete Documentation

## 📋 Table of Contents
1. [Project Overview](#project-overview)
2. [Hardware Setup](#hardware-setup)
3. [Features](#features)
4. [Installation & Upload](#installation--upload)
5. [Serial Commands](#serial-commands)
6. [File System](#file-system)
7. [Troubleshooting](#troubleshooting)
8. [Important Fixes](#important-fixes)

---

## 🎯 Project Overview

**ESP32 Production Counter** is an industrial counting system with real-time logging, OLED display, and SD card data persistence. It tracks production counts hourly, maintains cumulative totals, and can recover from power loss.

**Main Features:**
- Real-time production counting via GPIO interrupt
- 128x64 OLED display with live updates
- RTC (Real-Time Clock) for precise timestamping
- SD card logging with hourly summaries
- Power-loss recovery capability
- Serial debug menu with parameter control
- Production session tracking with start/stop times

---

## 🔧 Hardware Setup

### Pin Configuration

| Component | Pin | Function |
|-----------|-----|----------|
| **Counter Button** | GPIO 15 | Production count (FALLING interrupt) |
| **Diagnostic Button** | GPIO 27 | Enter diagnostic mode (FALLING interrupt) |
| **Production Button** | GPIO 25 | Start/Stop production (CHANGE interrupt) |
| **OLED SDA** | GPIO 21 | I2C Data |
| **OLED SCL** | GPIO 22 | I2C Clock |
| **SD CS** | GPIO 26 | SD Card Chip Select |
| **SD SCK** | GPIO 18 | VSPI Clock (SPI3) |
| **SD MOSI** | GPIO 23 | VSPI MOSI (SPI3) |
| **SD MISO** | GPIO 19 | VSPI MISO (SPI3) |

### Connections

**OLED Display (SSD1306):**
- I2C Address: `0x3C`
- Resolution: 128×64 pixels
- VCC → 3.3V, GND → GND

**RTC Module (DS3231):**
- I2C Address: `0x68`
- VCC → 3.3V, GND → GND

**SD Card (VSPI):**
- FAT32 format recommended
- CS pin controlled via GPIO 26

---

## ✨ Features

### 1. **Real-Time Production Counting**
- Press production button to START counting
- Press production button again to STOP
- Counter button increments count during production
- Display shows current production count in large font

### 2. **Hourly Automatic Logging**
- Creates `Hour_YYYYMMDD_HHMM.txt` file every hour
- Contains: hour details, hourly count, cumulative total
- Files organized by date and hour

### 3. **Production Sessions**
- Each production session saved as `Production_YYYYMMDD_HHMMSS_to_HHMMSS.txt`
- Contains: start time, stop time, session count

### 4. **Daily Summaries**
- `HourlyProduction_YYYYMMDD.txt` tracks all sessions per day
- Appends each session with time range and count

### 5. **Power-Loss Recovery**
- `prod_session.txt` stores active session state
- On reboot, automatically resumes incomplete production sessions
- Preserves count data during unexpected power loss

### 6. **OLED Display Modes**

**Production Active:**
```
Pro. Started

      156
      
  02.30 PM
```

**Production Stopped:**
```
Pro. Stopped

       87
       
  02.30 PM
```

---

## 📥 Installation & Upload

### 1. **Install Required Libraries**
In Arduino IDE, go to `Sketch → Include Library → Manage Libraries`:

- `Adafruit SSD1306` by Adafruit
- `Adafruit GFX Library` by Adafruit
- `RTClib` by Adafruit
- `SD` (built-in Arduino library)

### 2. **Board Settings**
- Board: `ESP32 Dev Module`
- Upload Speed: `921600`
- Flash Size: `4MB`
- Partition Scheme: `Default 4MB with spiffs`

### 3. **Upload Code**
1. Open `code_v3.cpp` in Arduino IDE
2. Select correct COM port
3. Click **Upload**
4. Wait for "Hard resetting via RTS pin..." message

### 4. **Verify**
Open Serial Monitor at **115200 baud**:
```
╔════════════════════════════════════════╗
║   ESP32 COUNTER - OLED 128x64 VERSION ║
╚════════════════════════════════════════╝

✓ I2C initialized
✓ OLED Display initialized
✓ SD Card initialized
✓ RTC responding
✓ System Ready
```

---

## 🖥️ Serial Commands

### Getting Help
```
INFO
```
Shows complete debug menu with all available commands.

### Setting Parameters

**1. Change Save Interval (how often to save count)**
```
SET,1,5000
```
Range: 1000-60000 ms (default: 5000)

**2. Change Debounce Delay (button debounce)**
```
SET,2,50
```
Range: 10-500 ms (default: 50)

**3. Change Max Count**
```
SET,3,9999
```
Range: 100-99999 (default: 9999)

**4. Change Status Display Time**
```
SET,4,3000
```
Range: 1000-30000 ms (default: 3000)

**Quick Debounce:**
```
DEBOUNCE,75
```

**Save Settings to EEPROM:**
```
SAVE
```
Settings persist after power cycle.

**Reset All Parameters:**
```
RESETPARAM
```

### Count Control

**Reset Current Count to Zero:**
```
RESET
```

**View Current Status:**
```
STATUS
```
Shows OLED, RTC, and SD card status.

### Time Management

**Set RTC Time:**
```
TIME,2025,11,15,14,30,45
```
Format: `TIME,YYYY,MM,DD,HH,MM,SS`

When you set a new time and the hour changes, the system automatically creates an hourly file.

### File Management

**List All Files:**
```
LS
```
Shows filename, size for each file on SD card.

**List Production Files:**
```
PROD
```
Shows only `Production_*.txt` files.

**Search Files:**
```
SEARCH,production
```
Case-insensitive search by filename pattern.

**Read File Contents:**
```
READ,Hour_20251115_1430.txt
```
Displays file contents line by line.

**Delete File:**
```
DEL,filename.txt
```
Permanently removes file from SD card.

### System Maintenance

**Re-initialize SD Card:**
```
REINIT
```
Useful if SD card disconnects. Reinitializes VSPI interface.

**Enter Diagnostic Mode:**
Press GPIO 27 button to test OLED, SD card, RTC.

---

## 📁 File System

### System Count Files (auto-managed)

| File | Purpose | Content |
|------|---------|---------|
| `/count.txt` | Current count | Single integer |
| `/hourly_count.txt` | Last hour's count | Single integer |
| `/cumulative_count.txt` | All-time total | Single integer |

**Updated:** Every 5 seconds during production or on hour change

### Production Session Files

**Format:** `/Production_YYYYMMDD_HHMMSS_to_HHMMSS.txt`

**Example:** `/Production_20251115_143022_to_143045.txt`

**Content:**
```
=== PRODUCTION SESSION ===
Production Started: 2025-11-15 14:30:22
Production Stopped: 2025-11-15 14:30:45
Production Count: 87
```

**Created:** When production stops

### Hourly Count Files (NEW FEATURE)

**Format:** `/Hour_YYYYMMDD_HHMM.txt`

**Example:** `/Hour_20251115_1430.txt` (Nov 15, 2025 at 2:30 PM)

**Content:**
```
Hour: 2025-11-15 14:00
Count: 156
Cumulative: 1845
```

**Created:** 
- When hour automatically changes
- When time is manually set via `TIME` command

### Daily Production Summary

**Format:** `/HourlyProduction_YYYYMMDD.txt`

**Example:** `/HourlyProduction_20251115.txt`

**Content:**
```
---
Session: 2025-11-15 08:00 to 2025-11-15 08:15
Count: 45
---
Session: 2025-11-15 14:30 to 2025-11-15 14:45
Count: 87
---
Session: 2025-11-15 16:20 to 2025-11-15 16:35
Count: 62
```

**Updated:** After each production session

### Recovery File (temporary)

**File:** `/prod_session.txt`

**Purpose:** Stores active production session state for power-loss recovery

**Deleted:** Automatically when production session completes normally

**Content Format:**
```
currentCount
productionStartCount
year
month
day
hour
minute
second
```

---

## 🔄 Workflow Example

### Daily Production Tracking

**8:00 AM**
- Hour changes → `Hour_20251115_0800.txt` created
- Count reset to 0

**8:15 AM**
- Start production (press production button)
- Count 45 items
- Stop production
- `Production_20251115_081500_to_081530.txt` created
- `HourlyProduction_20251115.txt` updated

**2:00 PM**
- Hour changes → `Hour_20251115_1400.txt` created
- Cumulative count continues accumulating
- Production session starts at 2:30 PM
- Count 87 items

**End of Day**
- All hourly files created automatically
- Daily summary file shows complete history

---

## 🛠️ Troubleshooting

### Problem: "SD Card not available"

**Solution:**
1. Check SD card connections (especially CS pin GPIO 26)
2. Format SD card as FAT32
3. Use command: `REINIT` to reinitialize
4. Try different SD card if issue persists

### Problem: Hour file not created

**Causes:**
- SD card not available
- Hour hasn't changed yet
- Check with `LS` command

**Solution:**
- Manually set time with `TIME` command
- System creates hourly file immediately when hour changes

### Problem: Can't read files (FILE NOT FOUND)

**Solution:**
1. Use `LS` command to list files
2. Copy exact filename from listing
3. Use `READ,filename.txt` command
4. Check SD card with `STATUS` command

### Problem: RTC shows wrong time

**Solution:**
```
TIME,2025,11,15,14,30,00
```
Set correct time using TIME command.

### Problem: Production count keeps resetting

**Solution:**
- Check debounce delay with `INFO`
- Verify counter button on GPIO 15
- Try increasing debounce: `SET,2,100`

### Problem: OLED display not showing anything

**Solution:**
1. Check I2C connections (GPIO 21/22)
2. Verify I2C address is 0x3C
3. Press GPIO 27 for diagnostics
4. Check display voltage (should be 3.3V)

---

## 🔒 Important Fixes Applied

### SD Card Corruption Fix
**Issue:** Files were visible but unreadable - "FILE NOT FOUND" errors

**Cause:** Manual CS pin toggling (`SD_BEGIN/SD_END` macros) interfered with SPI communication

**Fix:** Removed all manual CS control - SD library manages it internally

**Result:** All files now read/write reliably without corruption

### Hour Change Detection on Time Update
**Issue:** Hour files not created when time was set manually

**Fix:** Added automatic hour change detection after serial time update

**Result:** Hourly files created immediately when time changes

---

## 📊 Key Statistics

- **Memory:** Uses EEPROM for parameter persistence
- **SD Card Speed:** Auto-negotiates (400kHz, 1MHz, or 5MHz)
- **Update Rate:** 1 second display refresh
- **Count Limits:** 0-9999 per session
- **Button Debounce:** Configurable (10-500ms, default 50ms)
- **Save Interval:** Configurable (1-60 seconds, default 5 seconds)

---

## 🚀 Quick Start Guide

1. **Upload firmware** using Arduino IDE
2. **Type `INFO`** to see all commands
3. **Set time:** `TIME,2025,11,15,14,30,00`
4. **Press production button** to start
5. **Use counter button** to add items
6. **Press production button** to stop
7. **Type `LS`** to list files
8. **Type `READ,filename`** to view data

---

## 📞 Support

### Common Commands
- `INFO` - Show help menu
- `STATUS` - Check system health
- `LS` - List all files
- `REINIT` - Reinitialize SD card
- `RESET` - Reset count to 0

### Debug Info
- Press GPIO 27 for hardware diagnostics
- Check Serial Monitor at 115200 baud
- Use `STATUS` command for quick system check

---

## 📝 Version Info

**Firmware Version:** v3.0
**Last Updated:** November 15, 2025
**Board:** ESP32 Dev Module
**Status:** Production Ready ✅

---

## 📋 Changelog

### v3.0 - Current
- ✅ Fixed SD card file corruption issues
- ✅ Added hourly timestamped files (Hour_*.txt)
- ✅ Implemented power-loss recovery
- ✅ Added automatic hour change detection
- ✅ Enhanced serial command interface

### Known Limitations
- Max count: 9999 per session
- SD card: FAT32 format only
- RTC: DS3231 compatible only

---

**Happy Counting! 🎉**
