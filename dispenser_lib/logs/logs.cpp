/**
 * @file  logs.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 2024
 * @brief Logs implementation - event logging with automatic timestamps.
 *
 * This file contains the implementation for the logger class, including
 * timestamp generation in French locale and log entry management.
 */

#include "logs.hpp"
#include <time.h>
#include <stdio.h>

// ============================================================================
// Helper Functions for Timestamp Generation
// ============================================================================

/**
 * @brief Check if the system time is valid (NTP synchronized).
 *
 * Verifies whether the system has obtained a valid time from NTP server.
 *
 * @return bool
 *         - true if system time is valid (synchronized via NTP)
 *         - false if system time has not been synchronized
 *
 * @details
 * **NTP Synchronization Check:**
 * - Checks if time() >= January 1, 2021 (1609459200 seconds since epoch)
 * - Before NTP sync, time() returns a very small value
 * - After NTP sync, returns current Unix timestamp (large value)
 *
 * **Purpose:**
 * - Determines whether to use real date/time or just uptime for timestamps
 * - Used to switch timestamp format after WiFi/NTP initialization
 *
 * **Timeline:**
 * - During boot: time_is_valid() returns false
 * - After WiFi connects and NTP syncs: returns true
 * - init_time_ntp() waits for this condition
 */
static bool time_is_valid()
{
    time_t now = time(nullptr);
    return now >= 1609459200;
}

/**
 * @brief Generate a timestamp string in French locale.
 *
 * Creates a formatted timestamp with:
 * - Real date/time if NTP synchronized (French locale)
 * - System uptime if NTP not yet synchronized
 *
 * @return String containing the formatted timestamp
 *
 * @details
 * **Format if NTP Synchronized (After WiFi Connection):**
 * ```
 * "[Lun. 15 janv. 12:30] "
 * Format: [Day. Date Month HH:MM] (space after bracket)
 * ```
 *
 * **Format if Not Yet Synchronized (During Boot):**
 * ```
 * "[Uptime 00:05] "
 * Format: [Uptime HH:MM] where HH:MM is uptime hours and minutes
 * ```
 *
 * **French Locale Components:**
 *
 * Days of Week (3-character abbreviations):
 * - Dim. (Sunday), Lun. (Monday), Mar. (Tuesday), Mer. (Wednesday)
 * - Jeu. (Thursday), Ven. (Friday), Sam. (Saturday)
 *
 * Months (4-character abbreviations):
 * - janv., fevr., mars, avr., mai, juin, juil., aout, sept., oct., nov., dec.
 *
 * **Calculation if NTP Valid:**
 * - Calls time(nullptr) to get current Unix timestamp
 * - Converts to local time struct via localtime_r()
 * - Extracts tm_wday (day of week), tm_mday (date), tm_mon (month),
 *   tm_hour (hour), tm_min (minute)
 * - Formats with snprintf(): "[%s %d %s %02d:%02d] "
 *
 * **Calculation if NTP Invalid (Uptime Mode):**
 * - Calculates total_min = millis() / 60000 (uptime in minutes)
 * - Calculates hh = (total_min / 60) % 100 (hours, clamped to 00-99)
 * - Calculates mm = total_min % 60 (minutes)
 * - Formats with snprintf(): "[Uptime %02lu:%02lu] "
 *
 * @note
 * - Buffer size: 48 bytes (sufficient for longest format)
 * - French locale hardcoded (not locale-aware)
 * - Output includes trailing space for message concatenation
 *
 * Example Outputs:
 * @code
 * // During boot (before NTP sync):
 * make_timestamp_fr() → "[Uptime 00:00] "
 * make_timestamp_fr() → "[Uptime 00:05] "
 * make_timestamp_fr() → "[Uptime 00:15] "
 *
 * // After NTP sync (with WiFi):
 * make_timestamp_fr() → "[Lun. 15 janv. 12:30] "
 * make_timestamp_fr() → "[Dim. 1 fevr. 09:45] "
 * @endcode
 *
 * @see time_is_valid()
 */
