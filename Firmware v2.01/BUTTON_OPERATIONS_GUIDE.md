# Button Operations Guide - Visual Reference

## Button Overview

Your Production Counter has 3 buttons with different functions:

### Button 1: Counter Button (GPIO 15)
**Purpose:** Count items/events

### Button 2: Production Latch (GPIO 25) ⭐ NEW
**Purpose:** Toggle production ON/OFF

### Button 3: Diagnostic (GPIO 27)
**Purpose:** Run system tests

---

## Button 1: Counter Button (GPIO 15)

### Physical Setup
```
   ┌──────────────┐
   │  ESP32 Board │
   └──────────────┘
         │
    GPIO 15 ──[Button]──── GND
         │
      (pulled up internally)
```

### Function
- **Press:** Increments count by 1
- **Response Time:** Immediate (interrupt-based)
- **Debounce:** 50ms (prevents accidental double-counting)
- **Max Count:** 9999

### LED/Display Response
```
Before press:   Count: 00042
Press button
After press:    Count: 00043
                (updates to LCD immediately)
```

### Serial Output
```
[No output - counted silently]
[Count saved every 5 seconds]
```

### What Gets Counted?
- Real-time count (always incremented)
- Session count (if production is active)
- Hourly count (resets at hour boundary)

---

## Button 2: Production Latch (GPIO 25) ⭐ NEW FEATURE

### Physical Setup
```
   ┌──────────────┐
   │  ESP32 Board │
   └──────────────┘
         │
    GPIO 25 ──[Button]──── GND
         │
      (pulled up internally)
```

### Function
- **Press:** Toggles production ON ↔ OFF
- **Debounce:** 500ms (latching effect)
- **Creates:** Production log file on transition
- **Auto-Logs:** Start time, end time, item count

### State Machine

```
┌─────────────────────────────────────────┐
│            INITIAL STATE                │
│            [IDLE]                       │
│  LCD shows: [IDLE]                      │
│  Count display: Active                  │
│  Session counter: N/A                   │
└─────────────────────────────────────────┘
         │
         │ (Press GPIO 25)
         ↓
┌─────────────────────────────────────────┐
│   ACTION: PRODUCTION STARTING           │
│  Create log file: /PROD_*.txt           │
│  Start time: [recorded]                 │
│  Session count: [reset to 0]            │
└─────────────────────────────────────────┘
         │
         ↓
┌─────────────────────────────────────────┐
│            PRODUCTION ACTIVE            │
│            [PROD]                       │
│  LCD shows: [PROD] with time            │
│  Count display: Active                  │
│  Session counter: Active (separate)     │
│  Log file status: OPEN                  │
└─────────────────────────────────────────┘
         │
    Press counter button
    N times to count items
         │
         │ (Press GPIO 25 again)
         ↓
┌─────────────────────────────────────────┐
│   ACTION: PRODUCTION STOPPING           │
│  Update log file with end time          │
│  Record final count                     │
│  Log file status: CLOSED                │
└─────────────────────────────────────────┘
         │
         ↓
┌─────────────────────────────────────────┐
│            PRODUCTION IDLE              │
│            [IDLE]                       │
│  LCD shows: [IDLE] with time            │
│  Count display: Shows new session       │
│  Session counter: Reset                 │
│  Log file: Saved to SD card             │
└─────────────────────────────────────────┘
```

### Display Changes

**BEFORE Pressing Production Button:**
```
Row 0: [IDLE] 14:32:15
Row 1: Count: 00042
Row 2: Hour: 00156
Row 3: Ready
```

**AFTER Pressing Production Button:**
```
Row 0: [PROD] 14:32:15
Row 1: Count: 00042
Row 2: Session: 00000
Row 3: S:14:32:15
```

**AFTER Pressing Again (stopping production):**
```
Row 0: [IDLE] 14:32:20
Row 1: Count: 00055
Row 2: Hour: 00156
Row 3: PROD STOPPED

(brief status message, then back to normal)
```

### Log File Created/Updated

