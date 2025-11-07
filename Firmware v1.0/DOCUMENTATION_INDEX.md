# ESP32 Counter System - Documentation Index

## 📚 Complete Documentation Suite

Welcome to the ESP32 Hourly Counter System documentation. This package contains comprehensive guides to help you set up, operate, and troubleshoot your system.

---

## 📖 Document Guide

### 1. **QUICK_START.md** ⚡ START HERE!
**For**: First-time users, impatient developers  
**Length**: 5 minutes read  
**Contains**:
- 5-minute hardware setup
- Essential commands cheat sheet
- Common troubleshooting
- Expected behavior examples

**When to use**: Your first time running the system

---

### 2. **USER_MANUAL.md** 📘 Complete Reference
**For**: Daily operation, complete understanding  
**Length**: 20-30 minutes read  
**Contains**:
- System overview and features
- Detailed hardware setup guide
- Initial configuration steps
- Complete serial commands reference
- Parameter explanations
- Data storage format
- Specifications and safety

**When to use**: After quick start, for operational guidance

---

### 3. **WIRING_GUIDE.md** 🔌 Hardware Documentation
**For**: Wiring and hardware connections  
**Length**: 15-20 minutes read  
**Contains**:
- Complete wiring diagrams (ASCII art)
- Pin-by-pin connection guide
- Component specifications
- Power distribution details
- Troubleshooting by connection
- Best practices for layout
- Safety reminders

**When to use**: During hardware assembly, verifying connections

---

### 4. **TROUBLESHOOTING_FAQ.md** 🔧 Problem Solver
**For**: When something isn't working  
**Length**: Variable (reference document)  
**Contains**:
- Detailed troubleshooting for each component
- Problem diagnosis flowcharts
- Frequently asked questions with answers
- Emergency procedures
- Component failure indicators
- When to seek help

**When to use**: Something not working as expected

---

### 5. **EEPROM_SETTINGS_GUIDE.md** 💾 Settings Persistence
**For**: Understanding persistent storage  
**Length**: 10 minutes read  
**Contains**:
- Overview of EEPROM storage
- What settings get saved
- How to save and load settings
- Workflow examples
- Memory layout details
- Troubleshooting guide

**When to use**: Configuring system parameters, understanding persistence

---

### 6. **OPTIMIZATION_REPORT.md** ⚙️ Code Quality
**For**: Understanding code improvements  
**Length**: 5-10 minutes read  
**Contains**:
- Critical errors fixed
- Performance optimizations
- Code improvements applied
- Best practices implemented
- Verification checklist

**When to use**: Understanding the codebase, learning improvements

---

## 🎯 Quick Navigation by Task

### "I just got the hardware"
→ Read: **QUICK_START.md** + **WIRING_GUIDE.md**

### "I need to set up the system"
→ Read: **USER_MANUAL.md** (Initial Configuration section)

### "How do I use the system?"
→ Read: **USER_MANUAL.md** (Basic Operation section)

### "How do I change settings?"
→ Read: **USER_MANUAL.md** (Parameter Settings) + **EEPROM_SETTINGS_GUIDE.md**

### "Something isn't working"
→ Read: **TROUBLESHOOTING_FAQ.md**

### "I'm not sure about a connection"
→ Read: **WIRING_GUIDE.md**

### "I want to understand the system better"
→ Read: **USER_MANUAL.md** (entire document)

### "I'm curious about the code"
→ Read: **OPTIMIZATION_REPORT.md**

---

## 📋 Serial Commands Quick Reference

| Command | What It Does | Example |
|---------|-------------|---------|
| `INFO` | Show settings & all commands | `INFO` |
| `SAVE` | Save settings to EEPROM | `SAVE` |
| `SET,1,<ms>` | Change save interval | `SET,1,3000` |
| `SET,2,<ms>` | Change debounce | `SET,2,75` |
| `SET,3,<num>` | Change max count | `SET,3,99999` |
| `SET,4,<ms>` | Change status time | `SET,4,2000` |
| `DEBOUNCE,<ms>` | Quick debounce set | `DEBOUNCE,100` |
| `RESET` | Reset current count | `RESET` |
| `RESETPARAM` | Reset all parameters | `RESETPARAM` |
| `TIME,Y,M,D,H,M,S` | Set date & time | `TIME,2025,10,31,14,30,45` |

