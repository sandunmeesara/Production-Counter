#include "hal.h"
#include <Arduino.h>

// ========================================
// GPIO IMPLEMENTATION
// ========================================

void GPIO::init(Pin pin, Mode mode) {
  uint8_t pinMode = INPUT;
  
  switch (mode) {
    case INPUT_PULLUP:
      pinMode = INPUT_PULLUP;
      break;
    case OUTPUT:
      pinMode = OUTPUT;
      break;
    case INPUT:
      pinMode = INPUT;
      break;
  }
  
  pinMode((int)pin, pinMode);
  
  Serial.print("[GPIO] Initialized pin ");
  Serial.print((int)pin);
  Serial.print(" as ");
  Serial.println(getPinName(pin));
}

bool GPIO::read(Pin pin) {
  return digitalRead((int)pin) == HIGH;
}

GPIO::PinState GPIO::readState(Pin pin) {
  return digitalRead((int)pin) == HIGH ? HIGH : LOW;
}

void GPIO::write(Pin pin, bool value) {
  digitalWrite((int)pin, value ? HIGH : LOW);
}

void GPIO::setState(Pin pin, PinState state) {
  digitalWrite((int)pin, state == HIGH ? HIGH : LOW);
}

void GPIO::attachInterrupt(Pin pin, ISRCallback handler, int mode) {
  Serial.print("[GPIO] Attaching interrupt to pin ");
  Serial.println((int)pin);
  
  // Would call attachInterrupt() on real hardware
}

void GPIO::detachInterrupt(Pin pin) {
  Serial.print("[GPIO] Detaching interrupt from pin ");
  Serial.println((int)pin);
  
  // Would call detachInterrupt() on real hardware
}

void GPIO::initAll() {
  Serial.println("[GPIO] Initializing all pins...");
  
  init(COUNTER_BTN, INPUT_PULLUP);
  init(DIAG_BTN, INPUT_PULLUP);
  init(LATCH_BTN, INPUT_PULLUP);
  init(SD_CS, OUTPUT);
  
  Serial.println("[GPIO] All pins initialized");
}

const char* GPIO::getPinName(Pin pin) {
  switch (pin) {
    case COUNTER_BTN: return "COUNTER_BTN (GPIO 15)";
    case DIAG_BTN: return "DIAG_BTN (GPIO 27)";
    case LATCH_BTN: return "LATCH_BTN (GPIO 25)";
    case SD_CS: return "SD_CS (GPIO 26)";
  }
  return "UNKNOWN";
}

// ========================================
// TIMER IMPLEMENTATION
// ========================================

bool Timer::createTimer(uint8_t timerId, unsigned long intervalMs,
                        TimerCallback callback, TimerType type) {
  Serial.print("[Timer] Creating timer ");
  Serial.print(timerId);
  Serial.print(" with interval ");
  Serial.print(intervalMs);
  Serial.println("ms");
  
  // Would create timer on real hardware
  return true;
}

void Timer::startTimer(uint8_t timerId) {
  Serial.print("[Timer] Starting timer ");
  Serial.println(timerId);
  
  // Would start timer on real hardware
}

void Timer::stopTimer(uint8_t timerId) {
  Serial.print("[Timer] Stopping timer ");
  Serial.println(timerId);
  
  // Would stop timer on real hardware
}

void Timer::deleteTimer(uint8_t timerId) {
  Serial.print("[Timer] Deleting timer ");
  Serial.println(timerId);
  
  // Would delete timer on real hardware
}

bool Timer::isRunning(uint8_t timerId) {
  // Would check if timer is running
  return true;
}

unsigned long Timer::getElapsed(uint8_t timerId) {
  // Would return elapsed time for timer
  return 0;
}

void Timer::setInterval(uint8_t timerId, unsigned long intervalMs) {
  Serial.print("[Timer] Setting interval for timer ");
  Serial.print(timerId);
  Serial.print(" to ");
  Serial.print(intervalMs);
  Serial.println("ms");
}

void Timer::delay(unsigned long ms) {
  ::delay(ms);
}

void Timer::delayMicroseconds(unsigned int us) {
  ::delayMicroseconds(us);
}

unsigned long Timer::millis() {
  return ::millis();
}

unsigned long Timer::micros() {
  return ::micros();
}

// ========================================
// SERIAL_HAL IMPLEMENTATION
// ========================================

void Serial_HAL::init(unsigned long baudRate) {
  Serial.begin(baudRate);
  delay(1000);
  
  Serial.print("[Serial_HAL] Initialized at ");
  Serial.print(baudRate);
  Serial.println(" baud");
}

void Serial_HAL::print(const char* str) {
  Serial.print(str);
}

void Serial_HAL::println(const char* str) {
  Serial.println(str);
}

void Serial_HAL::printf(const char* format, ...) {
  // Would format and print like sprintf
  Serial.print(format);
}

