# ESP32 Counter System - Wiring Diagram & Pin Reference

## Complete Wiring Diagram

```
                          ┌─────────────┐
                          │   ESP32     │
                          │  Dev Board  │
                          └─────────────┘
                             │       │
         ┌────────────────────┼───────┼─────────────┐
         │                    │       │             │
         │              GPIO21(SDA)   │             │
         │              GPIO22(SCL)   │             │
         │                    │       │             │
    ┌────▼─────────┐      ┌──────────┐          ┌──┴────────┐
    │   OLED 0x3C  │      │ RTC DS   │          │  GPIO 15  │
    │  128x64 I2C  │      │  3231    │          │  Counter  │
    ├──────────────┤      ├──────────┤          │  Button   │
    │ SDA ─────────┼──────┤ SDA      │          │           │
    │ SCL ─────────┼──────┤ SCL      │     ┌────┤ Pin ──┐   │
    │ VCC ─────────┼──────┤ VCC      │     │    │      GND   │
    │ GND ─────────┼──────┤ GND      │     │    └───────┘    │
    │              │      │          │     │   10kΩ ↑        │
    │              │      │          │     │   Res. to 3.3V  │
    └──────────────┘      └──────────┘     │                 │
         (All on I2C                       └─────────────────┘
          GPIO 21/22)
          
                              ┌────────────┐
                              │  GPIO 27   │
                              │ Diagnostic │
                              │  Button    │
                              │            │
                          ┌───┤ Pin ──┐    │
                          │   │       GND  │
                          │   └────────┘   │
                          │   10kΩ ↑       │
                          │   Res. to 3.3V │
                          └────────────────┘

         ┌─────────────────────────────────┐
         │     SD Card Module (SPI)        │
         ├─────────────────────────────────┤
         │ GPIO 18 (CLK)   ─── CLK         │
         │ GPIO 19 (MISO)  ─── MISO        │
         │ GPIO 23 (MOSI)  ─── MOSI        │
         │ GPIO 26 (CS)    ─── CS          │
         │ 3.3V            ─── VCC         │
         │ GND              ─── GND         │
         └─────────────────────────────────┘
```

---

## Detailed Pin Connections

### I2C Bus (GPIO 21, 22)
Used for both OLED and RTC modules.

```
┌─────────────────────────────┐
│     I2C Bus (Shared)        │
├─────────────────────────────┤
│ GPIO 21 (SDA)               │
│  ├─→ OLED SDA               │
│  └─→ RTC SDA                │
│                             │
│ GPIO 22 (SCL)               │
│  ├─→ OLED SCL               │
│  └─→ RTC SCL                │
│                             │
│ 3.3V Power                  │
│  ├─→ OLED VCC               │
│  ├─→ RTC VCC                │
│  └─→ Pull-up resistors      │
│                             │
│ GND                         │
│  ├─→ OLED GND               │
│  └─→ RTC GND                │
└─────────────────────────────┘
```

### SPI Bus (GPIO 18, 19, 23, 26)
Used exclusively for SD Card module.

```
┌─────────────────────────────┐
│     VSPI Bus (SPI3)         │
├─────────────────────────────┤
│ GPIO 18 (CLK)               │
│  └─→ SD CLK                 │
│                             │
│ GPIO 19 (MISO)              │
│  └─→ SD MISO                │
│                             │
│ GPIO 23 (MOSI)              │
│  └─→ SD MOSI                │
│                             │
│ GPIO 26 (CS)                │
│  └─→ SD CS (Chip Select)    │
│                             │
│ 3.3V Power                  │
│  └─→ SD VCC                 │
│                             │
│ GND                         │
│  └─→ SD GND                 │
└─────────────────────────────┘
```

### Button Connections

#### Counter Button (GPIO 15)
```
    3.3V
      │
      ├─[10kΩ Resistor]─┬─── GPIO 15
      │                 │
      │          ┌──────┘
      │          │
      │       [Button]
      │          │
     GND────────┴─── GND
```