static String make_timestamp_fr()
{
    static const char *kDays[] = {"Dim.", "Lun.", "Mar.", "Mer.", "Jeu.", "Ven.", "Sam."};
    static const char *kMonths[] = {"janv.", "fevr.", "mars", "avr.", "mai", "juin", "juil.", "aout", "sept.", "oct.", "nov.", "dec."};

    char buf[48];

    if (time_is_valid())
    {
        time_t now = time(nullptr);
        struct tm tm_now;
        localtime_r(&now, &tm_now);

        snprintf(
            buf, sizeof(buf),
            "[%s %d %s %02d:%02d] ",
            kDays[tm_now.tm_wday],
            tm_now.tm_mday,
            kMonths[tm_now.tm_mon],
            tm_now.tm_hour,
            tm_now.tm_min);
        return String(buf);
    }

    uint32_t total_min = millis() / 60000UL;
    uint32_t hh = (total_min / 60UL) % 100UL;
    uint32_t mm = total_min % 60UL;

    snprintf(buf, sizeof(buf), "[Uptime %02lu:%02lu] ", (unsigned long)hh, (unsigned long)mm);
    return String(buf);
}

// ============================================================================
// Logger Implementation
// ============================================================================

/**
 * @brief Add a message to the application log with automatic timestamp.
 *
 * Records a message with an automatically generated timestamp and adds it
 * to the internal log entry vector.
 *
 * @param message String message to log
 *
 * @return Reference to this logger object (allows operator chaining)
 *
 * @details
 * **Process:**
 * 1. Generate timestamp via make_timestamp_fr()
 * 2. Concatenate timestamp + message
 * 3. Add complete line to log_entries vector
 * 4. Return reference to this for chaining
 *
 * **Timestamp Behavior:**
 * - Before NTP sync: "[Uptime HH:MM] message"
 * - After NTP sync: "[Lun. 15 janv. 12:30] message"
 *
 * **Example Usage:**
 * @code
 * app_log += "System boot complete";
 * app_log += String("IP: ") + WiFi.localIP().toString();
 * app_log += "Dog Feeder ready";
 * @endcode
 *
 * **Chaining Example:**
 * @code
 * (app_log += "Event 1") += "Event 2";  // Adds both messages
 * @endcode
 *
 * @note
 * - Each call creates a new log entry (one per message)
 * - Timestamp is unique to each entry
 * - Logs persist in memory (lost on power loss)
 * - Total log size limited by available ESP8266 RAM
 *
 * @see entries()
 */
dispenser_lib::logs::logger &dispenser_lib::logs::logger::operator+=(const String &message)
{
    String line = make_timestamp_fr() + message;
    log_entries.push_back(line);
    return *this;
}

/**
 * @brief Get a const reference to all logged entries.
 *
 * Provides read-only access to the complete log history.
 *
 * @return const reference to std::vector<String> containing all log entries
 *
 * @details
 * - Each entry includes timestamp and message
 * - Entries are in chronological order (appended as events occur)
 * - Read-only access (cannot modify via returned reference)
 * - Used by web API to send logs as JSON array
 *
 * **Web API Usage:**
 * - HTTP GET /logs?from=N returns all entries from index N onward
 * - Total entry count and from index included in JSON response
 * - Supports pagination of large log files
 *
 * **Direct Access Example:**
 * @code
 * const auto& entries = app_log.entries();
 * Serial.println("Total entries: " + String(entries.size()));
 * for (size_t i = 0; i < entries.size(); ++i) {
 *     Serial.println(entries[i]);
 * }
 * @endcode
 *
 * @note
 * - Returns reference (no copying of entire vector)
 * - size() method can be used to determine total entries
 * - Suitable for displaying log summary or debug output
 *
 * @see operator+=()
 */
const std::vector<String> &dispenser_lib::logs::logger::entries() const
{
    return log_entries;
}