---

## 🔑 Key Concepts to Understand

### Current Count
The count during the current hour. Resets to 0 when hour changes.
- **Stored in**: `/count.txt` on SD card
- **Display**: Right side of OLED
- **Reset with**: `RESET` command

### Hourly Count
The count from the most recent completed hour.
- **Stored in**: `/hourly_count.txt` on SD card
- **Display**: Left side of OLED (labeled "Prev Hour")
- **Updated**: Automatically when hour changes

### Cumulative Count
Total of all counts across all hours. Never resets (unless you delete file).
- **Stored in**: `/cumulative_count.txt` on SD card
- **Display**: Not shown on OLED (use `INFO` to see)
- **Updated**: When hour changes
- **Example**: Hour 1 = 45, Hour 2 = 38 → Cumulative = 83

### Persistent Settings
User-configurable parameters stored in EEPROM flash memory.
- **Stored in**: ESP32 internal EEPROM (512 bytes)
- **Saved with**: `SAVE` command
- **Persist through**: Power cycles, disconnections, reboots
- **Reset with**: `RESETPARAM` command

---

## 🛠️ System Architecture Overview

```
┌─────────────────────────────────────────────┐
│            ESP32 Microcontroller            │
├─────────────────────────────────────────────┤
│                                             │
│  I2C Bus (GPIO 21, 22)                     │
│  ├─ OLED Display (0x3C)                    │
│  └─ RTC Module DS3231 (0x68)               │
│                                             │
│  SPI Bus (GPIO 18, 19, 23, 26)             │
│  └─ SD Card Module                         │
│                                             │
│  GPIO Interrupts                           │
│  ├─ GPIO 15 (Counter Button)               │
│  └─ GPIO 27 (Diagnostic Button)            │
│                                             │
│  EEPROM Storage (512 bytes)                │
│  └─ Persistent Settings                    │
│                                             │
└─────────────────────────────────────────────┘
```

---

## ⚡ Most Important Rules

1. **⚠️ Use SAVE after changing parameters**
   - Without SAVE: Settings lost on restart
   - With SAVE: Settings persist forever

2. **⚠️ Baud rate MUST be 115200**
   - Wrong rate = garbled text
   - First thing to check if no serial output

3. **⚠️ ALWAYS use 3.3V logic**
   - ESP32 pins = 3.3V max
   - 5V will damage the board
   - Never apply 5V to GPIO or I2C

4. **⚠️ SD card must be FAT32 formatted**
   - Other formats won't work
   - System won't detect or use SD card

5. **⚠️ Pull-up resistors required for buttons**
   - 10kΩ resistor from GPIO to 3.3V
   - Without it: button won't work reliably

---

## 🐛 Debugging Tips

### Serial Monitor is Your Friend
- Always open Serial Monitor (115200 baud) when troubleshooting
- Watch startup messages for errors
- Use `INFO` command to check current state

### Systematic Testing
1. Check one component at a time
2. Verify connections before power
3. Watch for error messages
4. Try basic `INFO` command first
5. Document what you've tried

### Use Multimeter When Possible
- Verify 3.3V supply voltage
- Check GND continuity
- Test button with continuity mode
- Measure voltage on pins

---

## 📞 Support Resources

### Built-in Help
- Type `INFO` in Serial Monitor for command list
- Watch Serial Monitor startup for diagnostics
- Use diagnostic mode: Press GPIO 27 button

### Documentation
- Refer to specific guide for your issue
- Check TROUBLESHOOTING_FAQ.md first
- Read WIRING_GUIDE.md for hardware questions

### Online Resources
- Adafruit libraries documentation
- ESP32 official documentation
- Arduino community forum
- Electronics Stack Exchange

---

## 📝 Document Maintenance

