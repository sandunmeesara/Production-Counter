# ESP32 Counter System - Quick Start Guide

## 🚀 5-Minute Setup

### What You Need
- ESP32 board
- OLED display (128x64, I2C)
- DS3231 RTC module
- SD card module
- 2 push buttons
- USB cable

### Step 1: Upload Code (2 min)
1. Install Arduino IDE
2. Add ESP32 board: `Tools → Boards Manager → esp32`
3. Install libraries:
   - Adafruit SSD1306
   - Adafruit GFX
   - RTClib
4. Open `Code.cpp`
5. Select Board: "ESP32 Dev Module"
6. Click Upload

### Step 2: Connect Hardware (2 min)

**I2C Devices** (OLED + RTC):
```
ESP32 GPIO21 (SDA) → OLED SDA & RTC SDA
ESP32 GPIO22 (SCL) → OLED SCL & RTC SCL
ESP32 3.3V → OLED VCC & RTC VCC
ESP32 GND → OLED GND & RTC GND
```

**SD Card Module** (SPI):
```
ESP32 GPIO18 → SD CLK
ESP32 GPIO19 → SD MISO
ESP32 GPIO23 → SD MOSI
ESP32 GPIO26 → SD CS
ESP32 3.3V → SD VCC
ESP32 GND → SD GND
```

**Push Buttons**:
```
GPIO 15 → Counter button → 10kΩ resistor → 3.3V (with button to GND)
GPIO 27 → Diagnostic button → 10kΩ resistor → 3.3V (with button to GND)
```

### Step 3: First Run (1 min)
1. Connect USB cable
2. Open Serial Monitor (Ctrl+Shift+M)
3. Set baud rate to **115200**
4. You should see startup messages
5. Press counter button - number increases!

---

## 📋 Essential Commands

### View Current Status
```
INFO
```

### Adjust Debounce (Button responsiveness)
```
DEBOUNCE,50
SAVE
```

### Set Current Time
```
TIME,2025,10,31,14,30,45
```

### Save Settings (VERY IMPORTANT!)
```
SAVE
```

### Reset Counter
```
RESET
```

---

## 🎯 What You'll See

### Serial Monitor Output
```
✓ I2C initialized
✓ OLED Display initialized
✓ SD Card initialized
✓ RTC responding
✓ Interrupts configured
*** SYSTEM READY ***
```

### OLED Display
```
════════════════════════════════
   COUNTER
 Time: 02:30:45 PM
════════════════════════════════
 Prev Hour:     Current:
      312            245
════════════════════════════════
 Ready
════════════════════════════════
```

### Files Created on SD Card
- `/count.txt` - Current count
- `/hourly_count.txt` - Last hour's count  
- `/cumulative_count.txt` - Total of all hours
- `/2025_10_31_02_30PM.txt` - Hourly logs

---

## ⚙️ Quick Settings

### For Sensitive Buttons
```
DEBOUNCE,100
SAVE
```

### For Fast Counting
```
DEBOUNCE,25
SAVE
```

### For High Volumes
```
SET,3,99999
SAVE
```

### For Frequent Saves
```
SET,1,2000
SAVE
```

---

## ✅ Troubleshooting

| Problem | Solution |
|---------|----------|
| No serial output | Check baud rate = 115200 |
| Button not working | Try `DEBOUNCE,75` then `SAVE` |
| OLED blank | Verify I2C connections on GPIO 21/22 |
| SD card error | Format SD card to FAT32 |
| Settings reset | Use `SAVE` after changes |
| Time not updating | Check RTC connections |

---

## 📊 Example Workflow

```
INFO                              → View current settings
DEBOUNCE,60                       → Adjust button sensitivity
SAVE                              → Make it permanent
(Press button multiple times)      → Test counter
RESET                             → Clear current count
(Wait 1 hour)                      → Hour changes automatically
INFO                              → See cumulative total increased
```

---

## 🔑 Key Points to Remember

1. **Always use SAVE** after changing parameters!
   ```
   SET,2,100
   SAVE    ← Don't forget this!
   ```

2. **Default debounce is 50ms** - adjust if button too sensitive
   ```
   DEBOUNCE,75
   SAVE
   ```

3. **Hour changes automatically** at the hour boundary
   - Old count saved to cumulative
   - New count starts from 0
   - Log file created

4. **Data persists on SD card** - safe to power off
   - Count files always saved
   - Hourly logs preserved
   - EEPROM stores settings

5. **Serial baud rate is 115200**
   ```
   ⚠️ Common mistake: wrong baud rate
   ✓ Solution: Set to 115200
   ```

---

## 🎮 Interactive Commands

Try these in Serial Monitor:

```
TYPE THIS           WHAT IT DOES
─────────────────────────────────────────
INFO                Show all commands
DEBOUNCE,50         Set button delay
SAVE                Save settings
RESET               Clear count
RESETPARAM          Factory defaults
TIME,2025,10,31,14,30,45    Set date/time
```

---

## 📈 Expected Behavior

### Hour 1
- Press button 45 times
- Display shows: Current: 45
- Cumulative: 45

### Hour 2
- Press button 38 times  
- Display shows: Current: 38, Previous: 45
- When hour ends → Cumulative: 83

### Hour 3
- Press button 52 times
- Display shows: Current: 52, Previous: 38
- When hour ends → Cumulative: 135

---

## 🆘 Need Help?

1. **Open Serial Monitor** (Ctrl+Shift+M)
2. **Type**: `INFO`
3. **Read**: Debug menu output
4. **Check**: Your connections against the wiring diagram

### Common Fixes
```
Button not responding:
  → Try: DEBOUNCE,75
  → Then: SAVE
  
Settings lost after restart:
  → Did you use SAVE?
  → Try: SAVE again
  
OLED shows nothing:
  → Check GPIO 21 (SDA)
  → Check GPIO 22 (SCL)
  → Check 3.3V power
```

---

## 📞 Support Checklist

- [ ] Baud rate set to 115200
- [ ] All connections verified
- [ ] SD card formatted (FAT32)
- [ ] OLED showing startup message
- [ ] Button increments counter
- [ ] Settings saved with SAVE command
- [ ] Time set correctly

---

**Version**: 1.0  
**Last Updated**: October 31, 2025

**Good luck! You're ready to count! 🎉**