**When starting production (GPIO 25 press #1):**
```
/PROD_20251106_143215.txt
Created with:
=== PRODUCTION LOG ===
Started: 2025-11-06 14:32:15
End: [WAITING]
Count: 0
```

**When stopping production (GPIO 25 press #2):**
```
/PROD_20251106_143215.txt
Updated with:
=== PRODUCTION LOG ===
Started: 2025-11-06 14:32:15
Ended: 2025-11-06 14:35:45
Count: 13
```

### Serial Output

**When Starting Production:**
```
>>> Production started <<<
✓ Production log created: /PROD_20251106_143215.txt
```

**When Stopping Production:**
```
>>> Production stopped <<<
✓ Production log updated: /PROD_20251106_143215.txt
  Start: 2025-11-06 14:32:15
  End: 2025-11-06 14:35:45
  Count: 13
```

### Typical Production Session

```
TIME: 14:30:00
User action: Press GPIO 25

LCD shows: [PROD] 14:30:00

TIME: 14:30:15
User action: Press counter button 1 time
LCD updates: Count: 00001, Session: 00001

TIME: 14:30:45
User action: Press counter button 4 times (total 5)
LCD updates: Count: 00005, Session: 00005

TIME: 14:31:30
User action: Press GPIO 25 (stop production)

LCD shows: [IDLE] 14:31:30
SD Card: Log file saved with Count: 5

User can start new production session
or continue idle counting
```

---

## Button 3: Diagnostic Button (GPIO 27)

### Physical Setup
```
   ┌──────────────┐
   │  ESP32 Board │
   └──────────────┘
         │
    GPIO 27 ──[Button]──── GND
         │
      (pulled up internally)
```

### Function
- **Press:** Enters diagnostic mode
- **Hold:** 200ms+ to trigger
- **Exit:** Press again to return to normal
- **Tests:** LCD, SD card, RTC

### Diagnostic Screen

```
┌──────────────────┐
│    DIAGNOSTIC    │
│ LCD:   PASS      │
│ SD:    PASS      │
│ RTC:   PASS      │
│                  │
└──────────────────┘

After testing completes:

┌──────────────────┐
│    DIAGNOSTIC    │
│ LCD:   PASS      │
│ SD:    PASS      │
│ RTC:   PASS      │
│ Score: 3/3 (100%)│
│                  │
│ Press to exit    │
└──────────────────┘
```

### Serial Output During Diagnostics

```
>>> DIAGNOSTIC MODE <<<

Running tests...

LCD:   PASS
SD:    PASS
RTC:   PASS

Score: 3/3 (100%)

>>> Normal mode <<<
```

### Exit Diagnostics
Press the diagnostic button again to exit and return to normal operation.

---

## Combined Button Operations

### Scenario 1: Normal Counting Only

```
Press GPIO 15 multiple times
├─ Count increments each time
├─ LCD updates immediately
├─ SD card auto-saves every 5 seconds
└─ No production log created
```

### Scenario 2: Production Session

```
Press GPIO 25 (START)
├─ Production status: ON
├─ Log file created
├─ LCD shows [PROD]
│
Press GPIO 15 multiple times
├─ Count increments
├─ Session count increments
├─ Both shown on LCD
│
Press GPIO 25 (STOP)
├─ Production status: OFF
├─ Log file updated with time & count
├─ LCD shows [IDLE]
└─ Session data saved
```

### Scenario 3: Testing System

```
Press GPIO 27
├─ Enter diagnostic mode
├─ LCD shows test results
├─ Serial shows test status
│
Press GPIO 27 again
├─ Exit diagnostic mode
├─ Return to normal operation
└─ Counts preserved
```

---

## Response Time Summary

| Button | Function | Debounce | Response | Effect |
|--------|----------|----------|----------|--------|
| GPIO 15 | Counter | 50ms | Immediate | Increment count |
| GPIO 25 | Production | 500ms | ~1 second | Toggle mode + log file |
| GPIO 27 | Diagnostic | 200ms | ~2 seconds | Enter test mode |

---

## Understanding the Latching Button (GPIO 25)

### Why "Latching"?

The production button is called **latching** because:

1. **First Press:** "Latches" production ON
   - Like turning on a light switch
   - Stays ON until next press

2. **Second Press:** "Unlatches" production OFF
   - Like turning off the light switch
   - Stays OFF until next press

3. **Not Momentary:** 
   - ✗ Hold button = not needed
   - ✓ Toggle on/off = that's all you need

### Visual Latch Analogy

```
Position 1 (OFF):           Position 2 (ON):
┌─────────┐                 ┌─────────┐
│  PROD   │                 │  PROD   │
│ [IDLE]  │                 │ [ACTIVE]│
│ Switch  │                 │ Switch  │
│  DOWN   │                 │   UP    │
└─────────┘                 └─────────┘
    ↓                           ↑
    └─────── Press Once ────────┘
              (toggles)
```

---

## LED Indicator (if you add one)

### Optional Status LED

You can add an LED to show production status:

```
RED LED (production status):
├─ OFF = Production not active [IDLE]
└─ ON = Production active [PROD]

GREEN LED (system status):
├─ Blinking = Counting
├─ Steady = Ready/waiting
└─ Off = Error
```

---

## Troubleshooting Button Issues

### Counter Button (GPIO 15) not counting?
- [ ] Check physical connection
- [ ] Verify button press reaches ground
- [ ] Test in diagnostic mode
- [ ] Check debounce setting

### Production Button (GPIO 25) not toggling?
- [ ] Check physical connection to GND
- [ ] Try pressing longer
- [ ] Check 500ms debounce not too high
- [ ] Test in diagnostic mode
- [ ] Verify log files on SD card

### Diagnostic Button (GPIO 27) not working?
- [ ] Check physical connection
- [ ] Hold for 200ms+ minimum
- [ ] Verify LCD shows diagnostic screen
- [ ] Check serial output

---

## Advanced Button Configuration

### Change Debounce Delay (Serial Command)

```
// Current debounce: 50ms (counter button)
SET,2,50

// Make it faster: 25ms
SET,2,25

// Make it slower: 100ms
SET,2,100

// Save for next power-on
SAVE
```

### Production Button Debounce
Currently hardcoded to 500ms in code. To change:

Find in Code_v2.cpp:
```cpp
if (now - lastPress > 500) {  // Change 500 to your value
  productionActive = !productionActive;
  productionStateChanged = true;
  lastPress = now;
}
```

---

## Safety Notes

✓ All buttons use GPIO INPUT_PULLUP (built-in pullup resistors)
✓ Buttons connect directly to ground (no external resistors needed)
✓ Debouncing prevents accidental triggers
✓ Production logging is automatic (no manual intervention)
✓ Safe to press multiple times rapidly (debounce handles it)

---

**Last Updated:** November 6, 2025
