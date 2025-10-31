# EEPROM Settings Persistence Guide

## Overview
The ESP32 now supports saving all parameter settings to its internal flash memory (EEPROM). Settings will persist even after power cycles or disconnections.

## What Gets Saved
The following settings are stored in EEPROM:
- **Save Interval** - How often counts are saved to SD card
- **Debounce Delay** - Button debounce time (critical for performance)
- **Max Count** - Maximum count value before capping
- **Status Display Time** - How long status messages appear on OLED

## Serial Commands

### Quick Save Command
```
SAVE
```
**Use this to save all current settings to EEPROM**

Example:
```
SET,2,100
SAVE
```
Changes debounce to 100ms and saves it permanently.

### Reset Parameters to Defaults
```
RESETPARAM
```
**Resets ALL parameters back to original defaults and saves them**

- Save Interval: 5000 ms
- Debounce Delay: 50 ms
- Max Count: 9999
- Status Display Time: 3000 ms

## Workflow Example

### First Time Setup:
```
INFO                    (View current settings)
SET,2,75                (Change debounce to 75ms)
SET,1,3000              (Change save interval to 3000ms)
SAVE                    (Save settings to EEPROM)
```

Now, even after power cycle or disconnection, these settings will be loaded automatically on startup!

### Quick Debounce Adjustment:
```
DEBOUNCE,60             (Quick set to 60ms)
SAVE                    (Make it permanent)
```

## How It Works

1. **On Startup**: 
   - EEPROM is checked for valid saved settings
   - If found, they're loaded and used
   - If not found, defaults are used (and saved for next time)

2. **During Runtime**:
   - You can change any parameter with SET, DEBOUNCE, etc.
   - Changes are **temporary** until you use SAVE command

3. **After SAVE**:
   - Settings are written to ESP32's internal flash
   - They persist through power cycles, reboots, and disconnections

## Technical Details

### EEPROM Memory Layout
```
Address  Size    Parameter
0        4 bytes Save Interval
4        4 bytes Debounce Delay
8        4 bytes Max Count
12       4 bytes Status Display Time
16       1 byte  Magic Number (validation)
```

### Magic Number
- **0xAB** = Valid data in EEPROM
- If magic number doesn't match, EEPROM is considered empty

## Important Notes

⚠️ **Remember to SAVE!**
- Parameter changes are temporary
- Use `SAVE` command to make them permanent
- Without SAVE, settings revert after power cycle

⚠️ **Safe Ranges**
- All parameters are validated for safe ranges
- Invalid values are rejected automatically

✓ **Persistence**
- Settings survive power loss
- Settings survive board disconnections
- Settings survive software updates (unless board is reflashed completely)

## Troubleshooting

**Settings not persisting?**
- Make sure you used `SAVE` command after changing parameters
- Check serial output for "✓ Settings saved to EEPROM" message

**Want fresh start?**
- Use `RESETPARAM` command to reset everything to defaults

**Lost calibration?**
- Type `INFO` to see current settings
- Type `SAVE` to re-save them if needed

---

**Example Session:**
```
INFO
Current Settings:
  1. Save Interval:        3000 ms
  2. Debounce Delay:       75 ms
  3. Max Count:            9999
  4. Status Display Time:  3000 ms

Count Status:
  Current Count:           245
  Hourly Count:            312
  Cumulative Count:        2150

Commands:
  SET,1,<value>  - Change Save Interval (ms) [1000-60000]
  SET,2,<value>  - Change Debounce Delay (ms) [10-500]
  SET,3,<value>  - Change Max Count [100-99999]
  SET,4,<value>  - Change Status Display Time (ms) [1000-30000]
  DEBOUNCE,<value> - Quick set debounce delay (ms) [10-500]
  SAVE           - Save all settings to EEPROM
  RESET          - Reset Current Count to 0
  RESETPARAM     - Reset all parameters to defaults
  TIME,YYYY,MM,DD,HH,MM,SS - Set RTC time
  INFO           - Show this menu again
```