#### Diagnostic Button (GPIO 27)
```
    3.3V
      │
      ├─[10kΩ Resistor]─┬─── GPIO 27
      │                 │
      │          ┌──────┘
      │          │
      │       [Button]
      │          │
     GND────────┴─── GND
```

---

## Component-by-Component Wiring

### OLED Display (Adafruit SSD1306, 128x64)

| OLED Pin | ESP32 Pin | Color (Suggested) |
|----------|-----------|-------------------|
| GND      | GND       | Black             |
| VCC      | 3.3V      | Red               |
| SCL      | GPIO 22   | Yellow            |
| SDA      | GPIO 21   | Green             |

**Notes**:
- I2C address: 0x3C
- Requires 3.3V (NOT 5V)
- Can share I2C bus with RTC

### RTC Module (DS3231)

| RTC Pin  | ESP32 Pin | Color (Suggested) |
|----------|-----------|-------------------|
| GND      | GND       | Black             |
| VCC      | 3.3V      | Red               |
| SCL      | GPIO 22   | Yellow            |
| SDA      | GPIO 21   | Green             |

**Notes**:
- I2C address: 0x68
- Shares I2C bus with OLED
- Has backup battery (CR2032)
- Time persists when powered off

### SD Card Module

| SD Pin   | ESP32 Pin | Color (Suggested) |
|----------|-----------|-------------------|
| GND      | GND       | Black             |
| VCC      | 3.3V      | Red               |
| CLK      | GPIO 18   | Orange            |
| MOSI     | GPIO 23   | Brown             |
| MISO     | GPIO 19   | Purple            |
| CS       | GPIO 26   | Gray              |

**Notes**:
- Uses SPI3 (VSPI) bus
- DO NOT share with other SPI devices
- Format SD card to FAT32

### Counter Button (GPIO 15)

| Component | Connection |
|-----------|------------|
| Button A  | GPIO 15    |
| Button B  | GND        |
| Resistor  | 10kΩ       |
| Resistor Terminal 1 | 3.3V |
| Resistor Terminal 2 | GPIO 15 (junction with button) |

### Diagnostic Button (GPIO 27)

| Component | Connection |
|-----------|------------|
| Button A  | GPIO 27    |
| Button B  | GND        |
| Resistor  | 10kΩ       |
| Resistor Terminal 1 | 3.3V |
| Resistor Terminal 2 | GPIO 27 (junction with button) |

---

## Power Distribution

### 3.3V Power Rail
Connect all these to same 3.3V source:
- OLED VCC
- RTC VCC
- SD Card VCC
- Button pull-up resistors (2x 10kΩ to 3.3V)

⚠️ **Total current ~500mA max**
- ESP32 can supply 500mA from 3.3V
- Typical usage: 100-200mA
- Headroom: adequate for this system

### GND Reference
All GND connections tied together:
- ESP32 GND (multiple pins if available)
- OLED GND
- RTC GND
- SD Card GND
- Button GND
- Optional: External ground plane on breadboard

---

## Pin Summary Table

### All GPIO Pins Used

| Pin | Function | Voltage | Pull-up |
|-----|----------|---------|---------|
| GPIO 15 | Counter Button | 3.3V | Yes (10kΩ) |
| GPIO 18 | SD CLK | 3.3V | No |
| GPIO 19 | SD MISO | 3.3V | No |
| GPIO 21 | I2C SDA | 3.3V | Internal |
| GPIO 22 | I2C SCL | 3.3V | Internal |
| GPIO 23 | SD MOSI | 3.3V | No |
| GPIO 26 | SD CS | 3.3V | No |
| GPIO 27 | Diagnostic Button | 3.3V | Yes (10kΩ) |

### Available Pins (Unused)
GPIO 0, 2, 4, 5, 12, 13, 14, 16, 17, 25, 32, 33, 34, 35, 36, 39

---

## Troubleshooting by Connection

