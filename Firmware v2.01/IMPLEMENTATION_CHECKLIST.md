# Implementation Checklist - Production Counter System

## Pre-Implementation Phase

### Hardware Preparation
- [ ] ESP32 Dev Board (ESP32-DEVKIT-V1 recommended)
- [ ] 16x4 LCD Panel with PCF8574 I2C Backpack
- [ ] DS3231 Real-Time Clock Module
- [ ] SD Card Module (with level shifter recommended)
- [ ] 3x Push Buttons
- [ ] Breadboard or PCB
- [ ] Jumper Wires (various lengths)
- [ ] Power Supply (5V for ESP32 + SD module)
- [ ] USB Cable for programming

### Component Verification
- [ ] Test ESP32 with LED blink sketch
- [ ] Verify LCD I2C address with scanner sketch
- [ ] Test RTC module responds
- [ ] Format SD card to FAT32
- [ ] Verify push buttons are functioning

---

## Software Installation Phase

### Libraries Installation
- [ ] Install ESP32 Board Support (2.0.11 or later)
  - URL: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
- [ ] Install LiquidCrystal_I2C (1.1.2 or later)
- [ ] Install RTClib (2.1.1 or later)
- [ ] Verify SD.h is available (built-in)
- [ ] Verify EEPROM.h is available (built-in)

### Arduino IDE Configuration
- [ ] Select Board: ESP32 Dev Module
- [ ] Select Upload Speed: 115200
- [ ] Select CPU Frequency: 240 MHz
- [ ] Select Flash Size: 4MB (32Mb)
- [ ] Select Partition Scheme: Default 4MB with spiffs

### Code Preparation
- [ ] Download Code_v2.cpp to your Projects folder
- [ ] Open in Arduino IDE
- [ ] Verify all includes are recognized
- [ ] Check for any red squiggly compilation errors
- [ ] Review pin definitions match your hardware

---

## Hardware Assembly Phase

### Wiring - I2C Devices (GPIO 21=SDA, GPIO 22=SCL)

#### LCD 16x4 with PCF8574 Backpack
```
Backpack Pin | ESP32 GPIO | Signal
─────────────────────────────────
GND          | GND        | Ground
5V           | 5V         | Power
SDA          | 21         | I2C Data
SCL          | 22         | I2C Clock
```

#### DS3231 RTC Module
```
RTC Pin      | ESP32 GPIO | Signal
─────────────────────────────────
GND          | GND        | Ground
5V (or 3.3V) | 3.3V       | Power
SDA          | 21         | I2C Data
SCL          | 22         | I2C Clock
```

### Wiring - SPI Devices (VSPI - SPI3)

#### SD Card Module
```
SD Module Pin | ESP32 GPIO | Signal
──────────────────────────────────
GND           | GND        | Ground
3.3V (thru LV)| 3.3V       | Power
CS            | 26         | Chip Select
CLK           | 18         | SPI Clock
MOSI          | 23         | SPI Data Out
MISO          | 19         | SPI Data In
```

### Wiring - Push Buttons

#### Counter Button
```
GPIO 15 ──[Button]── GND
         ├─ 10kΩ Pull-up to 3.3V (optional, internal pullup used)
         └─ Contact closure to ground
```

#### Production Latch Button
```
GPIO 25 ──[Button]── GND
         ├─ 10kΩ Pull-up to 3.3V (optional, internal pullup used)
         └─ Contact closure to ground
```

#### Diagnostic Button
```
GPIO 27 ──[Button]── GND
         ├─ 10kΩ Pull-up to 3.3V (optional, internal pullup used)
         └─ Contact closure to ground
```

### Assembly Checklist
- [ ] All GND connections made
- [ ] I2C devices on same SDA/SCL bus
- [ ] SPI devices on correct VSPI pins
- [ ] Power connections correct (5V and 3.3V separate)
- [ ] Push buttons properly connected to ground
- [ ] SD card inserted properly
- [ ] LCD backpack contrast potentiometer adjusted
- [ ] All connections double-checked for shorts

---

## Initial Programming Phase

### First Upload
- [ ] Connect ESP32 via USB
- [ ] Verify COM port appears in Tools → Port
- [ ] Open Code_v2.cpp
- [ ] Click Upload (Sketch → Upload or Ctrl+U)
- [ ] Wait for upload message: "Leaving... Hard resetting via RTS pin..."
- [ ] Check Serial Monitor (115200 baud)
- [ ] Look for startup message in Serial output

