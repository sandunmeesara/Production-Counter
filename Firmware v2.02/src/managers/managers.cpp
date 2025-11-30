#include "managers.h"
#include <Arduino.h>
#include <cstring>

// ========================================
// PRODUCTION MANAGER IMPLEMENTATION
// ========================================

ProductionManager::ProductionManager() {
  sessionActive = false;
  sessionCount = 0;
  totalSessionCount = 0;
  startingCountValue = 0;
}

bool ProductionManager::startSession() {
  if (sessionActive) {
    Serial.println("[ProductionManager] ERROR: Session already active");
    return false;
  }
  
  sessionActive = true;
  sessionCount = 0;
  startingCountValue = 0;
  sessionStartTime = RTC_DS3231::now();  // Will be set by TimeManager
  
  Serial.println("[ProductionManager] Session started");
  Serial.print("  Start time: ");
  Serial.println(sessionStartTime.unixtime());
  
  return true;
}

bool ProductionManager::stopSession() {
  if (!sessionActive) {
    Serial.println("[ProductionManager] WARNING: No active session to stop");
    return false;
  }
  
  sessionActive = false;
  sessionStopTime = RTC_DS3231::now();  // Will be set by TimeManager
  
  Serial.println("[ProductionManager] Session stopped");
  Serial.print("  Stop time: ");
  Serial.println(sessionStopTime.unixtime());
  Serial.print("  Session count: ");
  Serial.println(sessionCount);
  
  // Update total count
  totalSessionCount += sessionCount;
  
  return true;
}

void ProductionManager::incrementCount() {
  if (!sessionActive) {
    return;  // Don't count if not in production
  }
  
  if (sessionCount < 9999) {
    sessionCount++;
  }
  
  // Log every 100 items for diagnostics
  if (sessionCount % 100 == 0) {
    Serial.print("[ProductionManager] Count: ");
    Serial.println(sessionCount);
  }
}

int ProductionManager::getSessionCount() const {
  return sessionCount;
}

unsigned long ProductionManager::getSessionDuration() const {
  if (!sessionActive) {
    // Calculate from start to stop time
    if (sessionStopTime.unixtime() >= sessionStartTime.unixtime()) {
      return sessionStopTime.unixtime() - sessionStartTime.unixtime();
    }
    return 0;
  }
  
  // Session still active - calculate current duration
  DateTime now = RTC_DS3231::now();
  if (now.unixtime() >= sessionStartTime.unixtime()) {
    return now.unixtime() - sessionStartTime.unixtime();
  }
  return 0;
}

bool ProductionManager::saveSessionToFile() {
  // File format: Production_YYYY-MM-DD_HHhMMm-HHhMMm.txt
  // This will be implemented with StorageManager
  
  Serial.print("[ProductionManager] Would save session to file");
  Serial.print(" | Start: ");
  Serial.print(sessionStartTime.hour());
  Serial.print("h");
  Serial.print(sessionStartTime.minute());
  Serial.print("m");
  Serial.print(" | Stop: ");
  Serial.print(sessionStopTime.hour());
  Serial.print("h");
  Serial.print(sessionStopTime.minute());
  Serial.print("m");
  Serial.print(" | Count: ");
  Serial.println(sessionCount);
  
  return true;
}

bool ProductionManager::loadSessionFromFile() {
  // Load from /prod_session.txt (recovery file)
  Serial.println("[ProductionManager] Loading session from recovery file");
  return true;
}

bool ProductionManager::clearSessionFile() {
  // Delete /prod_session.txt
  Serial.println("[ProductionManager] Clearing recovery file");
  return true;
}

bool ProductionManager::isRecoveryValid() const {
  // Check if recovery file exists and is valid
  return true;
}

bool ProductionManager::recover() {
  Serial.println("[ProductionManager] Attempting recovery from power loss");
  return loadSessionFromFile();
}

// ========================================
// TIME MANAGER IMPLEMENTATION
// ========================================

TimeManager::TimeManager() {
  lastTrackedHour = -1;
  timeInitialized = false;
}

