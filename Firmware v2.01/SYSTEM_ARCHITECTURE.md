# System Architecture Diagrams

## Overall System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32 PRODUCTION COUNTER                  │
│                    System Architecture v2.0                  │
└─────────────────────────────────────────────────────────────┘

┌──────────────────────┐
│   ESP32 Dev Board    │
│  (Microcontroller)   │
└──────────────────────┘
    │         │        │
    │         │        └─────────────────┬────────────────┐
    │         │                          │                │
    │    I2C  │                      SPI Bus         GPIO Pins
    │    Bus  │                      (VSPI)
    │    │    │                      │
    ├────┴────┬──────────┐           ├─────────────────┐
    │          │          │           │                 │
    │       GPIO 21   GPIO 22      GPIO 18          GPIO 15
    │       (SDA)      (SCL)       (SCK)            Counter
    │          │          │           │
    ├─ LCD 16x4           │       GPIO 19    GPIO 25      GPIO 27
    │  PCF8574          GPIO 22    (MISO)   Production  Diagnostic
    │  0x27 or 0x3F      (SCL)         │      Button     Button
    │                      │       GPIO 23
    ├─ RTC DS3231           │       (MOSI)
    │  0x68                 │           │
    │                       │       GPIO 26
    └─────────────────────┐ │         (CS)
                          │ │           │
                      ┌───┴─┴───────────┐
                      │                 │
                    Power            SD Card
                   Supply            Module
                   (5V)          (with data)
                 (3.3V)
```

---

## Data Flow Diagram

```
COUNTING PROCESS:
┌──────────────┐
│ Item Passes  │
│   Sensor     │
└──────────────┘
       │
       └─ GPIO 15 Signal (press)
            │
            ├─ Interrupt Handler
            │  └─ handleInterrupt()
            │
            ├─ currentCount++
            │
            ├─ If Production Active:
            │  └─ productionSessionCount++
            │
            ├─ Update LCD Display
            │
            └─ Save to SD (every 5 sec)


PRODUCTION SESSION PROCESS:
┌──────────────┐
│ Production   │
│  Button      │
│ (GPIO 25)    │
└──────────────┘
       │ Press
       ├─ Interrupt Handler
       │  └─ handleProductionLatch()
       │
       ├─ Toggle productionActive
       │
       ├─ If Starting:
       │  ├─ Create log file
       │  ├─ Record start time
       │  └─ Reset session count
       │
       ├─ If Stopping:
       │  ├─ Update log file
       │  ├─ Record end time
       │  └─ Save session count
       │
       ├─ Update LCD display
       │
       └─ Save to SD card
```

---

## I2C Bus Configuration

```
ESP32 I2C Bus (GPIO 21=SDA, GPIO 22=SCL)
────────────────────────────────────────

     ┌─────────────────────────────┐
     │      ESP32 (Master)         │
     │  GPIO 21 (SDA)              │
     │  GPIO 22 (SCL)              │
     └──────────┬────────┬─────────┘
                │        │
        ┌───────┘        └────────┐
        │                         │
        │ SDA/SCL                SDA/SCL
        │ Address 0x27           Address 0x68
        │                        (optional)
    ┌───────┐                ┌──────────┐
    │  LCD  │                │   RTC    │
    │16x4   │                │ DS3231   │
    │PCF8574│                │ Module   │
    │Backpk │                │ (if used)│
    └───────┘                └──────────┘

Address Range:
0x27 or 0x3F - LCD (most common 0x27)
0x68         - RTC (standard address)
```

---

## SPI Bus Configuration

```
ESP32 SPI Bus (VSPI - SPI3)
────────────────────────────

     ┌──────────────────────────┐
     │   ESP32 (SPI Master)     │
     │  GPIO 18 (SCK)           │
     │  GPIO 19 (MISO)          │
     │  GPIO 23 (MOSI)          │
     │  GPIO 26 (CS)            │
     └────┬─────┬─────┬────┬────┘
          │     │     │    │
    SCK   │ MISO│MOSI │    │ CS
          │     │     │    │
    ┌─────┴─────┴─────┴────┴────┐
    │                            │
    │      SD Card Module        │
    │                            │
    │   CLK  - GPIO 18           │
    │   MISO - GPIO 19           │
    │   MOSI - GPIO 23           │
    │   CS   - GPIO 26           │
    │   GND  - Ground            │
    │   3.3V - 3.3V Power        │
    │                            │
    └────────────────────────────┘