### Serial Output Expected
```
╔════════════════════════════════════════╗
║   ESP32 COUNTER - OLED 128x64 VERSION ║
╚════════════════════════════════════════╝

--- Loading Settings from EEPROM ---
⚠ No saved settings found in EEPROM (using defaults)

--- LCD Display Initialization ---
✓ LCD Display initialized (16x4)

[... more initialization messages ...]

╔════════════════════════════════════════╗
║          ✓✓✓ SYSTEM READY ✓✓✓          ║
║    LCD Display Configuration:          ║
║    • 16x4 characters                   ║
║    • I2C interface (0x27/0x3F)         ║
║    • Production latch on GPIO 25       ║
╚════════════════════════════════════════╝

Press GPIO 27 for diagnostics
Type 'INFO' in Serial for DEBUG MENU
```

---

## Component Testing Phase

### LCD Display Test
- [ ] LCD shows startup message for 2 seconds
- [ ] LCD shows all 4 rows clearly
- [ ] All 16 columns are visible
- [ ] Backlight is on (can adjust brightness)
- [ ] Characters are readable

**If LCD not showing:**
- [ ] Check I2C connections (21, 22, GND, 5V)
- [ ] Try different I2C address: change LCD_ADDRESS from 0x27 to 0x3F
- [ ] Adjust LCD contrast using potentiometer on backpack
- [ ] Check serial output for I2C initialization messages

### Counter Button Test
- [ ] Press GPIO 15 button
- [ ] Serial output shows count incrementing
- [ ] LCD shows count increasing
- [ ] Each press increments by 1
- [ ] Debounce prevents false triggers

**Testing command:**
```
Send "INFO" via Serial Monitor to see current count
```

### Production Latch Button Test
- [ ] Press GPIO 25 button once
- [ ] LCD display changes to [PROD] mode
- [ ] Serial output shows "PROD STARTED"
- [ ] Check SD card for new log file `/PROD_*.txt`
- [ ] Press GPIO 25 button again
- [ ] LCD display changes to [IDLE] mode
- [ ] Serial output shows "PROD STOPPED"
- [ ] Log file updated with end time and count

### Diagnostic Button Test
- [ ] Press GPIO 27 button for 200ms+
- [ ] LCD shows "DIAGNOSTIC"
- [ ] System tests LCD, SD, RTC
- [ ] Results shown as PASS/FAIL
- [ ] Press button again to exit diagnostic
- [ ] Returns to normal operation

### RTC Module Test
- [ ] Check Serial output for RTC time
- [ ] LCD shows current time
- [ ] Time increments every second
- [ ] Can set time via serial: `TIME,2025,11,06,14,32,15`

**If RTC not working:**
- [ ] Check I2C connections (21, 22, GND, 3.3V or 5V)
- [ ] Check RTC battery (if equipped)
- [ ] Try setting time via serial command

### SD Card Test
- [ ] Serial output shows "✓ SD Card ready"
- [ ] Log files are created during production
- [ ] Can read log files from SD card
- [ ] No corruption of existing files

**If SD not working:**
- [ ] Check connections (18, 19, 23, 26, GND, 3.3V)
- [ ] Format SD card to FAT32
- [ ] Check serial output for error messages
- [ ] Verify CS pin (GPIO 26) is high when idle

---

## Functional Testing Phase

### Production Session Test
**Procedure:**
1. Start system, wait for LCD to display normally
2. Press Production button (GPIO 25)
   - [ ] LCD shows [PROD]
   - [ ] Serial shows "PROD STARTED"
   - [ ] Log file created on SD
3. Press Counter button (GPIO 15) 10 times
   - [ ] Count increases: 1, 2, 3... 10
   - [ ] Session count matches
4. Wait 5 seconds (auto-save interval)
   - [ ] Count saved to SD
5. Press Production button (GPIO 25)
   - [ ] LCD shows [IDLE]
   - [ ] Serial shows "PROD STOPPED"
   - [ ] Log file updated
6. Check SD card log file contents
   - [ ] Should show: Start time, End time, Count: 10

### Multiple Sessions Test
- [ ] Repeat production session 3-4 times
- [ ] Verify each creates unique log file
- [ ] Check all files have correct timestamps
- [ ] Verify counts are accurate

### Hourly Log Test
- [ ] Wait for hour boundary (or manually advance RTC)
- [ ] Verify hour log file created
- [ ] Check hourly count in new file
- [ ] Verify count resets for new hour