bool TimeManager::initialize() {
  Serial.println("[TimeManager] Initializing RTC...");
  
  // This would check if RTC is available
  // For now, just mark as initialized
  timeInitialized = true;
  
  DateTime now = getCurrentTime();
  lastTrackedHour = now.hour();
  lastRecordedTime = now;
  
  Serial.print("[TimeManager] RTC initialized");
  Serial.print(" | Current time: ");
  Serial.print(now.year());
  Serial.print("-");
  if (now.month() < 10) Serial.print("0");
  Serial.print(now.month());
  Serial.print("-");
  if (now.day() < 10) Serial.print("0");
  Serial.print(now.day());
  Serial.print(" ");
  if (now.hour() < 10) Serial.print("0");
  Serial.print(now.hour());
  Serial.print(":");
  if (now.minute() < 10) Serial.print("0");
  Serial.print(now.minute());
  Serial.print(":");
  if (now.second() < 10) Serial.print("0");
  Serial.println(now.second());
  
  return true;
}

DateTime TimeManager::getCurrentTime() const {
  // In real implementation, read from RTC_DS3231
  // For now, return dummy time
  return DateTime(F(__DATE__), F(__TIME__));
}

bool TimeManager::setTime(DateTime newTime) {
  Serial.print("[TimeManager] Setting time to: ");
  Serial.print(newTime.year());
  Serial.print("-");
  if (newTime.month() < 10) Serial.print("0");
  Serial.print(newTime.month());
  Serial.print("-");
  if (newTime.day() < 10) Serial.print("0");
  Serial.print(newTime.day());
  Serial.print(" ");
  if (newTime.hour() < 10) Serial.print("0");
  Serial.print(newTime.hour());
  Serial.print(":");
  if (newTime.minute() < 10) Serial.print("0");
  Serial.print(newTime.minute());
  Serial.print(":");
  if (newTime.second() < 10) Serial.print("0");
  Serial.println(newTime.second());
  
  // Update RTC (would call rtc.adjust(newTime) in real code)
  lastRecordedTime = newTime;
  
  return true;
}

bool TimeManager::hasHourChanged() const {
  DateTime now = getCurrentTime();
  return now.hour() != lastTrackedHour;
}

int TimeManager::getCurrentHour() const {
  return getCurrentTime().hour();
}

void TimeManager::handleHourChange() {
  DateTime now = getCurrentTime();
  int newHour = now.hour();
  
  if (newHour != lastTrackedHour) {
    Serial.print("[TimeManager] Hour changed: ");
    Serial.print(lastTrackedHour);
    Serial.print(" → ");
    Serial.println(newHour);
    
    lastTrackedHour = newHour;
    lastRecordedTime = now;
  }
}

bool TimeManager::isTimeValid() const {
  DateTime now = getCurrentTime();
  
  // Check if time is reasonable
  // Year should be >= 2020
  // Month should be 1-12
  // Day should be 1-31
  // Hour should be 0-23
  // Minute should be 0-59
  // Second should be 0-59
  
  return (now.year() >= 2020 && 
          now.month() >= 1 && now.month() <= 12 &&
          now.day() >= 1 && now.day() <= 31 &&
          now.hour() >= 0 && now.hour() < 24 &&
          now.minute() >= 0 && now.minute() < 60 &&
          now.second() >= 0 && now.second() < 60);
}

bool TimeManager::isTimeInitialized() const {
  return timeInitialized;
}

const char* TimeManager::getTimeString(bool includeSeconds) const {
  static char buffer[25];
  DateTime now = getCurrentTime();
  
  if (includeSeconds) {
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
  } else {
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute());
  }
  
  return buffer;
}

// ========================================
// STORAGE MANAGER IMPLEMENTATION
// ========================================

StorageManager::StorageManager() {
  sdAvailable = false;
}

bool StorageManager::initialize() {
  Serial.println("[StorageManager] Initializing SD card...");
  
  // This would initialize the SD card
  // For now, just mark as available
  sdAvailable = true;
  
  Serial.println("[StorageManager] SD card initialized");
  return true;
}

bool StorageManager::writeFile(const char* filename, const char* data) {
  if (!sdAvailable) {
    Serial.println("[StorageManager] ERROR: SD card not available");
    return false;
  }
  
  Serial.print("[StorageManager] Writing to ");
  Serial.print(filename);
  Serial.print(" | Data: ");
  Serial.println(data);
  
  // Would write to file using SD library
  return true;
}

