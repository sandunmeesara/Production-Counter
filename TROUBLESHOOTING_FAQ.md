# ESP32 Counter System - Troubleshooting & FAQ

## Troubleshooting Guide

### OLED Display Issues

#### Problem: OLED Display Shows Nothing / Blank Screen

**Symptom**: OLED powered on (backlight visible) but no text displayed

**Root Causes & Solutions**:

1. **Incorrect I2C Address**
   ```
   Solution: Run I2C Scanner
   - Open Arduino IDE
   - Go to File → Examples → Wire → i2c_scanner
   - Upload and check Serial Monitor
   - Look for address (usually 0x3C or 0x3D)
   - If different from 0x3C, modify SCREEN_ADDRESS
   ```

2. **I2C Connection Loose**
   ```
   Check:
   - GPIO 21 (SDA) firmly connected to OLED SDA
   - GPIO 22 (SCL) firmly connected to OLED SCL
   - Gently tug on connections - should not wiggle
   - Try different breadboard holes if available
   ```

3. **Wrong Voltage**
   ```
   Verify:
   - OLED VCC connected to 3.3V (NOT 5V!)
   - Measure with multimeter: should read 3.0-3.3V on OLED VCC
   - If higher, check ESP32 power supply
   ```

4. **OLED Module Defective**
   ```
   Test:
   - Use I2C scanner to see if device detected
   - If not found, OLED likely defective
   - Try different OLED module
   ```

#### Problem: Text Cut Off or Wrong Position on Display

**Solutions**:
```
1. Check SCREEN_WIDTH and SCREEN_HEIGHT (should be 128 and 64)
2. Verify text size: setTextSize(1) or setTextSize(2)
3. Check cursor position: setCursor(x, y)
4. Recompile and re-upload code
```

---

### RTC (Real-Time Clock) Issues

#### Problem: "RTC not responding" / Time Not Updating

**Root Causes & Solutions**:

1. **I2C Connection Problem**
   ```
   Check:
   - GPIO 21 (SDA) connected to RTC SDA
   - GPIO 22 (SCL) connected to RTC SCL
   - Same I2C bus as OLED (this is correct!)
   ```

2. **RTC Address Not Found**
   ```
   Solution: Run I2C Scanner
   - Expected address: 0x68
   - If not detected, check connections
   - Check RTC battery (CR2032)
   ```

3. **RTC Battery Dead**
   ```
   Solution: Replace battery
   - Remove CR2032 battery
   - Install new CR2032 battery
   - Polarity: + facing up
   - After replacement, set time again: TIME,2025,10,31,14,30,45
   ```

4. **RTC Module Defective**
   ```
   Test:
   - Try with I2C scanner
   - If not detected at all, likely defective
   - Replace RTC module
   ```

#### Problem: Time Keeps Resetting / Wrong Time After Power Cycle

**Solutions**:
```
1. Set time: TIME,2025,10,31,14,30,45
2. Check RTC battery is installed
3. Try RESETPARAM then SAVE
4. Check for message: "RTC lost power!" in startup
5. If still resetting, RTC battery likely dead (replace)
```

---

### SD Card Issues

#### Problem: "SD Card initialization FAILED on all speeds"

**Root Causes & Solutions**:

1. **SD Card Not Formatted**
   ```
   Solution: Format to FAT32
   - On Windows: Right-click card → Format
   - File System: FAT32
   - Allocation Unit: Default
   - Volume Label: (optional)
   - Click Format → OK
   ```

2. **SD Wiring Incorrect**
   ```
   Verify connections:
   - GPIO 18 → SD CLK ✓
   - GPIO 19 → SD MISO ✓
   - GPIO 23 → SD MOSI ✓
   - GPIO 26 → SD CS ✓
   - 3.3V → SD VCC ✓
   - GND → SD GND ✓
   ```

3. **SD Card Not Inserted Fully**
   ```
   Solution:
   - Remove SD card
   - Clean gold contacts with dry cloth
   - Reinsert fully - should click
   - Verify with multimeter: check power at SD module
   ```

4. **SD Module Defective**
   ```
   Test:
   - Try different SD card
   - Try different SD module
   - Check for corrosion on pins
   ```

5. **Power Supply Insufficient**
   ```
   Check:
   - USB power supply at least 1A
   - Measure 3.3V rail with multimeter
   - Should be stable 3.0-3.3V
   - Not dropping during SD access
   ```

#### Problem: SD Card Works But Files Not Saving

**Solutions**:
```
1. Check serial message: "✓ SD Card ready"
2. Verify save interval with INFO command
3. Try manual save: RESET (which triggers save)
4. Check if SD card full: Remove files to free space
5. Try SAVE command to test EEPROM write
```

---

### Button Issues

#### Problem: Button Not Incrementing Counter

**Root Causes & Solutions**:

1. **Button Not Wired Correctly**
   ```
   Check:
   - Button terminal 1 → GPIO 15 (or GPIO 27 for diagnostic)
   - Button terminal 2 → GND
   - Pull-up resistor: 10kΩ from GPIO to 3.3V
   - No shorts between connections
   ```

2. **Debounce Time Too Short**
   ```
   Solution: Increase debounce
   Command: DEBOUNCE,100
   Then: SAVE
   
   Reason: Mechanical buttons "bounce"
   If debounce too short, bounce might be ignored
   ```

3. **Button Defective**
   ```
   Test:
   - Use multimeter: continuity test
   - When pressed, should show 0Ω
   - When released, should show ∞Ω
   ```

4. **GPIO 15 Configuration Issue**
   ```
   Note: GPIO 15 has special boot function
   Solution:
   - Ensure pull-down disabled during boot
   - Try alternate GPIO if available
   - Verify INPUT_PULLUP mode in code
   ```

#### Problem: Multiple Counts From Single Button Press

**Solution**: Increase debounce delay
```
DEBOUNCE,100    (increase from default 50ms)
SAVE

If still multiple:
DEBOUNCE,150
SAVE
```

#### Problem: Button Response Delayed

**Solutions**:
```
1. Check if debounce set too high: DEBOUNCE,50 (default)
2. Verify main loop delay: should be 10ms
3. Check SD card not busy writing
4. Try moving counter button to different GPIO
```

---

### Serial Communication Issues

#### Problem: No Serial Monitor Output / Garbled Text

**Root Causes & Solutions**:

1. **Wrong Baud Rate**
   ```
   ⚠️ MOST COMMON ISSUE!
   
   Solution: Set to 115200
   - Serial Monitor → Baud Rate dropdown
   - Select 115200 (bottom of list)
   - If was 9600, will see gibberish
   ```

2. **Wrong COM Port**
   ```
   Solution:
   - Arduino IDE → Tools → Port
   - Look for COMx (usually COM3-COM7)
   - Try different ports if unsure
   - Device Manager to see available ports
   ```

3. **USB Cable Defective**
   ```
   Test:
   - Try different USB cable
   - Try different USB port on computer
   - Verify cable is data cable (not charge-only)
   ```

4. **CH340 Driver Missing** (for clone boards)
   ```
   Solution:
   - Download CH340 driver
   - Install on Windows
   - Restart Arduino IDE
   - Reconnect board
   ```

5. **USB to Serial Chip Defective**
   ```
   Test:
   - Check for lights/LEDs on board
   - Try programming via USB-to-TTL adapter
   - If still no response, replace board
   ```

---

### Settings & EEPROM Issues

#### Problem: Settings Lost After Power Cycle

**Root Cause**: Forgot to use SAVE command

**Solution**:
```
After changing parameters, ALWAYS do:
SET,2,100    (example: set debounce)
SAVE         ← This is required!

Without SAVE: settings temporary only
With SAVE: settings stored in EEPROM
```

#### Problem: EEPROM Seems Corrupted

**Solutions**:
```
1. Reset to defaults: RESETPARAM
2. This will also save to EEPROM
3. Manually set each parameter again
4. Use SAVE after each change
```

#### Problem: Settings Changed But Not Taking Effect

**Solutions**:
```
1. Did you use SAVE? (check serial output)
2. Interrupt-sensitive settings take effect on next interrupt
3. For display settings, restart system
4. Try: SAVE followed by INFO to verify
```

---

### Counting Logic Issues

#### Problem: Count Goes Negative or Shows Wrong Value

**Solutions**:
```
1. Reset count: RESET
2. Increase debounce: DEBOUNCE,75 + SAVE
3. Check Max Count setting: SET,3,9999
4. Verify SD card not corrupted: Try RESETPARAM
```

#### Problem: Cumulative Count Doesn't Update

**Root Cause**: Hour change might not have triggered

**Solutions**:
```
1. Wait for actual hour change (14:59:59 → 15:00:00)
2. Check RTC time is correct: TIME,2025,10,31,14,30,45
3. Verify RTC responding: Check startup messages
4. Try manual test: Wait for hour boundary
```

#### Problem: Hourly Logs Not Created

**Solutions**:
```
1. Check SD card available: Watch startup message
2. Check disk space: Files directory
3. Verify RTC working: TIME command responds
4. Check hour actually changed: Watch serial for "Hour Changed"
```

---

### Performance Issues

#### Problem: Display Updates Sluggish / Freezes

**Solutions**:
```
1. Check save interval not too short: SET,1,5000
2. SD card might be busy: Try faster SD card
3. Increase loop delay if needed (currently 10ms)
4. Remove unnecessary Serial.print() calls
```

#### Problem: Button Response Slow

**Solutions**:
```
1. Reduce debounce: DEBOUNCE,25 (if faster needed)
2. Check save interval: Shouldn't block button
3. Verify no USB serial bottleneck
4. Reduce OLED update frequency if needed
```

---

## FAQ (Frequently Asked Questions)

### General Questions