### Power Cycle Test
- [ ] Let system run for 2+ minutes
- [ ] Record current count
- [ ] Power off ESP32
- [ ] Power on ESP32
- [ ] Check that count is restored from SD card
- [ ] Verify time is correct (RTC with battery)

---

## Configuration Phase

### Serial Menu Configuration
Send "INFO" to see debug menu:

**Setting Parameters:**
```
SET,1,5000        - Change save interval to 5000ms
SET,2,50          - Change debounce to 50ms
SET,3,9999        - Change max count to 9999
SET,4,3000        - Change status display to 3000ms
SAVE              - Save all settings to EEPROM
```

### Adjust if Needed
- [ ] Debounce too sensitive? → Increase SET,2 value
- [ ] Debounce too slow? → Decrease SET,2 value
- [ ] Save too frequent? → Increase SET,1 value
- [ ] Display not refreshing? → Check DISPLAY_UPDATE_INTERVAL

---

## Documentation Phase

### Create System Documentation
- [ ] Screenshot LCD display in both modes
- [ ] Document wiring with photos
- [ ] Create quick reference card
- [ ] Store configuration settings (backup EEPROM)
- [ ] Log test results and settings used

### Setup Documentation
- [ ] Note I2C address used
- [ ] Note all GPIO assignments
- [ ] Document debounce delay setting
- [ ] Note save interval setting
- [ ] Record RTC time when set

---

## Final Verification Phase

### System Running Checklist
- [ ] ESP32 powers on without errors
- [ ] LCD displays all information clearly
- [ ] Counter button increments count
- [ ] Production button toggles modes
- [ ] Diagnostic shows all devices PASS
- [ ] Time displayed correctly
- [ ] Log files created correctly
- [ ] No serial errors or warnings
- [ ] Settings persist after power cycle
- [ ] Counts restore from SD card

### Performance Verification
- [ ] LCD updates smoothly (500ms interval)
- [ ] Counter response is immediate
- [ ] Production toggle is responsive
- [ ] No I2C communication errors
- [ ] SD card writes successfully
- [ ] Memory usage is stable

---

## Deployment Phase

### Pre-Deployment Checklist
- [ ] System tested for 24+ hours continuously
- [ ] All features working as expected
- [ ] Settings saved and verified
- [ ] Backup of EEPROM settings taken
- [ ] Documentation complete
- [ ] Operating procedures documented

### Deployment Steps
- [ ] Install in production location
- [ ] Connect all sensors and buttons
- [ ] Verify operation in location
- [ ] Set production start time via serial
- [ ] Create startup procedure documentation
- [ ] Train operators on button usage
- [ ] Set monitoring/backup procedures

---

## Maintenance Schedule

### Daily
- [ ] Verify LCD display is clear
- [ ] Check button responsiveness
- [ ] Review log files for anomalies

### Weekly
- [ ] Backup log files from SD card
- [ ] Check RTC battery status
- [ ] Verify SD card integrity

### Monthly
- [ ] Check for SD card corruption
- [ ] Review statistics and trends
- [ ] Clean connections if needed

### Quarterly
- [ ] Check EEPROM for corruption
- [ ] Verify all sensors still working
- [ ] Update firmware if available

---

## Troubleshooting Reference

### LCD Issues
Problem: "LCD not displaying"
- [ ] Check power (5V to backpack)
- [ ] Check I2C connections
- [ ] Try different I2C address
- [ ] Adjust contrast potentiometer
- [ ] Check serial output for errors

### Button Issues
Problem: "Buttons not responding"
- [ ] Check connections to GND
- [ ] Verify GPIO pin numbers
- [ ] Test with diagnostic mode
- [ ] Check for debounce issues

### Production Log Issues
Problem: "Log files not created"
- [ ] Check SD card power (3.3V)
- [ ] Verify SD card is FAT32
- [ ] Check SPI connections
- [ ] Review serial output for SD errors

### Time Issues
Problem: "RTC shows wrong time"
- [ ] Set time via serial command
- [ ] Check RTC battery
- [ ] Verify I2C connections
- [ ] Try power cycle

---

**Estimated Total Setup Time: 2-4 hours**

- Hardware Assembly: 45-60 minutes
- Software Installation: 30-45 minutes
- Initial Testing: 60-90 minutes
- Fine-tuning and Documentation: 30-45 minutes

---

**Last Updated:** November 6, 2025
**System Version:** ESP32 Production Counter v2.0 (LCD Edition)