bool StorageManager::readFile(const char* filename, char* buffer, size_t maxSize) {
  if (!sdAvailable) {
    Serial.println("[StorageManager] ERROR: SD card not available");
    return false;
  }
  
  Serial.print("[StorageManager] Reading from ");
  Serial.println(filename);
  
  // Would read from file using SD library
  buffer[0] = '\0';  // Empty buffer for now
  return true;
}

bool StorageManager::fileExists(const char* filename) const {
  if (!sdAvailable) {
    return false;
  }
  
  // Would check if file exists
  Serial.print("[StorageManager] Checking if ");
  Serial.print(filename);
  Serial.println(" exists...");
  
  return true;
}

bool StorageManager::deleteFile(const char* filename) {
  if (!sdAvailable) {
    Serial.println("[StorageManager] ERROR: SD card not available");
    return false;
  }
  
  Serial.print("[StorageManager] Deleting ");
  Serial.println(filename);
  
  // Would delete file using SD library
  return true;
}

bool StorageManager::saveCount(const char* filename, int value) {
  if (!sdAvailable) {
    Serial.println("[StorageManager] ERROR: SD card not available");
    return false;
  }
  
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%d", value);
  
  Serial.print("[StorageManager] Saving count to ");
  Serial.print(filename);
  Serial.print(" | Value: ");
  Serial.println(value);
  
  return writeFile(filename, buffer);
}

int StorageManager::loadCount(const char* filename) const {
  if (!sdAvailable) {
    Serial.println("[StorageManager] ERROR: SD card not available");
    return 0;
  }
  
  Serial.print("[StorageManager] Loading count from ");
  Serial.println(filename);
  
  // Would read and parse integer from file
  return 0;
}

bool StorageManager::saveProductionSession(const char* filename,
                                          DateTime start, DateTime end, int count) {
  if (!sdAvailable) {
    Serial.println("[StorageManager] ERROR: SD card not available");
    return false;
  }
  
  Serial.print("[StorageManager] Saving production session to ");
  Serial.print(filename);
  Serial.print(" | Count: ");
  Serial.println(count);
  
  // Would format and write session data
  return true;
}

bool StorageManager::saveDailyLog(const char* filename, const char* data) {
  return writeFile(filename, data);
}

bool StorageManager::listFiles() {
  if (!sdAvailable) {
    Serial.println("[StorageManager] ERROR: SD card not available");
    return false;
  }
  
  Serial.println("[StorageManager] Listing files...");
  
  // Would enumerate files on SD card
  return true;
}

bool StorageManager::searchFiles(const char* pattern) {
  if (!sdAvailable) {
    Serial.println("[StorageManager] ERROR: SD card not available");
    return false;
  }
  
  Serial.print("[StorageManager] Searching for files matching: ");
  Serial.println(pattern);
  
  // Would search for matching files
  return true;
}

int StorageManager::countFiles() const {
  if (!sdAvailable) {
    return 0;
  }
  
  Serial.println("[StorageManager] Counting files...");
  
  // Would count files on SD card
  return 0;
}

bool StorageManager::formatSD() {
  if (!sdAvailable) {
    Serial.println("[StorageManager] ERROR: SD card not available");
    return false;
  }
  
  Serial.println("[StorageManager] WARNING: Formatting SD card (destructive operation)");
  
  // Would format SD card
  return true;
}

// ========================================
// DISPLAY MANAGER IMPLEMENTATION
// ========================================

DisplayManager::DisplayManager() {
  displayDirty = true;
  lastRefresh = 0;
  refreshRate = 100;  // 100ms default
}

bool DisplayManager::initialize() {
  Serial.println("[DisplayManager] Initializing OLED display...");
  
  // Would initialize display
  displayDirty = true;
  
  Serial.println("[DisplayManager] OLED display initialized");
  return true;
}

void DisplayManager::update() {
  unsigned long now = millis();
  
  // Check if refresh is needed
  if (!needsRefresh()) {
    return;
  }
  
  // Would update OLED display
  lastRefresh = now;
  displayDirty = false;
}

void DisplayManager::clear() {
  Serial.println("[DisplayManager] Clearing display");
  displayDirty = true;
}

void DisplayManager::showMainScreen(int count, DateTime time, bool isProducing) {
  Serial.print("[DisplayManager] Showing main screen | Count: ");
  Serial.print(count);
  Serial.print(" | Producing: ");
  Serial.println(isProducing ? "YES" : "NO");
  
  displayDirty = true;
}