| Document | Last Updated | Version |
|----------|-------------|---------|
| QUICK_START.md | Oct 31, 2025 | 1.0 |
| USER_MANUAL.md | Oct 31, 2025 | 1.0 |
| WIRING_GUIDE.md | Oct 31, 2025 | 1.0 |
| TROUBLESHOOTING_FAQ.md | Oct 31, 2025 | 1.0 |
| EEPROM_SETTINGS_GUIDE.md | Oct 31, 2025 | 1.0 |
| OPTIMIZATION_REPORT.md | Oct 31, 2025 | 1.0 |

---

## 🎓 Learning Path

### Beginner Path (New to all this)
1. Read: QUICK_START.md
2. Assemble: Hardware using WIRING_GUIDE.md
3. Run: Code and watch Serial Monitor
4. Learn: Basic operation from USER_MANUAL.md
5. Experiment: Try changing one parameter at a time

### Intermediate Path (Some experience)
1. Skim: QUICK_START.md for overview
2. Read: USER_MANUAL.md sections 2-5
3. Configure: Parameters using EEPROM_SETTINGS_GUIDE.md
4. Optimize: Your specific use case
5. Troubleshoot: Using TROUBLESHOOTING_FAQ.md as needed

### Advanced Path (Experienced developer)
1. Check: OPTIMIZATION_REPORT.md for code quality
2. Review: Code.cpp directly
3. Modify: Code for specific requirements
4. Reference: USER_MANUAL.md for system behavior
5. Debug: Using Serial Monitor diagnostic output

---

## ✅ Pre-Launch Checklist

Before declaring your system ready:

- [ ] Serial Monitor shows startup messages
- [ ] OLED displays "Starting..." then "COUNTER"
- [ ] RTC shows correct time
- [ ] SD card initialized successfully
- [ ] Counter button increments count
- [ ] Diagnostic button enters diagnostic mode
- [ ] Settings can be changed with SET commands
- [ ] SAVE command confirms data written
- [ ] Settings persist after power cycle
- [ ] Hourly logs created on hour change
- [ ] Cumulative total updates correctly

---

## 🎉 Success Criteria

Your system is working correctly when:

✓ Counts increment reliably on button press  
✓ OLED displays update in real-time  
✓ Hour changes trigger automatic logging  
✓ Settings persist after disconnect  
✓ Serial commands respond as documented  
✓ SD card contains count files  
✓ No error messages in Serial Monitor  
✓ No magic smoke (excellent sign!)  

---

## 📦 Files in This Package

```
Code.cpp                    - Main firmware code
USER_MANUAL.md             - Complete user manual
QUICK_START.md             - Quick start guide
WIRING_GUIDE.md            - Hardware wiring documentation
TROUBLESHOOTING_FAQ.md     - Problem solving guide
EEPROM_SETTINGS_GUIDE.md   - Settings persistence guide
OPTIMIZATION_REPORT.md     - Code improvements report
DOCUMENTATION_INDEX.md     - This file
```

---

## 🚀 Ready to Begin?

### First Time Here?
→ Start with: **QUICK_START.md**

### Already Have Hardware?
→ Jump to: **WIRING_GUIDE.md**

### Want to Learn Everything?
→ Read: **USER_MANUAL.md**

### Something Broken?
→ Check: **TROUBLESHOOTING_FAQ.md**

---

## 📞 Quick Help

**Q: Where do I start?**  
A: QUICK_START.md (5 minutes)

**Q: My button doesn't work?**  
A: TROUBLESHOOTING_FAQ.md → Button Issues

**Q: How do I set parameters?**  
A: USER_MANUAL.md → Serial Commands Reference

**Q: How do I wire it up?**  
A: WIRING_GUIDE.md → Pin Configuration

**Q: How do I save settings permanently?**  
A: EEPROM_SETTINGS_GUIDE.md or type `SAVE`

---

**Welcome to the ESP32 Counter System!**

*Your questions are probably answered in the documentation above. Happy counting! 🎉*

---

**Document Version**: 1.0  
**Created**: October 31, 2025  
**Status**: Complete ✅
