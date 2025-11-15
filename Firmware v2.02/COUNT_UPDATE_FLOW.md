# Complete Count Update Flow - ESP32 Production Counter

This document explains exactly how each count file is updated in the system.

---

## File Types and Their Purpose

| File | Purpose | When Updated | Scope |
|------|---------|--------------|-------|
| `/count.txt` | Current production count | Every 5 seconds during production | Resets each hour |
| `/hourly_count.txt` | Count from last complete hour | Every hour when production stops | Last hour only |
| `/cumulative_count.txt` | Total of all hours combined | Every hour (always) | Lifetime total |
| `/Hour_YYYYMMDD_HHMM.txt` | Archived count for specific hour | Every hour when hour changes | That specific hour only |
| `/prod_session.txt` | Active production recovery file | Continuously during production | Current session only |

---

## Flow 1: DURING PRODUCTION (Main Loop - Every 5 seconds)

### Variables Being Incremented
```
When GPIO 15 button is pressed:
  currentCount++ (incremented in handleInterrupt ISR)
  countChanged = true
```

### Update Process (Line 653-666 in code)

```
EVERY 5 SECONDS (SAVE_INTERVAL):
│
├─ IF countChanged AND sdAvailable AND (now - lastSaveTime > 5000ms)
│  │
│  ├─ currentCount → /count.txt
│  │  (This is the real-time counter during production)
│  │
│  ├─ IF productionActive
│  │  └─ Save production state to /prod_session.txt (for power loss recovery)
│  │
│  └─ lastSaveTime = now
│
└─ IF countChanged OR needsFullRedraw
   └─ Update OLED display with current count
```

### Example During Production
```
Time: 14:00:00 - Production starts
  count.txt = 0
  currentCount = 0
  countAtHourStart = 0

Time: 14:00:15 - 15 units counted
  count.txt = 15 (saved)
  currentCount = 15
  hourly_count.txt = (not updated yet)
  cumulative_count.txt = (not updated yet)

Time: 14:01:00 - 50 units counted total
  count.txt = 50 (saved)
  currentCount = 50
  hourly_count.txt = (still not updated)
  cumulative_count.txt = (still not updated)
```

---

## Flow 2: EVERY HOUR (Main Loop - Every 1 second check)

### Hour Change Detection (Line 639-646 in code)

```
EVERY 1 SECOND:
│
└─ IF rtcNow.hour() != lastHour
   │  (Hour changed!)
   │
   └─ handleHourChange(rtcNow)
      └─ lastHour = rtcNow.hour()
```

### Inside handleHourChange() - ALWAYS Executes (Line 938-1002)

```
HOUR CHANGED (e.g., 14:00 → 15:00):
│
├─ Calculate: countThisHour = currentCount - countAtHourStart
│  (This is the count during THIS hour only)
│
├─ Save hourly file: /Hour_YYYYMMDD_HHMM.txt
│  └─ Contains: countThisHour value
│
├─ ACCUMULATE cumulative count:
│  └─ cumulativeCount += countThisHour
│     (ALWAYS happens, even if production is active)
│
└─ NOW BRANCH based on production status:
```

### Branch A: IF Production is NOT Active (Line 965-984)

```
PRODUCTION STOPPED CASE:
│
├─ currentCount = 0 (RESET for new hour)
│  count.txt = 0
│
├─ hourlyCount = finalCount (store old count)
│  hourly_count.txt = oldCount
│
├─ cumulative_count.txt = cumulativeCount (UPDATED)
│
├─ countAtHourStart = 0 (reset for next hour)
│
└─ Display "Hour Logged" message
```

### Branch B: IF Production IS Active (Line 987-1002)

```
PRODUCTION ACTIVE CASE:
│
├─ currentCount = PRESERVED (NOT reset, keeps counting)
│  count.txt = NOT updated (keeps current value)
│
├─ hourly_count.txt = NOT updated (only updates when production stops)
│
├─ cumulative_count.txt = UPDATED (ALWAYS updated)
│
├─ countAtHourStart = currentCount (update baseline for next hour)
│
└─ Display cumulative update message
```

### Example: Hour Change with Production

```
SCENARIO: Production active, hour changes 14:00 → 15:00

Before hour change:
  currentCount = 150
  countAtHourStart = 120 (set when production started)
  cumulativeCount = 500

When hour changes:
  countThisHour = 150 - 120 = 30 units in this hour
  
  /Hour_20251115_1400.txt created:
    └─ Count in this hour: 30
  
  cumulativeCount += 30 = 530
  cumulative_count.txt = 530 (SAVED)
  
  countAtHourStart = 150 (update baseline)
  currentCount = 150 (STAYS, production continues)
  count.txt = still shows 150
```