void DisplayManager::showStatus(const char* message, unsigned long duration) {
  Serial.print("[DisplayManager] Status: ");
  Serial.print(message);
  Serial.print(" (for ");
  Serial.print(duration);
  Serial.println("ms)");
  
  displayDirty = true;
}

void DisplayManager::showError(const char* errorMessage) {
  Serial.print("[DisplayManager] ERROR: ");
  Serial.println(errorMessage);
  
  displayDirty = true;
}

void DisplayManager::showDiagnostics(const char* results) {
  Serial.print("[DisplayManager] Diagnostics: ");
  Serial.println(results);
  
  displayDirty = true;
}

void DisplayManager::showInitializationScreen() {
  Serial.println("[DisplayManager] Showing initialization screen");
  displayDirty = true;
}

void DisplayManager::showReadyScreen() {
  Serial.println("[DisplayManager] Showing ready screen");
  displayDirty = true;
}

void DisplayManager::showProductionScreen(int count) {
  Serial.print("[DisplayManager] Showing production screen | Count: ");
  Serial.println(count);
  
  displayDirty = true;
}

void DisplayManager::showDiagnosticScreen() {
  Serial.println("[DisplayManager] Showing diagnostic screen");
  displayDirty = true;
}

void DisplayManager::showErrorScreen(const char* message) {
  Serial.print("[DisplayManager] Showing error screen: ");
  Serial.println(message);
  
  displayDirty = true;
}

void DisplayManager::displayText(int x, int y, const char* text, int textSize) {
  Serial.print("[DisplayManager] Text at (");
  Serial.print(x);
  Serial.print(",");
  Serial.print(y);
  Serial.print") size ");
  Serial.print(textSize);
  Serial.print(": ");
  Serial.println(text);
}

void DisplayManager::displayNumber(int x, int y, int value, int textSize) {
  Serial.print("[DisplayManager] Number at (");
  Serial.print(x);
  Serial.print(",");
  Serial.print(y);
  Serial.print") size ");
  Serial.print(textSize);
  Serial.print(": ");
  Serial.println(value);
}

void DisplayManager::displayCentered(int y, const char* text, int textSize) {
  Serial.print("[DisplayManager] Centered at Y=");
  Serial.print(y);
  Serial.print(" size ");
  Serial.print(textSize);
  Serial.print(": ");
  Serial.println(text);
}

void DisplayManager::displayLine(int y) {
  Serial.print("[DisplayManager] Line at Y=");
  Serial.println(y);
}

void DisplayManager::setBrightness(uint8_t level) {
  Serial.print("[DisplayManager] Setting brightness to ");
  Serial.println(level);
}

void DisplayManager::setRefreshRate(unsigned long rateMs) {
  refreshRate = rateMs;
  Serial.print("[DisplayManager] Setting refresh rate to ");
  Serial.print(rateMs);
  Serial.println("ms");
}

bool DisplayManager::needsRefresh() const {
  return (millis() - lastRefresh > refreshRate) || displayDirty;
}

void DisplayManager::markDirty() {
  displayDirty = true;
}

void DisplayManager::markClean() {
  displayDirty = false;
}

void DisplayManager::drawProgressBar(int y, int value, int maxValue) {
  Serial.print("[DisplayManager] Progress bar at Y=");
  Serial.print(y);
  Serial.print(" | ");
  Serial.print(value);
  Serial.print("/");
  Serial.println(maxValue);
}

// ========================================
// LOGGER MANAGER IMPLEMENTATION
// ========================================

LoggerManager::LogLevel LoggerManager::currentLevel = LoggerManager::INFO;
bool LoggerManager::fileLoggingEnabled = false;

void LoggerManager::initialize(LogLevel level) {
  currentLevel = level;
  Serial.println("[LoggerManager] Logger initialized");
}

void LoggerManager::debug(const char* format, ...) {
  if (currentLevel > DEBUG) return;
  
  Serial.print("[DEBUG] ");
  // In real implementation, would format with va_args
  Serial.println(format);
}

void LoggerManager::info(const char* format, ...) {
  if (currentLevel > INFO) return;
  
  Serial.print("[INFO] ");
  Serial.println(format);
}

void LoggerManager::warn(const char* format, ...) {
  if (currentLevel > WARN) return;
  
  Serial.print("[WARN] ");
  Serial.println(format);
}