### OLED Display Not Working
```
Check:
1. GPIO 21 (SDA) connected to OLED SDA
2. GPIO 22 (SCL) connected to OLED SCL
3. OLED 3.3V connected to ESP32 3.3V (NOT 5V!)
4. OLED GND connected to ESP32 GND
5. I2C address is 0x3C (run I2C scanner)
```

### RTC Not Responding
```
Check:
1. GPIO 21 (SDA) connected to RTC SDA
2. GPIO 22 (SCL) connected to RTC SCL
3. RTC 3.3V connected to ESP32 3.3V
4. RTC GND connected to ESP32 GND
5. RTC I2C address is 0x68 (run I2C scanner)
6. RTC battery installed (CR2032)
```

### SD Card Initialization Failed
```
Check:
1. GPIO 18 (CLK) → SD CLK
2. GPIO 19 (MISO) → SD MISO
3. GPIO 23 (MOSI) → SD MOSI
4. GPIO 26 (CS) → SD CS
5. SD Card VCC connected to 3.3V
6. SD Card GND connected to ESP32 GND
7. SD card formatted to FAT32
8. SD card not write-protected
```

### Button Not Responding
```
Check:
1. GPIO 15 (Counter) or GPIO 27 (Diagnostic) connected
2. Button wired to GND on other side
3. 10kΩ pull-up resistor installed
4. Pull-up resistor connects to 3.3V
5. Connections not loose (wiggle gently, no change = good)
```

---

## Power Considerations

### Current Draw Estimate
```
Component          Typical Current
───────────────────────────────────
ESP32              80-160 mA
OLED Display       15-20 mA
RTC Module         0.5-1 mA
SD Card Module     50-100 mA (varies with operation)
───────────────────────────────────
Total Typical      150-300 mA
Total Peak         ~500 mA
```

### USB Power Supply Requirements
- **Minimum**: 1A (safe margin)
- **Recommended**: 2A (for stable operation)
- **Type**: Micro-USB with 3.3V LDO regulator built into ESP32

---

## Best Practices

### Breadboard Layout
```
Left side:           Middle:           Right side:
3.3V rail            OLED              Buttons
GND rail             RTC               Pull-up Res.
SD Module            Connections
```

### Cable Organization
- Use different colors for different signals
- Keep I2C lines close together (short)
- Keep SPI lines close together (short)
- Keep power and ground lines separate from signal lines
- Avoid crossing signal lines with power lines

### Connector Tips
- Use female headers for reusable connections
- Add dupont connectors for easy disconnect
- Label each connection with tape
- Double-check before powering on

---

## Quick Wiring Checklist

### Pre-Assembly
- [ ] All components identified and working
- [ ] Breadboard and jumper wires ready
- [ ] Pull-up resistors (2x 10kΩ) available

### Assembly
- [ ] I2C devices (OLED, RTC) connected
- [ ] SD Card module connected on SPI
- [ ] Counter button wired to GPIO 15
- [ ] Diagnostic button wired to GPIO 27
- [ ] All GND connections tied together
- [ ] All 3.3V connections tied together
- [ ] No exposed wires crossing
- [ ] All connections secure (gentle tug test)

### Testing
- [ ] Upload code to ESP32
- [ ] Serial monitor shows startup messages
- [ ] OLED displays "Starting..."
- [ ] Button increments counter
- [ ] No magic smoke (good sign!)

---

## Safety Reminders

⚠️ **CRITICAL**: ESP32 GPIO = 3.3V MAX
- Do NOT apply 5V to any GPIO pin
- Do NOT apply more than 3.3V to buttons
- Do NOT short 3.3V to GND
- Do NOT insert/remove modules while powered

✓ **Safe Practices**:
- Power off before wiring changes
- Double-check all connections
- Use regulated 5V USB supply
- Use current-limiting USB cable

---

**Diagram Version**: 1.0  
**Last Updated**: October 31, 2025

**Pro Tip**: Take photos of your working setup for future reference! 📸
