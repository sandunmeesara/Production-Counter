#ifndef MANAGERS_H
#define MANAGERS_H

#include <Arduino.h>
#include <RTClib.h>

// ========================================
// PRODUCTION MANAGER
// ========================================
class ProductionManager {
public:
  // Session control
  bool startSession();
  bool stopSession();
  bool isSessionActive() const { return sessionActive; }
  
  // Counting
  void incrementCount();
  int getSessionCount() const;
  int getTotalSessionCount() const { return totalSessionCount; }
  
  // Session info
  DateTime getStartTime() const { return sessionStartTime; }
  DateTime getStopTime() const { return sessionStopTime; }
  unsigned long getSessionDuration() const;
  
  // File management
  bool saveSessionToFile();
  bool loadSessionFromFile();
  bool clearSessionFile();
  
  // Recovery
  bool isRecoveryValid() const;
  bool recover();
  
private:
  bool sessionActive = false;
  volatile int sessionCount = 0;
  int totalSessionCount = 0;
  
  DateTime sessionStartTime;
  DateTime sessionStopTime;
  
  int startingCountValue = 0;
};

// ========================================
// TIME MANAGER
// ========================================
class TimeManager {
public:
  // Initialization
  bool initialize();
  
  // Time operations
  DateTime getCurrentTime() const;
  bool setTime(DateTime newTime);
  
  // Hour tracking
  bool hasHourChanged() const;
  int getCurrentHour() const;
  int getLastHour() const { return lastTrackedHour; }
  
  // Hour change handler
  void handleHourChange();
  
  // Time validation
  bool isTimeValid() const;
  bool isTimeInitialized() const;
  
  // Diagnostics
  const char* getTimeString(bool includeSeconds = true) const;
  
private:
  int lastTrackedHour = -1;
  DateTime lastRecordedTime;
  bool timeInitialized = false;
};

// ========================================
// STORAGE MANAGER
// ========================================
class StorageManager {
public:
  // Initialization
  bool initialize();
  bool isAvailable() const { return sdAvailable; }
  
  // File operations
  bool writeFile(const char* filename, const char* data);
  bool readFile(const char* filename, char* buffer, size_t maxSize);
  bool fileExists(const char* filename) const;
  bool deleteFile(const char* filename);
  
  // Count file operations
  bool saveCount(const char* filename, int value);
  int loadCount(const char* filename) const;
  
  // Production file operations
  bool saveProductionSession(const char* filename, 
                             DateTime start, DateTime end, int count);
  bool saveDailyLog(const char* filename, const char* data);
  
  // Directory operations
  bool listFiles();
  bool searchFiles(const char* pattern);
  int countFiles() const;
  
  // Cleanup
  bool formatSD();
  
private:
  bool sdAvailable = false;
};

// ========================================
// DISPLAY MANAGER
// ========================================
class DisplayManager {
public:
  // Initialization
  bool initialize();
  
  // Display updates
  void update();
  void clear();
  
  // Content updates
  void showMainScreen(int count, DateTime time, bool isProducing);
  void showStatus(const char* message, unsigned long duration);
  void showError(const char* errorMessage);
  void showDiagnostics(const char* results);
  
  // State-specific displays
  void showInitializationScreen();
  void showReadyScreen();
  void showProductionScreen(int count);
  void showDiagnosticScreen();
  void showErrorScreen(const char* message);
  
  // Low-level operations
  void displayText(int x, int y, const char* text, int textSize = 1);
  void displayNumber(int x, int y, int value, int textSize = 1);
  void displayCentered(int y, const char* text, int textSize = 1);
  void displayLine(int y);
  
  // Configuration
  void setBrightness(uint8_t level);
  void setRefreshRate(unsigned long rateMs);
  
  // Refresh control
  bool needsRefresh() const;
  void markDirty();
  void markClean();
  
private:
  unsigned long lastRefresh = 0;
  unsigned long refreshRate = 100;  // ms
  bool displayDirty = true;
  
  void drawProgressBar(int y, int value, int maxValue);
};

// ========================================
// LOGGER MANAGER
// ========================================
class LoggerManager {
public:
  // Log levels
  enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
  };
  
  // Initialization
  static void initialize(LogLevel level = INFO);
  
  // Logging functions
  static void debug(const char* format, ...);
  static void info(const char* format, ...);
  static void warn(const char* format, ...);
  static void error(const char* format, ...);
  static void fatal(const char* format, ...);
  
  // File logging
  static void logToFile(const char* filename, const char* message);
  
  // Settings
  static void setLogLevel(LogLevel level);
  static void enableFileLogging(bool enable);
  
private:
  static LogLevel currentLevel;
  static bool fileLoggingEnabled;
  static const char* logLevelName(LogLevel level);
};

// ========================================
// CONFIGURATION MANAGER
// ========================================
class ConfigManager {
public:
  // Settings structure
  struct Settings {
    unsigned long saveInterval = 5000;
    unsigned long debounceDelay = 50;
    int maxCount = 9999;
    unsigned long statusDisplayDuration = 3000;
  };
  
  // Initialization
  bool initialize();
  
  // Settings access
  Settings getSettings() const { return settings; }
  bool setSettings(const Settings& newSettings);
  
  // Individual parameter access
  unsigned long getSaveInterval() const { return settings.saveInterval; }
  unsigned long getDebounceDelay() const { return settings.debounceDelay; }
  int getMaxCount() const { return settings.maxCount; }
  unsigned long getStatusDisplayDuration() const { return settings.statusDisplayDuration; }
  
  // Setters
  void setSaveInterval(unsigned long interval);
  void setDebounceDelay(unsigned long delay);
  void setMaxCount(int maxCount);
  void setStatusDisplayDuration(unsigned long duration);
  
  // Persistence
  bool loadFromEEPROM();
  bool saveToEEPROM();
  void resetToDefaults();
  
  // Validation
  bool validateSettings() const;
  
private:
  Settings settings;
  static const uint32_t EEPROM_MAGIC = 0xABCDEF00;
  
  bool isValid() const;
};

#endif // MANAGERS_H