void LoggerManager::error(const char* format, ...) {
  if (currentLevel > ERROR) return;
  
  Serial.print("[ERROR] ");
  Serial.println(format);
}

void LoggerManager::fatal(const char* format, ...) {
  Serial.print("[FATAL] ");
  Serial.println(format);
}

void LoggerManager::logToFile(const char* filename, const char* message) {
  if (!fileLoggingEnabled) {
    return;
  }
  
  Serial.print("[LoggerManager] Logging to ");
  Serial.print(filename);
  Serial.print(": ");
  Serial.println(message);
}

void LoggerManager::setLogLevel(LogLevel level) {
  currentLevel = level;
  Serial.print("[LoggerManager] Log level set to ");
  Serial.println(logLevelName(level));
}

void LoggerManager::enableFileLogging(bool enable) {
  fileLoggingEnabled = enable;
  Serial.print("[LoggerManager] File logging ");
  Serial.println(enable ? "enabled" : "disabled");
}

const char* LoggerManager::logLevelName(LogLevel level) {
  switch (level) {
    case DEBUG: return "DEBUG";
    case INFO: return "INFO";
    case WARN: return "WARN";
    case ERROR: return "ERROR";
    case FATAL: return "FATAL";
  }
  return "UNKNOWN";
}

// ========================================
// CONFIG MANAGER IMPLEMENTATION
// ========================================

ConfigManager::ConfigManager() {
  settings.saveInterval = 5000;
  settings.debounceDelay = 50;
  settings.maxCount = 9999;
  settings.statusDisplayDuration = 3000;
}

bool ConfigManager::initialize() {
  Serial.println("[ConfigManager] Loading configuration...");
  
  return loadFromEEPROM();
}

bool ConfigManager::setSettings(const Settings& newSettings) {
  if (!validateSettings()) {
    Serial.println("[ConfigManager] ERROR: Invalid settings");
    return false;
  }
  
  settings = newSettings;
  Serial.println("[ConfigManager] Settings updated");
  
  return saveToEEPROM();
}

void ConfigManager::setSaveInterval(unsigned long interval) {
  if (interval >= 1000 && interval <= 60000) {
    settings.saveInterval = interval;
    Serial.print("[ConfigManager] Save interval set to ");
    Serial.println(interval);
  }
}

void ConfigManager::setDebounceDelay(unsigned long delay) {
  if (delay >= 10 && delay <= 500) {
    settings.debounceDelay = delay;
    Serial.print("[ConfigManager] Debounce delay set to ");
    Serial.println(delay);
  }
}

void ConfigManager::setMaxCount(int maxCount) {
  if (maxCount >= 100 && maxCount <= 99999) {
    settings.maxCount = maxCount;
    Serial.print("[ConfigManager] Max count set to ");
    Serial.println(maxCount);
  }
}

void ConfigManager::setStatusDisplayDuration(unsigned long duration) {
  if (duration >= 1000 && duration <= 10000) {
    settings.statusDisplayDuration = duration;
    Serial.print("[ConfigManager] Status display duration set to ");
    Serial.println(duration);
  }
}

bool ConfigManager::loadFromEEPROM() {
  Serial.println("[ConfigManager] Loading settings from EEPROM");
  
  // Would read from EEPROM
  // For now, use defaults
  
  return true;
}

bool ConfigManager::saveToEEPROM() {
  Serial.println("[ConfigManager] Saving settings to EEPROM");
  
  // Would write to EEPROM
  
  return true;
}

void ConfigManager::resetToDefaults() {
  Serial.println("[ConfigManager] Resetting to default settings");
  
  settings.saveInterval = 5000;
  settings.debounceDelay = 50;
  settings.maxCount = 9999;
  settings.statusDisplayDuration = 3000;
  
  saveToEEPROM();
}

bool ConfigManager::validateSettings() const {
  return settings.saveInterval >= 1000 && settings.saveInterval <= 60000 &&
         settings.debounceDelay >= 10 && settings.debounceDelay <= 500 &&
         settings.maxCount >= 100 && settings.maxCount <= 99999 &&
         settings.statusDisplayDuration >= 1000 && settings.statusDisplayDuration <= 10000;
}

bool ConfigManager::isValid() const {
  return validateSettings();
}