**Q: What's the maximum count value?**
```
A: Default is 9999
   Configurable up to 99999 with: SET,3,99999 + SAVE
```

**Q: How many hours of data can it store?**
```
A: Limited only by SD card size
   Typical 4GB SD card = 50,000+ hourly records
```

**Q: Can I use 5V buttons instead of 3.3V?**
```
A: NO! ESP32 GPIO is 3.3V only
   Will damage the ESP32
   Always use 3.3V logic
```

**Q: What's the accuracy of the counter?**
```
A: 100% accurate (hardware interrupt-based)
   No counts are lost if debounce not too aggressive
```

**Q: Can I count two different things simultaneously?**
```
A: Yes! Use GPIO 15 for one, and another unused GPIO for second
   Requires code modification for separate counters
```

### Technical Questions

**Q: Why do I need pull-up resistors on buttons?**
```
A: Keeps GPIO high when button not pressed
   When button pressed, GPIO pulled to ground
   Creates clean logic signal
   Without it: floating pin = unpredictable behavior
```

**Q: Can I use 5V SD card module?**
```
A: No! Must use 3.3V module
   5V module will damage ESP32 SPI pins
   Always verify module voltage before connecting
```

**Q: What's the I2C clock speed?**
```
A: Default: 100 kHz (standard mode)
   Can go up to 400 kHz (fast mode)
   Current implementation: 100 kHz
```

**Q: Why do settings need EEPROM?**
```
A: EEPROM stores settings in flash memory
   Persists through power cycles
   Without it, settings lost on restart
```

### Operational Questions

**Q: How often should I press the button?**
```
A: No limit! Can count at any speed
   Debounce only matters for single press duration
```

**Q: What happens if I exceed Max Count?**
```
A: Counter stops incrementing at Max Count
   Set higher Max Count if needed: SET,3,99999
```

**Q: Can I set time to past/future date?**
```
A: Yes! TIME command accepts any valid date
   Range: 2020-2100
   Format: TIME,YYYY,MM,DD,HH,MM,SS
```

**Q: What happens when SD card is full?**
```
A: SD library will fail to write
   Check file sizes in SD card
   Delete old logs to free space
   System continues counting but won't save
```

### Maintenance Questions

**Q: How often should I back up data?**
```
A: At least weekly
   Copy .txt files from SD card to computer
   SD cards can fail unexpectedly
```

**Q: How long will SD card last?**
```
A: Typical: 5-10 years
   Write cycles: 100,000+ typical
   Current system: ~365 writes/year (low wear)
```

**Q: Should I format SD card periodically?**
```
A: Only if getting errors
   Normal operation: no need to format
   If issues: format to FAT32 and reload files
```

**Q: Is RTC battery replaceable?**
```
A: Yes! CR2032 coin cell
   Lasts 5+ years
   Simple to replace (pry out and install new one)
```

---

## Emergency Fixes (When All Else Fails)

### "Hard Reset" Procedure

```
1. Disconnect USB cable from ESP32
2. Remove SD card from module
3. Wait 10 seconds
4. Reinsert SD card
5. Reconnect USB cable
6. Watch Serial Monitor startup sequence
7. Type: INFO
```

### Complete Reset to Factory Defaults

```
1. Open Serial Monitor (115200)
2. Type: RESETPARAM
3. Wait for message: "✓ Settings reset to defaults"
4. Type: TIME,2025,10,31,14,30,45 (set correct time)
5. Power cycle by disconnecting USB
6. Reconnect USB
7. System should be fully reset
```

### Recovery from Corrupted SD Card

```
1. Remove SD card from module
2. Insert into card reader on computer
3. Format to FAT32 (all files deleted)
4. Reinsert into module
5. Power on ESP32
6. Watch for "File initialization complete"
7. New empty files will be created
```

---

## When to Replace Components

| Component | Signs of Failure |
|-----------|-----------------|
| OLED | No display even with I2C detected |
| RTC | Doesn't detect on I2C, time resets |
| SD Module | Fails all initialization speeds |
| ESP32 | Won't program, no USB detection |
| Button | No continuity in multimeter test |
| SD Card | Can't format, always errors |

---

## Getting Professional Help

If none of this works:

1. **Document everything**:
   - Take photo of wiring
   - Screenshot of serial monitor output
   - Record what you've tried
   - Note exact error messages

2. **Check online resources**:
   - Adafruit forums
   - Arduino community
   - ESP32 GitHub issues
   - Electronics Stack Exchange

3. **Provide information**:
   - Complete wiring diagram
   - Serial monitor output
   - Code version used
   - Component part numbers
   - Steps already tried

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | Oct 31, 2025 | Initial troubleshooting guide |

---

**Pro Tips** 💡:
- Always check serial monitor baud rate first!
- Take photos of working setup
- Keep a backup of good SD card files
- Document your parameter settings
- Keep spare parts (buttons, SD cards)

**Happy troubleshooting!** 🔧
