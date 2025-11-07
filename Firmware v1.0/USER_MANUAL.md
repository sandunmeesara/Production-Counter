# ESP32 Hourly Counter System - User Manual

## Table of Contents
1. [System Overview](#system-overview)
2. [Hardware Setup](#hardware-setup)
3. [Initial Configuration](#initial-configuration)
4. [Basic Operation](#basic-operation)
5. [Serial Commands Reference](#serial-commands-reference)
6. [Parameter Settings](#parameter-settings)
7. [Data Storage](#data-storage)
8. [Troubleshooting](#troubleshooting)
9. [Specifications](#specifications)
10. [Safety Information](#safety-information)

---

## System Overview

The ESP32 Hourly Counter System is an advanced counting device designed for automated data logging with hourly tracking, cumulative totaling, and real-time OLED display feedback.

### Key Features
- **Real-time counting** via interrupt-driven button input
- **128x64 OLED display** showing live counter status
- **Hourly logging** with automatic hour change detection
- **Cumulative tracking** across all hours
- **SD Card storage** for persistent data logging
- **RTC (Real-Time Clock)** for accurate time tracking
- **EEPROM persistence** for settings storage
- **Adjustable parameters** via serial commands
- **Diagnostic mode** for system testing
- **12-hour AM/PM display** format

---

## Hardware Setup

### Components Required
1. **ESP32 Development Board**
2. **Adafruit SSD1306 OLED Display** (128x64, I2C interface)
3. **DS3231 RTC Module** (Real-Time Clock)
4. **SD Card Module** (SPI interface)
5. **Push Button** (Counter trigger)
6. **Push Button** (Diagnostic trigger)
7. **Micro SD Card** (Optional, for data logging)
8. **USB Cable** (For power and serial communication)
9. **Breadboard & Jumper Wires**

### Pin Configuration

#### OLED Display (I2C)
| ESP32 Pin | OLED Pin |
|-----------|----------|
| GPIO 21   | SDA      |
| GPIO 22   | SCL      |
| 3.3V      | VCC      |
| GND       | GND      |

#### RTC Module (I2C - Same bus as OLED)
| ESP32 Pin | RTC Pin |
|-----------|---------|
| GPIO 21   | SDA     |
| GPIO 22   | SCL     |
| 3.3V      | VCC     |
| GND       | GND     |

#### SD Card Module (SPI - VSPI/SPI3)
| ESP32 Pin | SD Pin |
|-----------|--------|
| GPIO 18   | CLK    |
| GPIO 19   | MISO   |
| GPIO 23   | MOSI   |
| GPIO 26   | CS     |
| 3.3V      | VCC    |
| GND       | GND    |

#### Push Buttons
| Button Type | ESP32 Pin | Function |
|------------|-----------|----------|
| Counter Button | GPIO 15 | Increment counter on press |
| Diagnostic Button | GPIO 27 | Enter diagnostic mode |
| Button GND | GND | Both buttons connect to GND |

### Wiring Notes
- ⚠️ Use pull-up resistor (10kΩ) for counter button
- ⚠️ Use pull-up resistor (10kΩ) for diagnostic button
- ✓ I2C bus supports both OLED and RTC on same pins
- ✓ SD card should have its own SPI bus (VSPI)

---

## Initial Configuration

### First-Time Setup Steps

#### Step 1: Upload Code
1. Open Arduino IDE
2. Install required libraries:
   - Adafruit GFX Library
   - Adafruit SSD1306
   - RTClib
   - SD Library
3. Connect ESP32 via USB cable
4. Select Board: "ESP32 Dev Module"
5. Select COM port
6. Upload the Code.cpp sketch

#### Step 2: Connect Serial Monitor
1. Open Serial Monitor (Ctrl+Shift+M)
2. Set baud rate to **115200**
3. Watch the startup sequence

#### Step 3: Initial Startup
You should see:
```
╔════════════════════════════════════════╗
║   ESP32 COUNTER - OLED 128x64 VERSION ║
╚════════════════════════════════════════╝

--- Loading Settings from EEPROM ---
✓ Settings loaded from EEPROM
  Save Interval: 5000 ms
  Debounce Delay: 50 ms
  Max Count: 9999
  Status Display Time: 3000 ms

✓ I2C initialized
--- OLED Display Initialization ---
✓ OLED Display initialized
✓ CS pin configured
--- VSPI (SPI3) for SD Card ---
  SCK:  GPIO 18
  MOSI: GPIO 23
  MISO: GPIO 19
  CS:   GPIO 26
✓ VSPI bus initialized

--- SD Card Initialization ---
Attempting SD initialization on VSPI...
=== SD Card Initialization ===
✓ SD Card initialized at 400 kHz
✓ SD Card ready

--- RTC Initialization ---
✓ RTC responding
✓ RTC time: 2025-10-31 14:30:45

--- File System Check ---
✓ Loaded counts: current=0, hourly=0, cumulative=0

✓ Interrupts configured

╔════════════════════════════════════════╗
║          ✓✓✓ SYSTEM READY ✓✓✓          ║
║    OLED Display Configuration:         ║
║    • 128x64 pixels                     ║
║    • I2C interface (0x3C)              ║
╚════════════════════════════════════════╝

Press GPIO 27 for diagnostics
Type 'INFO' in Serial for DEBUG MENU
```

#### Step 4: Set Current Time (if needed)
```
TIME,2025,10,31,14,30,45
```
Format: `TIME,YYYY,MM,DD,HH,MM,SS`

---

## Basic Operation

### Normal Operation Mode

#### Counting
1. Press the **Counter Button** (GPIO 15) to increment the counter
2. Current count displays on OLED screen in real-time
3. Count is automatically saved to SD card every 5 seconds

#### Display Information
The OLED shows:
```
╔════════════════════════════╗
║   COUNTER              (Title)
║ Time: 02:30:45 PM     (12-hr format)
╠════════════════════════════╣
║ Prev Hour:    ↑            │
║        312    │ Current:   │
║              ↓        245   │
╠════════════════════════════╣
║ Ready        (Status bar)  │
╚════════════════════════════╝
```

#### Hourly Cycle
- When hour changes (e.g., 14:59 → 15:00):
  1. Current count is added to cumulative total
  2. Current count resets to 0
  3. Hourly log file is created
  4. "Hour Logged" message appears on display
  5. All data is saved to SD card

#### Example Hourly Progression
```
Hour 1:  Count 45 items → Cumulative = 45
Hour 2:  Count 38 items → Cumulative = 83
Hour 3:  Count 52 items → Cumulative = 135
```

### Diagnostic Mode

#### Entering Diagnostic Mode
- Press **Diagnostic Button** (GPIO 27) for 2+ seconds
- OR type `DIAG` in serial monitor

#### Diagnostic Tests Performed
✓ OLED Display test
✓ SD Card availability check
✓ RTC module validation

#### Exiting Diagnostic Mode
- Press **Diagnostic Button** (GPIO 27) again
- System returns to normal operation

---

## Serial Commands Reference

### Getting Help
```
INFO
```
Displays the debug menu with all available commands and current settings.

### Counter Reset
```
RESET
```
Resets the **current hour's count to 0** (does NOT reset cumulative total).

### Parameter Configuration

#### Change Save Interval
```
SET,1,3000
```
Sets how often counts are saved to SD card (3000 ms = 3 seconds).
- Range: 1000-60000 ms
- Default: 5000 ms

#### Change Debounce Delay
```
SET,2,100
```
Sets button debounce time to prevent multiple counts from single press.
- Range: 10-500 ms
- Default: 50 ms
- Quick command: `DEBOUNCE,100`

#### Change Max Count
```
SET,3,99999
```
Sets maximum count value before capping.
- Range: 100-99999
- Default: 9999

#### Change Status Display Time
```
SET,4,2000
```
Sets how long status messages appear on OLED.
- Range: 1000-30000 ms
- Default: 3000 ms

### Settings Management

#### Save Settings to EEPROM
```
SAVE
```
Makes current parameter changes **permanent**. Settings will persist after power cycle.

⚠️ **IMPORTANT**: Changes are temporary until you use `SAVE` command!

#### Reset Parameters to Defaults
```
RESETPARAM
```
Resets ALL parameters to factory defaults and saves them.

Default values:
- Save Interval: 5000 ms
- Debounce Delay: 50 ms
- Max Count: 9999
- Status Display Time: 3000 ms

### Time Management

#### Set Current Date & Time
```
TIME,2025,10,31,14,30,45
```
Format: `TIME,YYYY,MM,DD,HH,MM,SS`
- Year: 2020-2100
- Month: 01-12
- Day: 01-31
- Hour: 00-23 (24-hour format)
- Minute: 00-59
- Second: 00-59

---

## Parameter Settings

### Understanding Each Parameter

#### 1. Save Interval
**What it does**: How frequently counts are written to SD card

| Value | Best For |
|-------|----------|
| 1000 ms | High-reliability systems |
| 3000 ms | Balanced (Recommended) |
| 5000 ms | Default, lower SD wear |
| 10000 ms | Very low-frequency events |

#### 2. Debounce Delay
**What it does**: Time between button presses to avoid double-counting

| Value | Best For |
|-------|----------|
| 10 ms | High-speed mechanical buttons |
| 25 ms | Standard electronic buttons |
| 50 ms | Default, general purpose |
| 100 ms | Noisy/worn buttons |
| 200+ ms | Very unreliable buttons |

#### 3. Max Count
**What it does**: Maximum value counter can reach before capping

| Value | Best For |
|-------|----------|
| 9999 | Default, fits on OLED |
| 99999 | Large item counts |
| 999999 | Very high-volume counting |

#### 4. Status Display Time
**What it does**: How long temporary status messages show on OLED

| Value | Best For |
|-------|----------|
| 1000 ms | Quick feedback |
| 2000 ms | Readable, not too long |
| 3000 ms | Default, comfortable reading |
| 5000+ ms | Important notifications |

### Configuration Example

**Scenario**: High-speed production line counter
```
SET,2,25       (Fast debounce for mechanical buttons)
SAVE
SET,3,99999    (Support high volume)
SAVE
SET,1,2000     (Save more frequently)
SAVE
```

---

## Data Storage

### SD Card File Structure
```
/
├── count.txt              (Current hour count)
├── hourly_count.txt       (Last hour's count)
├── cumulative_count.txt   (Total of all hours)
├── 2025_10_31_02_30PM.txt (Hour 1 log)
├── 2025_10_31_03_30PM.txt (Hour 2 log)
└── 2025_10_31_04_30PM.txt (Hour 3 log)
```

### Count Files (count.txt, hourly_count.txt, cumulative_count.txt)
Contains single integer value:
```
245
```

### Hourly Log Files
Format: `YYYY_MM_DD_HH_MMAM/PM.txt`

Contents:
```
Time: 2025-10-31 14:30
Hour Count: 245
Cumulative: 1350
```

### EEPROM Storage
Internal ESP32 flash memory stores:
- Save Interval
- Debounce Delay
- Max Count
- Status Display Time
- Validation magic number

---

## Troubleshooting

### Common Issues & Solutions

#### Issue: "SSD1306 allocation failed!"
**Symptom**: OLED not initializing
**Solutions**:
1. Check I2C connections (SDA/SCL)
2. Verify OLED address: `0x3C`
3. Use I2C scanner sketch to find actual address
4. Check 3.3V power supply to OLED

#### Issue: "SD Card initialization FAILED"
**Symptom**: SD card won't initialize
**Solutions**:
1. Verify SD card wiring (especially CS pin)
2. Try formatting SD card to FAT32
3. Check SD card isn't corrupted
4. Verify 3.3V power to SD module
5. Check SPI clock connections

#### Issue: "RTC not responding"
**Symptom**: Time not updating
**Solutions**:
1. Check I2C connections (SDA/SCL)
2. Verify RTC battery if installed
3. Use I2C scanner to verify RTC address: `0x68`
4. Check RTC module isn't defective

#### Issue: Button not working
**Symptom**: Counter doesn't increment
**Solutions**:
1. Verify button wiring to GPIO 15
2. Check button connected to GND
3. Test pull-up resistor (10kΩ)
4. Increase debounce delay: `DEBOUNCE,100`
5. Check GPIO 15 isn't used for boot

#### Issue: Settings not saving
**Symptom**: Parameters reset after power cycle
**Solutions**:
1. Use `SAVE` command after changes
2. Check serial output: "✓ Settings saved to EEPROM"
3. Try `RESETPARAM` to restore defaults

#### Issue: High count numbers not displaying
**Symptom**: Display shows wrong count
**Solutions**:
1. Adjust Max Count: `SET,3,99999`
2. SAVE settings: `SAVE`
3. Reset counter: `RESET`

#### Issue: Serial monitor not showing messages
**Symptom**: No serial output
**Solutions**:
1. Check baud rate: **115200**
2. Verify USB cable connection
3. Check COM port selection
4. Try different USB port
5. Update CH340 driver (if using clone board)

---

## Specifications

### System Specifications

| Parameter | Value |
|-----------|-------|
| **Microcontroller** | ESP32 |
| **Display** | OLED 128x64 (I2C) |
| **Real-Time Clock** | DS3231 |
| **Data Storage** | SD Card (via SPI) |
| **Settings Storage** | EEPROM (512 bytes) |
| **Communication** | USB Serial (115200 baud) |
| **Operating Voltage** | 5V USB / 3.3V Logic |

### Counter Specifications

| Parameter | Value |
|-----------|-------|
| **Default Max Count** | 9999 |
| **Configurable Max** | 100-99999 |
| **Debounce Range** | 10-500 ms |
| **Default Debounce** | 50 ms |
| **Save Interval Range** | 1000-60000 ms |
| **Default Save Interval** | 5000 ms |

### Performance Specifications

| Parameter | Value |
|-----------|-------|
| **Count Accuracy** | Hardware interrupt (100% reliable) |
| **RTC Accuracy** | ±2 ppm (±59 seconds/year) |
| **Display Refresh** | Real-time (~100ms) |
| **Data Persistence** | Indefinite (SD card & EEPROM) |

### Time Specifications

| Parameter | Value |
|-----------|-------|
| **Time Format** | 12-hour AM/PM on display |
| **Internal Format** | 24-hour |
| **Date Range** | 2020-2100 |
| **RTC Battery Backup** | 10+ years (DS3231) |

---

## Safety Information

### Electrical Safety
- ⚠️ Do NOT exceed 5V on ESP32 pins
- ⚠️ Do NOT apply power backwards
- ✓ Use regulated power supply
- ✓ Verify all connections before power-on
- ✓ Use properly shielded cables if in noisy environment

### Button Safety
- ⚠️ Do NOT hold counter button while powered off
- ✓ Button connects directly to GND through pull-up
- ✓ No high voltage or current on button pins

### SD Card Handling
- ⚠️ Do NOT remove SD card while system is operating
- ⚠️ Do NOT expose SD card to static discharge
- ✓ Power off before inserting/removing SD card
- ✓ Store SD card in anti-static bag when not in use

### Environmental
- Operating Temperature: 0°C to 50°C
- Storage Temperature: -20°C to 85°C
- Humidity: 10% to 90% (non-condensing)
- Altitude: 0 to 3000 meters

---

## Getting Help

### Diagnostic Commands
```
INFO           - Show debug menu
SAVE           - Save settings
RESETPARAM     - Reset to factory defaults
```

### Debug Information
Check Serial Monitor startup messages for:
- ✓ System version and features
- ✓ Loaded settings from EEPROM
- ✓ Hardware initialization status
- ✓ File system status
- ✓ Current counts loaded from SD card

### Support
If issues persist:
1. Take note of error messages
2. Run diagnostic mode (press GPIO 27)
3. Screenshot serial monitor output
4. Verify all connections match wiring diagram
5. Try RESETPARAM to restore defaults

---

## Appendix: Quick Reference

### Command Cheat Sheet
```
INFO                    View settings & commands
SET,1,5000             Set save interval
SET,2,50               Set debounce delay
SET,3,9999             Set max count
SET,4,3000             Set status display time
DEBOUNCE,50            Quick set debounce
SAVE                   Save to EEPROM
RESET                  Reset count to 0
RESETPARAM             Reset all parameters
TIME,2025,10,31,14,30,45  Set date/time
```

### Default Settings
```
Save Interval:        5000 ms
Debounce Delay:       50 ms
Max Count:            9999
Status Display Time:  3000 ms
```

### Required Baud Rate
```
Serial Monitor: 115200
```

### I2C Addresses
```
OLED Display: 0x3C
RTC Module:   0x68
```

### SPI Pins (VSPI)
```
Clock (CLK):   GPIO 18
MISO:          GPIO 19
MOSI:          GPIO 23
Chip Select:   GPIO 26
```

---

**Manual Version**: 1.0  
**Last Updated**: October 31, 2025  
**System Version**: ESP32 Hourly Counter v1.0

**For questions or feedback, refer to the Serial Monitor debug information.**
