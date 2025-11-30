#ifndef STATE_HANDLERS_H
#define STATE_HANDLERS_H

#include "state_manager.h"
#include "managers.h"
#include "hal.h"
#include <Arduino.h>

/**
 * State Handler Functions
 * 
 * These functions implement the execution logic for each FSM state.
 * They are called repeatedly by the main loop during state execution.
 * 
 * Each handler:
 * 1. Performs state-specific operations
 * 2. Checks for state transition events
 * 3. Calls appropriate manager methods
 * 4. Logs execution details for debugging
 * 5. Returns true if state remains valid, false if recovery needed
 */

// ============================================================================
// INITIALIZATION STATE HANDLER
// ============================================================================
/**
 * Execute Initialization State
 * 
 * Responsibilities:
 * - Initialize all hardware peripherals (GPIO, I2C, SPI, Serial, RTC, SD)
 * - Verify hardware is responsive
 * - Perform recovery operations if needed
 * - Display startup progress
 * - Transition to READY state when complete
 * 
 * @return true if initialization successful, false if recovery needed
 */
bool executeInitializationState();

// ============================================================================
// READY STATE HANDLER
// ============================================================================
/**
 * Execute Ready State
 * 
 * Responsibilities:
 * - Wait for production start signal
 * - Perform periodic maintenance (save counts, check time)
 * - Detect hour changes and update cumulative counts
 * - Monitor system health (heap, temperature, watchdog)
 * - Update display with status information
 * - Transition to PRODUCTION on start signal
 * 
 * @return true if state remains healthy, false if error recovery needed
 */
bool executeReadyState();

// ============================================================================
// PRODUCTION STATE HANDLER
// ============================================================================
/**
 * Execute Production State
 * 
 * Responsibilities:
 * - Count items in real-time
 * - Update OLED display with live count
 * - Save count periodically to avoid data loss
 * - Detect hour changes during production
 * - Monitor for stop signal
 * - Transition to READY on stop signal
 * 
 * @return true if state remains healthy, false if error recovery needed
 */
bool executeProductionState();

// ============================================================================
// DIAGNOSTIC STATE HANDLER
// ============================================================================
/**
 * Execute Diagnostic State
 * 
 * Responsibilities:
 * - Run hardware diagnostics (GPIO, I2C, SPI, RTC, SD, etc)
 * - Display test results on OLED
 * - Log diagnostic results to serial and SD
 * - Verify all critical systems are operational
 * - Return detailed status information
 * - Allow user to exit diagnostic mode
 * 
 * @return true if diagnostics complete, false if critical failure
 */
bool executeDiagnosticState();

// ============================================================================
// ERROR STATE HANDLER
// ============================================================================
/**
 * Execute Error State
 * 
 * Responsibilities:
 * - Display error message on OLED
 * - Log error details to SD card
 * - Attempt recovery operations (reinitialize failed hardware)
 * - Monitor for user recovery signal
 * - Blink LED to indicate error
 * - Transition back to READY or INITIALIZATION on recovery
 * 
 * @return true if error handled, false if unrecoverable
 */
bool executeErrorState();

// ============================================================================
// GUARD CONDITIONS
// ============================================================================

/**
 * Check if system can start production
 * 
 * Conditions:
 * - No production session already active
 * - Display is initialized and responsive
 * - Heap has sufficient free memory (>= 50KB)
 * 
 * @return true if safe to start production
 */
bool canStartProduction();

/**
 * Check if system can stop production
 * 
 * Conditions:
 * - Always true (graceful shutdown at any time)
 * 
 * @return true (always safe to stop)
 */
bool canStopProduction();

/**
 * Check if heap memory is healthy
 * 
 * Conditions:
 * - Free heap >= 50KB (safety threshold)
 * - No fragmentation issues detected
 * 
 * @return true if heap healthy
 */
bool isHeapHealthy();

/**
 * Check if time is valid
 * 
 * Conditions:
 * - Year >= 2020
 * - Month 1-12
 * - Day valid for month
 * - Hour 0-23
 * - Minute 0-59
 * - Second 0-59
 * 
 * @return true if RTC time is valid
 */
bool isTimeValid();

/**
 * Check if display is initialized
 * 
 * @return true if OLED is responsive
 */
bool isDisplayReady();

/**
 * Check if storage is available
 * 
 * @return true if SD card is mounted
 */
bool isStorageAvailable();

/**
 * Check if RTC is initialized
 * 
 * @return true if RTC is responding
 */
bool isRTCReady();

// ============================================================================
// INITIALIZATION SEQUENCE HELPERS
// ============================================================================

/**
 * Initialize Serial Communication
 * 
 * Baud rate: 115200
 * Enables serial logging for all operations
 * 
 * @return true if successful
 */
bool initializeSerial();

/**
 * Initialize GPIO Pins
 * 
 * Configures:
 * - Counter button (GPIO 15, interrupt)
 * - Diagnostic button (GPIO 27, interrupt)
 * - Production latch (GPIO 25, interrupt)
 * - SD chip select (GPIO 26)
 * 
 * @return true if all pins initialized
 */
bool initializeGPIO();