```

---

## GPIO Pin Assignment

```
ESP32 Development Board Pin Map

         GND ─────  5V
         GND ─────  5V
         GND ─────  3.3V
      GPIO 15 ─── BUTTON (Counter)
      GPIO 2  ─── (unused)
      GPIO 4  ─── (unused)
      GPIO 5  ─── (unused)
      GPIO 18 ─── SPI Clock (SD)
      GPIO 19 ─── SPI MISO  (SD)
      GPIO 21 ─── I2C SDA   (LCD/RTC)
      GPIO 22 ─── I2C SCL   (LCD/RTC)
      GPIO 23 ─── SPI MOSI  (SD)
      GPIO 25 ─── BUTTON (Production) ← NEW
      GPIO 26 ─── SPI CS    (SD)
      GPIO 27 ─── BUTTON (Diagnostic)
      GPIO 32 ─── (unused)
      GPIO 33 ─── (unused)
      GPIO 35 ─── (unused)
```

---

## State Machine: Production Session

```
                    ┌─────────────┐
                    │    IDLE     │
                    │  [Display]  │
                    │  ┌─────────┐│
                    │  │[IDLE]   ││
                    │  │Count: X ││
                    │  │Hour: Y  ││
                    │  └─────────┘│
                    └──────┬──────┘
                           │
                    Press GPIO 25
                           │
                           ├─ Set productionActive = true
                           ├─ Create log file
                           ├─ Record start time
                           └─ Reset sessionCount = 0
                           │
                           ↓
                    ┌─────────────────┐
                    │  PRODUCTION     │
                    │    ACTIVE       │
                    │  [Display]      │
                    │  ┌───────────┐  │
                    │  │[PROD] HH:MM│  │
                    │  │Count: X   │  │
                    │  │Session: X │  │
                    │  │S:HH:MM:SS │  │
                    │  └───────────┘  │
                    └─────────┬────────┘
                              │
                       Press Counter
                       (GPIO 15)
                       N times
                              │
                    ┌─ Count increments
                    └─ Session count increments
                              │
                       Press GPIO 25
                              │
                              ├─ Set productionActive = false
                              ├─ Update log file
                              ├─ Record end time
                              └─ Save sessionCount
                              │
                              ↓
                    ┌──────────────────┐
                    │  LOG FILE SAVED  │
                    │  To SD card:     │
                    │ /PROD_*.txt      │
                    │ Start time ✓     │
                    │ End time ✓       │
                    │ Count ✓          │
                    └────────┬─────────┘
                             │
                             ↓
                    ┌─────────────┐
                    │    IDLE     │
                    │  [Display]  │
                    │  ┌─────────┐│
                    │  │[IDLE]   ││
                    │  │Count: +X││
                    │  │Hour: Y  ││
                    │  └─────────┘│
                    └─────────────┘
                    (Ready for next session)
```

---

## Data Flow: SD Card Logging

```
┌──────────────────────────────────┐
│  System Boot                     │
│  (Load counts from files)        │
└──────────────┬───────────────────┘
               │
               ├─ Read /count.txt
               ├─ Read /hourly_count.txt
               └─ Read /cumulative_count.txt
               │
        ┌──────┴─────────┐
        │                │
    ┌───┴────┐       ┌───┴────┐
    │ Counter │       │Counting│
    │ Active  │       │Started │
    └───┬────┘       └───┬────┘
        │                │
   Auto-save          Create Log
   Every 5 sec    /PROD_*.txt
        │                │
        ├────────┬───────┘
        │        │
    Counting   Counting
     Stops     Stops
        │        │
        ├────────┴────────────────┐
        │                         │
        ├─ Update current count   ├─ Update log file
        ├─ Update hourly count    ├─ Add end time
        └─ Write to SD            └─ Write to SD
           /count.txt                /PROD_*.txt
           /hourly_count.txt
```

---

## Display Update Cycle

```
Main Loop (every 10ms)
├─ Check interrupts
│  ├─ Counter button (GPIO 15)
│  ├─ Production button (GPIO 25)
│  └─ Diagnostic button (GPIO 27)
│
├─ Update counters
│  ├─ currentCount
│  ├─ productionSessionCount
│  └─ hourlyCount
│
├─ Check timers
│  ├─ Save interval (5 sec)
│  ├─ Display update (500ms)
│  └─ Hour boundary
│
├─ Update LCD (if needed)
│  ├─ Row 0: Status + Time
│  ├─ Row 1: Current count
│  ├─ Row 2: Session/Hour
│  └─ Row 3: Production time/Status
│
└─ Save to SD (if time)
   └─ Write counts to files