---

## Flow 3: WHEN PRODUCTION STOPS

### stopProduction() Function (Line 1338-1360)

```
PRODUCTION BUTTON RELEASED:
│
├─ productionActive = false
│
├─ Calculate: productionCount = currentCount - productionStartCount
│  (Total units counted during production)
│
├─ Save production session: /Production_YYYYMMDD_HHMM_to_HHMM.txt
│  └─ Contains session start, stop, and count
│
├─ Save hourly production: /HourlyProduction_YYYYMMDD.txt
│  └─ Appends session data to daily file
│
├─ Clear: /prod_session.txt (delete recovery file)
│
└─ On next hour change:
   └─ All counts will be properly saved and reset
      (because productionActive = false now)
```

---

## Flow 4: SYSTEM STARTUP / POWER LOSS RECOVERY

### On Boot (Line 451-458)

```
STARTUP:
│
├─ Read /count.txt → currentCount
├─ Read /hourly_count.txt → hourlyCount
├─ Read /cumulative_count.txt → cumulativeCount
│
└─ IF /prod_session.txt exists
   └─ Restore production state (recovering from power loss)
      ├─ productionActive = true
      ├─ currentCount = savedCount
      └─ Display "Production Recovered!"
```

---

## Summary Table: What Gets Updated When

| Event | count.txt | hourly_count.txt | cumulative_count.txt | Hour_*.txt | prod_session.txt |
|-------|-----------|------------------|----------------------|-----------|------------------|
| **Button press** | ✓ counts up | - | - | - | - |
| **Every 5 sec (saving)** | ✓ saved | - | - | - | ✓ (if active) |
| **Hour changes (prod OFF)** | ✓ reset to 0 | ✓ updated | ✓ updated | ✓ created | - |
| **Hour changes (prod ON)** | - | - | ✓ updated | ✓ created | ✓ updated |
| **Production stops** | - | - | - | - | ✓ deleted |

---

## Real-World Example: Full Day

```
08:00 AM - Production Starts (latch button pressed)
  currentCount = 0
  countAtHourStart = 0
  cumulative = 100 (from yesterday)
  count.txt = 0

08:15 AM - 25 units counted
  currentCount = 25
  count.txt = 25 (auto-save)

08:45 AM - 45 units counted total
  currentCount = 45
  count.txt = 45 (auto-save)

09:00 AM - HOUR CHANGES ✓
  countThisHour = 45 - 0 = 45
  Hour_20251115_0800.txt created (contains: 45)
  
  cumulativeCount += 45 = 145
  cumulative_count.txt = 145 ✓ SAVED
  
  currentCount = 45 (STAYS, production continues)
  countAtHourStart = 45 (new baseline)
  count.txt = 45 (unchanged)

10:00 AM - HOUR CHANGES ✓
  50 more units counted (95 total now)
  countThisHour = 95 - 45 = 50
  Hour_20251115_0900.txt created (contains: 50)
  
  cumulativeCount += 50 = 195
  cumulative_count.txt = 195 ✓ SAVED
  
  currentCount = 95 (STAYS)
  countAtHourStart = 95 (new baseline)

12:00 PM - Production STOPS (latch button released)
  productionCount = 95 - 0 = 95 units total
  Production_20251115_080000_to_120000.txt created
  prod_session.txt deleted
  
  Next hour change will complete the flow

13:00 PM - HOUR CHANGES ✓
  No more counting since production stopped
  countThisHour = 95 - 95 = 0
  Hour_20251115_1200.txt created (contains: 0)
  
  cumulativeCount += 0 = 195
  cumulative_count.txt = 195 (no change)
  
  currentCount = 0 (RESET now)
  countAtHourStart = 0 (reset)
  hourly_count.txt = 95 ✓ SAVED
  count.txt = 0 ✓ SAVED
```

---

## Key Takeaways

1. **count.txt** - Real-time counter, resets every hour (or when production stops)
2. **hourly_count.txt** - Only updated when production stops OR hour changes after production stops
3. **cumulative_count.txt** - ALWAYS updated every hour, regardless of production status
4. **Hour_*.txt** - Created every hour with only counts from that specific hour
5. **prod_session.txt** - Only exists during active production, provides recovery on power loss

Each file serves a specific purpose in tracking production at different time scales!