/**
 * Initialize I2C Bus
 * 
 * Devices:
 * - OLED Display (0x3C)
 * - RTC Module (0x68)
 * 
 * Configuration:
 * - SDA: GPIO 21
 * - SCL: GPIO 22
 * - Frequency: 400kHz
 * 
 * @return true if bus initialized
 */
bool initializeI2C();

/**
 * Initialize SPI Bus
 * 
 * Devices:
 * - SD Card Reader
 * 
 * Configuration:
 * - MOSI: GPIO 23
 * - MISO: GPIO 19
 * - CLK: GPIO 18
 * - CS: GPIO 26
 * 
 * @return true if bus initialized
 */
bool initializeSPI();

/**
 * Initialize RTC Module
 * 
 * Synchronizes with DS3231 RTC chip
 * Validates time range
 * 
 * @return true if RTC synchronized
 */
bool initializeRTC();

/**
 * Initialize Display (OLED)
 * 
 * Device:
 * - 128x64 OLED on I2C (0x3C)
 * 
 * @return true if OLED responsive
 */
bool initializeDisplay();

/**
 * Initialize Storage (SD Card)
 * 
 * Auto-detects speed
 * Creates required directories
 * 
 * @return true if SD card mounted
 */
bool initializeStorage();

/**
 * Initialize Managers
 * 
 * Sets up:
 * - ProductionManager
 * - TimeManager
 * - StorageManager
 * - DisplayManager
 * - LoggerManager
 * - ConfigManager
 * 
 * @return true if all managers ready
 */
bool initializeManagers();

/**
 * Recover from Power Loss
 * 
 * Operations:
 * - Restore production session state
 * - Reload count from persistent storage
 * - Verify data integrity
 * - Update display with recovered state
 * 
 * @return true if recovery successful
 */
bool recoverFromPowerLoss();

// ============================================================================
// PERIODIC MAINTENANCE HELPERS (called from READY state)
// ============================================================================

/**
 * Perform Hour Boundary Operations
 * 
 * Called when hour changes:
 * - Update cumulative count
 * - Create daily log file
 * - Reset hourly counters
 * - Log hour transition
 * 
 * @return true if successful
 */
void handleHourBoundary();

/**
 * Save State to Persistent Storage
 * 
 * Called periodically (default every 5 seconds):
 * - Save current count to SD
 * - Update recovery file
 * - Log checkpoint
 * 
 * @return true if save successful
 */
bool saveCheckpoint();

/**
 * Monitor System Health
 * 
 * Checks:
 * - Heap memory (log warning if < 50KB)
 * - Chip temperature (log warning if > 80°C)
 * - Watchdog status (feed watchdog)
 * - Voltage level (if available)
 * 
 * @return true if all checks pass
 */
bool checkSystemHealth();

/**
 * Update Status Display
 * 
 * Shows:
 * - Current time
 * - Cumulative count
 * - Last session info
 * - System status (OK/WARN/ERROR)
 * 
 * @return true if display updated
 */
bool updateStatusDisplay();

// ============================================================================
// PRODUCTION STATE HELPERS
// ============================================================================

/**
 * Handle Production Item Counted
 * 
 * Called from counter interrupt:
 * - Increment session count
 * - Update display (batched every 100ms)
 * - Save count periodically
 * - Log every 100 items
 * 
 * @return true if count incremented
 */
bool handleItemCounted();

/**
 * Update Production Display
 * 
 * Shows:
 * - Current session count
 * - Session start time and duration
 * - Items per minute rate
 * - Estimated completion time (if quota set)
 * 
 * @return true if display updated
 */
bool updateProductionDisplay();

/**
 * Save Production Progress
 * 
 * Called periodically during production:
 * - Save current session count
 * - Update recovery file
 * - Log checkpoint
 * 
 * @return true if save successful
 */
bool saveProductionProgress();

// ============================================================================
// DIAGNOSTIC HELPERS
// ============================================================================

/**
 * Test GPIO Pins
 * 
 * Tests:
 * - All button inputs
 * - LED outputs
 * - Pin responsiveness
 * 
 * @return true if all pins functional
 */
bool testGPIO();

/**
 * Test I2C Bus
 * 
 * Tests:
 * - OLED at 0x3C
 * - RTC at 0x68
 * - Bus speed and reliability
 * 
 * @return true if all devices responding
 */
bool testI2C();

/**
 * Test SPI Bus
 * 
 * Tests:
 * - SD card communication
 * - Bus speed
 * - Data integrity
 * 
 * @return true if bus functional
 */
bool testSPI();

/**
 * Test RTC Functionality
 * 
 * Tests:
 * - Time accuracy
 * - Oscillator frequency
 * - Alarm functionality
 * 
 * @return true if RTC accurate
 */
bool testRTC();

/**
 * Test Storage
 * 
 * Tests:
 * - SD card read/write
 * - File creation/deletion
 * - Sufficient free space
 * 
 * @return true if storage functional
 */
bool testStorage();

/**
 * Test Memory
 * 
 * Tests:
 * - Free heap size
 * - EEPROM read/write
 * - No memory corruption
 * 
 * @return true if memory healthy
 */
bool testMemory();

/**
 * Run All Diagnostics
 * 
 * Runs complete diagnostic suite
 * Displays results on OLED
 * Logs results to SD card
 * 
 * @return true if all tests pass
 */
bool runAllDiagnostics();

#endif // STATE_HANDLERS_H