bool Serial_HAL::available() {
  return Serial.available() > 0;
}

int Serial_HAL::read() {
  return Serial.read();
}

String Serial_HAL::readLine() {
  String result = "";
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      break;
    }
    result += c;
  }
  return result;
}

String Serial_HAL::readUntil(char terminator) {
  String result = "";
  while (Serial.available()) {
    char c = Serial.read();
    if (c == terminator) {
      break;
    }
    result += c;
  }
  return result;
}

void Serial_HAL::setBaudRate(unsigned long baud) {
  Serial.print("[Serial_HAL] Setting baud rate to ");
  Serial.println(baud);
  
  // Would set baud rate on real hardware
}

void Serial_HAL::flush() {
  Serial.flush();
}

int Serial_HAL::availableForWrite() {
  return 256;  // Typical buffer size
}

// ========================================
// I2C IMPLEMENTATION
// ========================================

bool I2C::init(uint8_t sdaPin, uint8_t sclPin, uint32_t frequency) {
  Serial.print("[I2C] Initializing I2C on SDA=");
  Serial.print(sdaPin);
  Serial.print(" SCL=");
  Serial.print(sclPin);
  Serial.print(" @ ");
  Serial.print(frequency);
  Serial.println("Hz");
  
  // Wire.begin(sdaPin, sclPin);
  
  return true;
}

bool I2C::write(uint8_t address, const uint8_t* data, size_t length) {
  Serial.print("[I2C] Writing ");
  Serial.print(length);
  Serial.print(" bytes to address 0x");
  Serial.println(address, HEX);
  
  return true;
}

bool I2C::read(uint8_t address, uint8_t* buffer, size_t length) {
  Serial.print("[I2C] Reading ");
  Serial.print(length);
  Serial.print(" bytes from address 0x");
  Serial.println(address, HEX);
  
  return true;
}

bool I2C::writeRead(uint8_t address, const uint8_t* writeData, size_t writeLength,
                    uint8_t* readBuffer, size_t readLength) {
  Serial.print("[I2C] Write-Read to address 0x");
  Serial.println(address, HEX);
  
  return true;
}

bool I2C::devicePresent(uint8_t address) {
  Serial.print("[I2C] Checking device at address 0x");
  Serial.println(address, HEX);
  
  return true;
}

bool I2C::scanDevices(uint8_t* addresses, size_t maxDevices, size_t& foundCount) {
  Serial.println("[I2C] Scanning for devices...");
  foundCount = 0;
  
  return true;
}

void I2C::setClockSpeed(uint32_t frequency) {
  Serial.print("[I2C] Setting clock speed to ");
  Serial.print(frequency);
  Serial.println("Hz");
}

const char* I2C::getLastError() {
  return "No error";
}

// ========================================
// SPI_HAL IMPLEMENTATION
// ========================================

bool SPI_HAL::init(SPIBus bus, uint32_t frequency) {
  Serial.print("[SPI_HAL] Initializing SPI bus ");
  Serial.print(bus);
  Serial.print(" @ ");
  Serial.print(frequency);
  Serial.println("Hz");
  
  return true;
}

bool SPI_HAL::initWithPins(SPIBus bus, uint8_t sckPin, uint8_t misoPin,
                           uint8_t mosiPin, uint8_t csPin) {
  Serial.print("[SPI_HAL] Initializing SPI bus ");
  Serial.print(bus);
  Serial.print(" with custom pins - SCK:");
  Serial.print(sckPin);
  Serial.print(" MISO:");
  Serial.print(misoPin);
  Serial.print(" MOSI:");
  Serial.print(mosiPin);
  Serial.print(" CS:");
  Serial.println(csPin);
  
  return true;
}

uint8_t SPI_HAL::transfer(uint8_t data) {
  // Would transfer single byte over SPI
  return 0;
}

void SPI_HAL::transfer(uint8_t* data, size_t length) {
  Serial.print("[SPI_HAL] Transferring ");
  Serial.print(length);
  Serial.println(" bytes");
}

bool SPI_HAL::transfer(const uint8_t* writeBuf, uint8_t* readBuf, size_t length) {
  Serial.print("[SPI_HAL] Bidirectional transfer of ");
  Serial.print(length);
  Serial.println(" bytes");
  
  return true;
}

void SPI_HAL::selectDevice(uint8_t csPin) {
  digitalWrite(csPin, LOW);
}

void SPI_HAL::deselectDevice(uint8_t csPin) {
  digitalWrite(csPin, HIGH);
}

void SPI_HAL::setClockSpeed(uint32_t frequency) {
  Serial.print("[SPI_HAL] Setting clock speed to ");
  Serial.print(frequency);
  Serial.println("Hz");
}

void SPI_HAL::setMode(uint8_t mode) {
  Serial.print("[SPI_HAL] Setting SPI mode to ");
  Serial.println(mode);
}