```

---

## File System Structure on SD Card

```
SD Card Root
│
├─ count.txt (current count)
│  └─ Contains: "1234"
│
├─ hourly_count.txt (last hour's count)
│  └─ Contains: "567"
│
├─ cumulative_count.txt (total count)
│  └─ Contains: "9999"
│
├─ PROD_20251106_143025.txt (production session)
│  ├─ =================
│  ├─ PRODUCTION LOG
│  ├─ =================
│  ├─ Started: 2025-11-06 14:30:25
│  ├─ Ended: 2025-11-06 14:35:45
│  └─ Count: 42
│
├─ PROD_20251106_153100.txt (another session)
│  └─ (same format)
│
└─ 20251106_14[AM].txt (hourly log)
   ├─ Time: 2025-11-06 14:00:00
   ├─ Hour Count: 234
   └─ Cumulative: 5678
```

---

## Button Response Timeline

```
User presses Counter Button (GPIO 15)
│
├─ t=0ms      GPIO 15 goes LOW (button pressed)
│
├─ t=1-49ms   Debounce window (50ms)
│             Interrupt suppressed
│
├─ t=50ms     First valid interrupt detected
│
├─ t=50-51ms  handleInterrupt() executes
│             ├─ currentCount++
│             ├─ countChanged = true
│             └─ lastInterruptTime = updated
│
├─ t=51ms     Main loop detects countChanged
│
├─ t=51-100ms LCD display updates (if throttle allows)
│
├─ t=100-500ms SD card saves (if 5 sec interval met)
│
└─ t=500+ms   System ready for next press


User presses Production Button (GPIO 25)
│
├─ t=0ms      GPIO 25 goes LOW (button pressed)
│
├─ t=1-499ms  Debounce window (500ms)
│             Interrupt suppressed
│
├─ t=500ms    First valid interrupt detected
│
├─ t=500-501ms handleProductionLatch() executes
│              ├─ productionActive = !productionActive
│              ├─ productionStateChanged = true
│              └─ lastPress = updated
│
├─ t=501ms    Main loop detects productionStateChanged
│
├─ t=501-600ms Create/Update log file
│
├─ t=600-700ms LCD display updates
│
├─ t=700-1000ms Status message displays
│
└─ t=1000+ms   System ready for next press
```

---

## Hardware Connection Diagram

```
ESP32 Board (Top View)

                    USB Port
                       │
    ┌────────────────────┴────────────────────┐
    │                                         │
    │  [GND]  [5V]   [3.3V]   [GND]   [5V]   │
    │   │      │       │        │      │     │
    │   └──┬─┬─┘───┬─┬─┘───┬──┬┘      │     │
    │      │ │     │ │     │  │       │     │
    │  ┌───┴─┴─────┴─┴─────┴──┴───────┴─────┘
    │  │
    │  ├─ GPIO 15 (Counter Button)
    │  ├─ GPIO 25 (Production Button) ← NEW
    │  ├─ GPIO 27 (Diagnostic Button)
    │  ├─ GPIO 21 (I2C SDA)
    │  ├─ GPIO 22 (I2C SCL)
    │  ├─ GPIO 18 (SPI Clock)
    │  ├─ GPIO 19 (SPI MISO)
    │  ├─ GPIO 23 (SPI MOSI)
    │  └─ GPIO 26 (SPI CS)
    │
    └──────────────────────────────────────

External Connections:

GPIO 15 ─┬─ [Button] ─── GND
         │
         └─ 10kΩ ─── 3.3V (pullup - optional)

GPIO 25 ─┬─ [Button] ─── GND
         │
         └─ 10kΩ ─── 3.3V (pullup - optional)

GPIO 27 ─┬─ [Button] ─── GND
         │
         └─ 10kΩ ─── 3.3V (pullup - optional)

GPIO 21 ──────┬────────┬──────── I2C SDA
          LCD │        │ RTC
          P21 │        │ A4
GPIO 22 ──────┼────────┼──────── I2C SCL
          P22 │        │ A5
              └────────┘

GPIO 18 ──────── SPI Clock
GPIO 19 ──────── SPI MISO
GPIO 23 ──────── SPI MOSI
GPIO 26 ──────── SPI CS
      GND ──────── GND
    3.3V ──────── Power
          └─ SD Card Module
```

---

## System Initialization Sequence

```
START
  │
  ├─ Serial.begin(115200)
  │  └─ Initialize serial communication
  │
  ├─ Display startup banner
  │
  ├─ Load settings from EEPROM
  │
  ├─ Initialize I2C (GPIO 21, 22)
  │
  ├─ Initialize LCD
  │  └─ Display startup message
  │
  ├─ Configure SD chip select
  │
  ├─ Initialize SPI (VSPI/SPI3)
  │
  ├─ Initialize SD Card
  │  └─ Detect card and file system
  │
  ├─ Initialize RTC
  │  └─ Check/set time
  │
  ├─ Load counts from SD
  │  ├─ Load /count.txt
  │  ├─ Load /hourly_count.txt
  │  └─ Load /cumulative_count.txt
  │
  ├─ Setup GPIO interrupts
  │  ├─ GPIO 15 (Counter)
  │  ├─ GPIO 25 (Production) ← NEW
  │  └─ GPIO 27 (Diagnostic)
  │
  ├─ Display system ready message
  │
  └─ Enter main loop
     └─ Wait for button presses and counting
```

---

**Last Updated:** November 6, 2025
**System Version:** ESP32 Production Counter v2.0 (LCD Edition)

