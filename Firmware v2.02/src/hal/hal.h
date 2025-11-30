#ifndef HAL_H
#define HAL_H

#include <Arduino.h>

// ========================================
// GPIO ABSTRACTION LAYER
// ========================================
class GPIO {
public:
  enum Pin : uint8_t {
    COUNTER_BTN = 15,
    DIAG_BTN = 27,
    LATCH_BTN = 25,
    SD_CS = 26,
  };
  
  enum Mode : uint8_t {
    INPUT_PULLUP,
    OUTPUT,
    INPUT,
  };
  
  enum PinState : uint8_t {
    LOW = 0,
    HIGH = 1,
  };
  
  // Pin initialization
  static void init(Pin pin, Mode mode);
  
  // Pin reading
  static bool read(Pin pin);
  static PinState readState(Pin pin);
  
  // Pin writing
  static void write(Pin pin, bool value);
  static void setState(Pin pin, PinState state);
  
  // Interrupt handling
  typedef void (*ISRCallback)();
  static void attachInterrupt(Pin pin, ISRCallback handler, int mode);
  static void detachInterrupt(Pin pin);
  
  // All pins initialization
  static void initAll();
  
  // Debug/diagnostic
  static const char* getPinName(Pin pin);
};

// ========================================
// TIMER ABSTRACTION LAYER
// ========================================
class Timer {
public:
  // Timer types
  enum TimerType {
    TIMER_PERIODIC,    // Repeating timer
    TIMER_ONESHOT,     // Single execution
  };
  
  // Timer callback
  typedef void (*TimerCallback)();
  
  // Timer management
  static bool createTimer(uint8_t timerId, unsigned long intervalMs, 
                          TimerCallback callback, TimerType type = TIMER_PERIODIC);
  static void startTimer(uint8_t timerId);
  static void stopTimer(uint8_t timerId);
  static void deleteTimer(uint8_t timerId);
  
  // Timer queries
  static bool isRunning(uint8_t timerId);
  static unsigned long getElapsed(uint8_t timerId);
  static void setInterval(uint8_t timerId, unsigned long intervalMs);
  
  // Delay functions
  static void delay(unsigned long ms);
  static void delayMicroseconds(unsigned int us);
  
  // Timing utilities
  static unsigned long millis();
  static unsigned long micros();
};

// ========================================
// SERIAL ABSTRACTION LAYER
// ========================================
class Serial_HAL {
public:
  // Initialization
  static void init(unsigned long baudRate = 115200);
  
  // Data transmission
  static void print(const char* str);
  static void println(const char* str);
  static void printf(const char* format, ...);
  
  // Data reception
  static bool available();
  static int read();
  static String readLine();
  static String readUntil(char terminator);
  
  // Configuration
  static void setBaudRate(unsigned long baud);
  static void flush();
  
  // Buffer operations
  static int availableForWrite();
};

// ========================================
// I2C ABSTRACTION LAYER
// ========================================
class I2C {
public:
  // Initialization
  static bool init(uint8_t sdaPin, uint8_t sclPin, uint32_t frequency = 100000);
  
  // Device communication
  static bool write(uint8_t address, const uint8_t* data, size_t length);
  static bool read(uint8_t address, uint8_t* buffer, size_t length);
  static bool writeRead(uint8_t address, const uint8_t* writeData, size_t writeLength,
                        uint8_t* readBuffer, size_t readLength);
  
  // Device detection
  static bool devicePresent(uint8_t address);
  static bool scanDevices(uint8_t* addresses, size_t maxDevices, size_t& foundCount);
  
  // Configuration
  static void setClockSpeed(uint32_t frequency);
  
  // Error handling
  static const char* getLastError();
};

// ========================================
// SPI ABSTRACTION LAYER
// ========================================
class SPI_HAL {
public:
  enum SPIBus {
    SPI_BUS_HSPI = 1,   // Hardware SPI (shared)
    SPI_BUS_VSPI = 2,   // Hardware SPI (dedicated)
  };
  
  // Initialization
  static bool init(SPIBus bus, uint32_t frequency = 5000000);
  static bool initWithPins(SPIBus bus, uint8_t sckPin, uint8_t misoPin, 
                           uint8_t mosiPin, uint8_t csPin = -1);
  
  // Data transfer
  static uint8_t transfer(uint8_t data);
  static void transfer(uint8_t* data, size_t length);
  static bool transfer(const uint8_t* writeBuf, uint8_t* readBuf, size_t length);
  
  // Chip select control
  static void selectDevice(uint8_t csPin);
  static void deselectDevice(uint8_t csPin);
  
  // Configuration
  static void setClockSpeed(uint32_t frequency);
  static void setMode(uint8_t mode);  // 0-3 for SPI modes
  
  // Bus management
  static void beginTransaction();
  static void endTransaction();
};

// ========================================
// WATCHDOG ABSTRACTION LAYER
// ========================================
class Watchdog {
public:
  // Initialization
  static bool init(uint32_t timeoutSeconds = 30);
  
  // Watchdog control
  static void feed();
  static void reset();
  static void enable();
  static void disable();
  
  // Configuration
  static void setTimeout(uint32_t seconds);
  
  // Status
  static bool isEnabled();
  static uint32_t getTimeout();
  
  // Recovery info
  static bool wasTriggered();
  static void clearTriggerFlag();
};

// ========================================
// POWER MANAGEMENT ABSTRACTION LAYER
// ========================================
class PowerManager {
public:
  // Power modes
  enum PowerMode {
    NORMAL,         // Full performance
    LIGHT_SLEEP,    // CPU off, peripherals on
    DEEP_SLEEP,     // Deep sleep, only RTC active
    MODEM_SLEEP,    // WiFi off
  };
  
  // Power operations
  static void init();
  static void setPowerMode(PowerMode mode);
  static void sleep(unsigned long durationMs);
  static void deepSleep(unsigned long durationUs);
  
  // Power monitoring
  static uint32_t getChipID();
  static const char* getChipModel();
  static uint32_t getChipCores();
  static uint32_t getCpuFreqMHz();
  
  // Memory monitoring
  static uint32_t getFreeHeap();
  static uint32_t getLargestFreeBlock();
  static uint32_t getHeapFragmentation();
  
  // Thermal monitoring
  static float getChipTemperature();
};

// ========================================
// EEPROM ABSTRACTION LAYER
// ========================================
class EEPROM_HAL {
public:
  // Initialization
  static bool init(size_t sizeBytes);
  
  // Read operations
  static uint8_t read(size_t address);
  static void readBytes(size_t address, uint8_t* buffer, size_t length);
  static uint32_t readUInt32(size_t address);
  
  // Write operations
  static void write(size_t address, uint8_t value);
  static void writeBytes(size_t address, const uint8_t* data, size_t length);
  static void writeUInt32(size_t address, uint32_t value);
  
  // Persistence
  static bool commit();
  static bool clear();
  
  // Status
  static size_t getSize();
};

#endif // HAL_H
