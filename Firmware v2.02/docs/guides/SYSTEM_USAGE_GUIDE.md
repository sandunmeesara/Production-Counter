# ESP32 Production Counter - Complete User Guide

## Table of Contents
1. [System Overview](#system-overview)
2. [Hardware Setup](#hardware-setup)
3. [Getting Started](#getting-started)
4. [Daily Operation](#daily-operation)
5. [Serial Commands](#serial-commands)
6. [File Management](#file-management)
7. [Data Files Explained](#data-files-explained)
8. [Troubleshooting](#troubleshooting)

---

## System Overview

The ESP32 Production Counter is an automated system that counts production units and logs data with timestamps. It features:

- **Real-time counting** with interrupt-based GPIO detection
- **OLED display** showing current counts and time
- **Hourly tracking** with automatic cumulative counting
- **SD card logging** for data persistence
- **Serial interface** for configuration and debugging
- **Power loss recovery** - automatic restoration of production sessions
- **3-attempt startup** with error detection

### Key Features
✅ Interrupt-driven counting (no missed counts)
✅ Automatic time synchronization via RTC
✅ Persistent storage on SD card
✅ Real-time OLED feedback
✅ Production session recovery
✅ Configurable parameters
✅ Diagnostic mode

---

## Hardware Setup

### Components Required
1. **ESP32 Development Board**
2. **SSD1306 OLED Display** (128x64, I2C)
3. **DS3231 Real-Time Clock Module** (I2C)
4. **SD Card Reader Module** (SPI)
5. **3 Push Buttons** (for counting, diagnostics, production control)
6. **Micro SD Card** (any capacity, FAT32 format)

### Pin Connections

#### I2C Pins (OLED + RTC)
```
ESP32 GPIO 21 (SDA) → OLED SDA + RTC SDA
ESP32 GPIO 22 (SCL) → OLED SCL + RTC SCL
ESP32 GND → OLED GND + RTC GND
ESP32 3.3V → OLED VCC + RTC VCC
```

#### SPI Pins (SD Card)
```
ESP32 GPIO 18 (SCK)  → SD Card CLK
ESP32 GPIO 19 (MISO) → SD Card MISO
ESP32 GPIO 23 (MOSI) → SD Card MOSI
ESP32 GPIO 26 (CS)   → SD Card CS
ESP32 GND → SD Card GND
ESP32 3.3V → SD Card VCC
```

#### Button Pins
```
GPIO 15 → Production Counter Button (to GND)
GPIO 27 → Diagnostic Button (to GND)
GPIO 25 → Production Latch Button (to GND)
```

---

## Getting Started

### Step 1: Upload Firmware

1. Open Arduino IDE
2. Select **Tools → Board → ESP32**
3. Select **Upload Speed: 921600**
4. Select **Flash Size: 4MB**
5. Open `code_v3.cpp`
6. Click **Upload**
7. Wait for compilation and upload to complete

### Step 2: Initial Setup

When the system boots:
- OLED shows "Starting..."
- System attempts to initialize (up to 3 retries)
- Serial monitor shows initialization status
- If successful, displays "SYSTEM READY"

### Step 3: Set the Time

Open **Serial Monitor** (Tools → Serial Monitor, 115200 baud):

```
Type: TIME,2025,11,15,14,30,45
(Format: TIME,YYYY,MM,DD,HH,MM,SS)

Output:
✓ RTC time set to: 2025-11-15 14:30:45
```

### Step 4: Check Startup Status

```
Type: STATUS

Output:
OLED Display:   ✓ OK (Connected)
RTC Module:     ✓ OK
SD Card:        ✓ READY
SD Card Size:   [Size in MB]
```

---

## Daily Operation

### Starting Production

1. **Press GPIO 25 Button (Production Latch)**
   - OLED displays "Pro. Started"
   - System begins counting
   - count.txt is created/updated

2. **Watch the Display**
   - Top: "Pro. Started"
   - Center: Large current count
   - Bottom: Current time (HH.MM format)

### Counting Units

1. **Press GPIO 15 Button** for each unit produced
   - Display updates in real-time
   - Count saved every 5 seconds
   - Maximum: 9999 units per session

2. **No manual input needed** - all saving is automatic

### Stopping Production

1. **Release GPIO 25 Button (Production Latch)**
   - OLED displays "Pro. Stopped"
   - System stops counting
   - Production session saved to file

2. **System Logs:**
   - Production start/stop times
   - Total units counted
   - Session saved to SD card

### Automatic Hourly Updates

When the hour changes (e.g., 14:59 → 15:00):

1. **Cumulative count updated** in `cumulative_count.txt`
2. **Session data preserved** if production is active
3. **Display refreshes** with new data
4. **Serial shows:** Hour change message

---

## Serial Commands

### Debug Menu

Type `INFO` to see all available commands:

```
╔════════════════════════════════════════╗
║        DEBUG MENU - PARAMETERS         ║
╚════════════════════════════════════════╝

Current Settings:
  1. Save Interval:        5000 ms
  2. Debounce Delay:       50 ms
  3. Max Count:            9999
  4. Status Display Time:  3000 ms

Count Status:
  Current Count:           145
  Hourly Count:            89
  Cumulative Count:        234
```

### Parameter Configuration Commands

#### Set Save Interval (how often count is saved)
```
SET,1,5000    → Save every 5 seconds (1000-60000 ms)
SET,1,10000   → Save every 10 seconds
```

#### Set Debounce Delay (button bounce protection)
```
SET,2,50      → 50ms debounce (10-500 ms)
SET,2,30      → 30ms debounce (more responsive)
```

#### Set Maximum Count
```
SET,3,9999    → Maximum 9999 units (100-99999)
SET,3,1000    → Maximum 1000 units
```

#### Set Status Display Duration
```
SET,4,3000    → Show status for 3 seconds (1000-30000 ms)
SET,4,2000    → Show status for 2 seconds
```

#### Quick Debounce Adjustment
```
DEBOUNCE,50   → Set debounce directly
DEBOUNCE,30   → Fast response counting
```

#### Save Settings to Memory
```
SAVE          → Save all parameters to EEPROM
              (persists after power cycle)
```

#### Reset Parameters to Defaults
```
RESETPARAM    → Reset all to factory defaults
```

#### Reset Current Count to Zero
```
RESET         → Set current count to 0
```

---

## File Management Commands

### List All Files
```
LS            → Show all files on SD card
              
Output:
Index | Filename                          | Size (bytes)
  1   | count.txt                         | 4
  2   | cumulative_count.txt              | 4
  3   | hourly_count.txt                  | 3
  4   | Production_2025-11-15_14h30m-...  | 156
  5   | DailyProduction_2025-11-15.txt    | 450
```

### Search Files
```
SEARCH,production    → Find files with "production" in name
SEARCH,2025-11-15    → Find files from specific date
```

### Read File Contents
```
READ,count.txt                      → Show current count
READ,cumulative_count.txt           → Show cumulative total
READ,DailyProduction_2025-11-15.txt → Show daily summary
```

Example output:
```
File size: 156 bytes
─────────────────────────────────────────────
1 | === PRODUCTION SESSION ===
2 | Production Started: 2025-11-15 14:30:45
3 | Production Stopped: 2025-11-15 16:45:30
4 | Production Count: 523
─────────────────────────────────────────────
✓ Read 156 bytes in 4 lines
```

### List Production Files
```
PROD          → Show all production session files
              
Output: Shows all files starting with "Production_"
```

### Delete Files
```
DEL,oldfile.txt       → Delete specific file
DEL,Production_*.txt  → Delete production file
```

### SD Card Status and Reinitialization
```
STATUS        → Check SD card connection and size
REINIT        → Re-initialize SD card (if disconnected)
```

---

## Data Files Explained

### 1. **count.txt**
- **Purpose:** Current production count
- **Updated:** Every 5 seconds during production
- **Resets:** Every hour (or when production stops)
- **Content:** Single number (0-9999)
- **Example:** `145`

### 2. **hourly_count.txt**
- **Purpose:** Count from last completed hour
- **Updated:** Only when production stops OR hour changes while stopped
- **Content:** Single number
- **Example:** `89`
- **Note:** Preserves the count from the last hour worked

### 3. **cumulative_count.txt**
- **Purpose:** Total of all hours combined (lifetime total)
- **Updated:** Every hour change (automatic)
- **Never resets** (keeps running total)
- **Content:** Single number
- **Example:** `1250`

### 4. **Production_2025-11-15_14h30m-16h45m.txt**
- **Purpose:** Session details for each production run
- **Created:** When you stop production (release latch button)
- **Filename:** Shows date and start-to-stop time
- **Content:**
  ```
  === PRODUCTION SESSION ===
  Production Started: 2025-11-15 14:30:45
  Production Stopped: 2025-11-15 16:45:30
  Production Count: 523
  ```
- **Multiple files:** One file per production session

### 5. **DailyProduction_2025-11-15.txt**
- **Purpose:** Daily summary of all production sessions
- **Created:** First time production stops on that day
- **Updated:** Each time production stops (appends data)
- **Content:** All sessions for that day
  ```
  ---
  Session: 2025-11-15 08h00m to 2025-11-15 12h30m
  Count: 450
  ---
  Session: 2025-11-15 13h00m to 2025-11-15 17h00m
  Count: 523
  ```

### 6. **prod_session.txt**
- **Purpose:** Recovery file (internal use)
- **Created:** During active production
- **Deleted:** When production stops
- **Use:** If power fails during production, this file recovers the session on next boot

---

## Troubleshooting

### Issue: OLED Display Not Showing

**Symptoms:** Black screen, no startup message

**Solutions:**
1. Check I2C connections (GPIO 21, 22)
2. Verify OLED power (3.3V, GND)
3. Confirm I2C address is 0x3C:
   ```
   Type: STATUS
   Should show: OLED Display:   ✓ OK (Connected)
   ```
4. Run diagnostics: Press GPIO 27 button

### Issue: RTC Module Not Recognized

**Symptoms:** Serial shows "RTC not responding"

**Solutions:**
1. Check I2C connections (GPIO 21, 22)
2. Verify RTC power and ground
3. Confirm DS3231 I2C address is 0x68
4. Try command: `STATUS`

### Issue: SD Card Not Detected

**Symptoms:** "SD Card not available" in serial monitor

**Solutions:**
1. Ensure SD card is FAT32 formatted
2. Check all SPI connections (GPIO 18, 19, 23, 26)
3. Try reinitializing: `REINIT`
4. Verify SD card works in computer
5. Check power supply (5V/3.3V)

### Issue: Count Not Updating

**Symptoms:** OLED and count.txt not changing

**Solutions:**
1. Ensure production is started (press GPIO 25)
2. Check GPIO 15 button connections
3. Adjust debounce: `SET,2,30` (lower value)
4. Test diagnostics: Press GPIO 27 button
5. Try: `RESET` to clear any stuck state

### Issue: Time Not Saved

**Symptoms:** Time reverts after reboot

**Solutions:**
1. Check RTC battery (CR2032)
2. Verify RTC power connections
3. Confirm command format: `TIME,2025,11,15,14,30,45`
4. Check serial output for error messages

### Issue: Cumulative Count Incorrect

**Symptoms:** Cumulative total doesn't match expected value

**Solutions:**
1. Check `READ,cumulative_count.txt`
2. Verify production sessions: `READ,DailyProduction_2025-11-15.txt`
3. Reset if needed: `RESET` (sets current to 0)
4. Cumulative only updates on hour changes

### Issue: Files Not Saving

**Symptoms:** Files don't appear on SD card

**Solutions:**
1. Check SD card is FAT32
2. Verify SD card isn't full
3. Try `LS` command to list files
4. Re-format SD card if corrupted
5. Use `REINIT` to reinitialize SD

### Issue: System Reboots Continuously

**Symptoms:** "FATAL ERROR" after 3 retries

**Solutions:**
1. Check all hardware connections
2. Verify power supply stability
3. Press GPIO 27 for diagnostics
4. Review serial output for specific error
5. Try hardware reset (power cycle)

### Issue: Button Not Responding

**Symptoms:** Button presses not counted

**Solutions:**
1. Check button connections (pull to GND)
2. Verify correct GPIO pin (15 for counter)
3. Test with different debounce: `SET,2,50`
4. Check for voltage (should go LOW when pressed)
5. Inspect button for mechanical failure

---

## Power Loss Recovery

### What Happens?

If power fails **during active production:**

1. **prod_session.txt** saves current state
2. On reboot, system detects this file
3. Serial shows: "PRODUCTION SESSION RECOVERED FROM UNEXPECTED POWER LOSS"
4. OLED shows: "Production Recovered!"
5. Count restored, production resumes

### Testing Recovery

1. Start production (press GPIO 25)
2. Let it count for a minute
3. Disconnect power
4. Reconnect power
5. Should see "Production Recovered!" message
6. All counts preserved

---

## Daily Workflow Example

### Morning Start (08:00 AM)
```
1. Turn on system
   → "SYSTEM READY" displays
   
2. Check time: TYPE: STATUS
   → Verify RTC time is correct
   
3. Verify SD card: TYPE: LS
   → Confirm SD card working
```

### Production Session 1 (08:00 - 12:00)
```
1. Press GPIO 25 (Latch Button)
   → OLED: "Pro. Started"
   
2. Press GPIO 15 for each unit
   → Count increments
   → Saved every 5 seconds
   
3. Work continues...
   → At 09:00 (hour change):
     - cumulative_count.txt updates
     - count.txt continues from where left off
   
4. 12:00 - Press GPIO 25 again
   → OLED: "Pro. Stopped"
   → Production_2025-11-15_08h00m-12h00m.txt created
   → DailyProduction_2025-11-15.txt created/updated
```

### Check Results
```
TYPE: READ,DailyProduction_2025-11-15.txt

Output:
---
Session: 2025-11-15 08h00m to 2025-11-15 12h00m
Count: 450
```

### Production Session 2 (13:00 - 17:00)
```
1. Lunch break ends
2. Press GPIO 25
   → OLED: "Pro. Started"
3. Count continues...
4. 17:00 - Press GPIO 25
   → Production_2025-11-15_13h00m-17h00m.txt created
```

### End of Day Review
```
TYPE: READ,cumulative_count.txt
→ Total for all hours today

TYPE: READ,DailyProduction_2025-11-15.txt
→ All sessions for the day

TYPE: LS
→ Verify all files saved correctly
```

---

## Advanced Tips

### Optimizing Performance

1. **Faster Response:** `SET,2,30` (lower debounce)
2. **More Frequent Saves:** `SET,1,2000` (save every 2 sec)
3. **Longer Display:** `SET,4,5000` (show messages 5 sec)

### Data Backup

1. Monthly: Connect SD card to PC
2. Copy DailyProduction files to backup
3. Archive old Production files
4. Keep cumulative_count.txt as reference

### Monitoring Production

1. Check current count: `READ,count.txt`
2. Check hourly: `READ,hourly_count.txt`
3. Check cumulative: `READ,cumulative_count.txt`
4. Review sessions: `READ,DailyProduction_*.txt`

### Troubleshooting Production Issues

1. **Missed counts?** Lower debounce: `SET,2,30`
2. **Too many false counts?** Raise debounce: `SET,2,100`
3. **Data not saving?** Check SD: `REINIT`
4. **Time drifts?** Reset RTC: `TIME,YYYY,MM,DD,HH,MM,SS`

---

## Technical Specifications

### System Limits
- **Maximum count per session:** 9999 units
- **Maximum debounce delay:** 500 ms
- **Save interval:** 1000-60000 ms
- **Production sessions:** Unlimited (SD card dependent)

### Storage Requirements
- **SD card:** Minimum 512MB (any size works)
- **File format:** FAT32
- **Typical daily usage:** <50 KB

### Time Accuracy
- **RTC accuracy:** ±5 minutes/month (DS3231)
- **Counting accuracy:** 100% (interrupt-based)
- **Data logging:** Real-time with timestamps

### Power Requirements
- **ESP32:** 5V / 240mA typical
- **OLED:** 3.3V / 20mA
- **RTC:** 3.3V / 1mA
- **SD Card:** 3.3V / 50mA (peaks to 200mA)

---

## Support & Troubleshooting Contact

If you encounter issues not covered here:

1. Check the troubleshooting section above
2. Review serial output for error messages
3. Run diagnostics: Press GPIO 27 button
4. Record the error message and timestamp
5. Check file integrity: `LS` command

---

## End of Documentation

This system is designed for reliable, automatic production counting with minimal user intervention. Follow the daily workflow for best results and maximum data accuracy.

**Happy counting!**