void SPI_HAL::beginTransaction() {
  // Would protect SPI transaction
}

void SPI_HAL::endTransaction() {
  // Would end SPI transaction
}

// ========================================
// WATCHDOG IMPLEMENTATION
// ========================================

bool Watchdog::init(uint32_t timeoutSeconds) {
  Serial.print("[Watchdog] Initializing with ");
  Serial.print(timeoutSeconds);
  Serial.println(" second timeout");
  
  // esp_task_wdt_init(timeoutSeconds, true);
  
  return true;
}

void Watchdog::feed() {
  // esp_task_wdt_reset();
}

void Watchdog::reset() {
  Serial.println("[Watchdog] Resetting system...");
  
  // ESP.restart();
}

void Watchdog::enable() {
  Serial.println("[Watchdog] Enabling watchdog");
}

void Watchdog::disable() {
  Serial.println("[Watchdog] Disabling watchdog");
}

void Watchdog::setTimeout(uint32_t seconds) {
  Serial.print("[Watchdog] Setting timeout to ");
  Serial.print(seconds);
  Serial.println(" seconds");
}

bool Watchdog::isEnabled() {
  return true;
}

uint32_t Watchdog::getTimeout() {
  return 30;  // Default timeout
}

bool Watchdog::wasTriggered() {
  return false;
}

void Watchdog::clearTriggerFlag() {
  Serial.println("[Watchdog] Clearing trigger flag");
}

// ========================================
// POWER MANAGER IMPLEMENTATION
// ========================================

void PowerManager::init() {
  Serial.println("[PowerManager] Initializing power management...");
}

void PowerManager::setPowerMode(PowerMode mode) {
  const char* modeNames[] = {"NORMAL", "LIGHT_SLEEP", "DEEP_SLEEP", "MODEM_SLEEP"};
  Serial.print("[PowerManager] Setting power mode to ");
  Serial.println(modeNames[mode]);
}

void PowerManager::sleep(unsigned long durationMs) {
  Serial.print("[PowerManager] Sleeping for ");
  Serial.print(durationMs);
  Serial.println("ms");
}

void PowerManager::deepSleep(unsigned long durationUs) {
  Serial.print("[PowerManager] Deep sleeping for ");
  Serial.print(durationUs);
  Serial.println("µs");
}

uint32_t PowerManager::getChipID() {
  return 0xDEADBEEF;  // Dummy ID
}

const char* PowerManager::getChipModel() {
  return "ESP32";
}

uint32_t PowerManager::getChipCores() {
  return 2;
}

uint32_t PowerManager::getCpuFreqMHz() {
  return 240;
}

uint32_t PowerManager::getFreeHeap() {
  return ESP.getFreeHeap();
}

uint32_t PowerManager::getLargestFreeBlock() {
  return ESP.getMaxAllocHeap();
}

uint32_t PowerManager::getHeapFragmentation() {
  uint32_t free = ESP.getFreeHeap();
  uint32_t max = ESP.getMaxAllocHeap();
  
  if (max == 0) return 0;
  return 100 - ((max * 100) / free);
}

float PowerManager::getChipTemperature() {
  // temperatureRead() returns Celsius (0-127)
  return 25.0f;  // Dummy value
}

// ========================================
// EEPROM_HAL IMPLEMENTATION
// ========================================

bool EEPROM_HAL::init(size_t sizeBytes) {
  Serial.print("[EEPROM_HAL] Initializing EEPROM (");
  Serial.print(sizeBytes);
  Serial.println(" bytes)");
  
  // EEPROM.begin(sizeBytes);
  
  return true;
}

uint8_t EEPROM_HAL::read(size_t address) {
  // return EEPROM.read(address);
  return 0;
}

void EEPROM_HAL::readBytes(size_t address, uint8_t* buffer, size_t length) {
  for (size_t i = 0; i < length; i++) {
    buffer[i] = 0;  // read(address + i);
  }
}

uint32_t EEPROM_HAL::readUInt32(size_t address) {
  // return EEPROM.readUInt(address);
  return 0;
}

void EEPROM_HAL::write(size_t address, uint8_t value) {
  // EEPROM.write(address, value);
}

void EEPROM_HAL::writeBytes(size_t address, const uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    // write(address + i, data[i]);
  }
}

void EEPROM_HAL::writeUInt32(size_t address, uint32_t value) {
  // EEPROM.writeUInt(address, value);
}

bool EEPROM_HAL::commit() {
  Serial.println("[EEPROM_HAL] Committing changes...");
  
  // return EEPROM.commit();
  return true;
}

bool EEPROM_HAL::clear() {
  Serial.println("[EEPROM_HAL] Clearing EEPROM...");
  
  // Not typically done
  return true;
}

size_t EEPROM_HAL::getSize() {
  return 512;  // Default size
}
