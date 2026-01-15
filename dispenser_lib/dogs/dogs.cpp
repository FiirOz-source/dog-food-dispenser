/**
 * @file  dogs.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 2024
 * @brief Dogs implementation - dog object and feeding schedule tracking.
 *
 * This file contains the implementation for the dog class, including
 * RFID tag matching, feeding schedule tracking, and interval enforcement.
 */

#include "dogs.hpp"

/**
 * @brief Check if a given RFID tag matches this dog's identification tag.
 *
 * Performs a string comparison between the provided tag and this dog's registered tag.
 *
 * @param tag RFID tag string to match (typically 8-character alphanumeric)
 *
 * @return bool - true if tag == this dog's rfid_tag, false otherwise
 *
 * @details
 * - Uses Arduino String equality operator (==)
 * - Case-sensitive comparison
 * - Empty or null tags will return false
 *
 * Example:
 * @code
 * dog jop("Jop", "0080D552");
 * bool is_jop = jop.matches_tag("0080D552");  // Returns true
 * bool is_not_jop = jop.matches_tag("002E2989");  // Returns false
 * @endcode
 */
bool dispenser_lib::dogs::dog::matches_tag(const String tag)
{
    return tag == rfid_tag;
}

/**
 * @brief Record that this dog has just been fed.
 *
 * Updates the last feeding timestamp to the current system uptime.
 * This enforces the 12-hour minimum feeding interval.
 *
 * @return void
 *
 * @details
 * - Records current millis() as last_fed_ms
 * - Sets fed_once flag to true
 * - Should be called immediately after food dispensing
 *
 * Example (in feeding sequence):
 * @code
 * // Dog detected and RFID matched
 * if (jop.can_feed()) {
 *     jop.mark_fed();             // Record feeding time
 *     servo_motor->open();        // Dispense food
 *     delay(2000);
 *     servo_motor->close();
 * }
 * @endcode
 *
 * @note
 * - Timing uses Arduino millis() which wraps every ~50 days
 * - Subsequent can_feed() calls will block for 12 hours from this timestamp
 * - Must be called for feeding interval enforcement to work
 */
void dispenser_lib::dogs::dog::mark_fed()
{
    last_fed_ms = millis();
    fed_once = true;
}

/**
 * @brief Get elapsed time since this dog was last fed.
 *
 * Returns the duration in milliseconds since mark_fed() was called.
 *
 * @return unsigned long
 *         - 0UL if dog has never been fed
 *         - Otherwise: (millis() - last_fed_ms) as unsigned long
 *
 * @details
 * - Returns 0 if fed_once is false
 * - Otherwise returns the time difference in milliseconds
 * - Safe for comparison and arithmetic due to unsigned long
 *
 * Example:
 * @code
 * unsigned long ms_ago = jop.time_since_fed();
 * unsigned long seconds_ago = ms_ago / 1000UL;
 * unsigned long minutes_ago = seconds_ago / 60UL;
 * unsigned long hours_ago = minutes_ago / 60UL;
 * @endcode
 *
 * @note
 * - millis() wraps every ~50 days on ESP8266
 * - Unsigned arithmetic handles wraparound correctly
 * - Used internally by can_feed() for interval checking
 */
unsigned long dispenser_lib::dogs::dog::time_since_fed()
{
    if (!fed_once)
    {
        return 0UL;
    }
    return (unsigned long)(millis() - last_fed_ms);
}

/**
 * @brief Determine if this dog is eligible to be fed (12-hour interval met).
 *
 * Checks if the required 12-hour interval has passed since the last feeding.
 *
 * @return bool
 *         - true if dog has never been fed, OR at least 12 hours have elapsed
 *         - false if less than 12 hours since last feeding
 *
 * @details
 * **Feeding Interval:**
 * - Dogs are limited to one feeding per 12-hour period
 * - 12 hours = 43,200,000 milliseconds (12 * 60 * 60 * 1000)
 * - Uses unsigned long arithmetic to handle wraparound
 *
 * **Logic:**
 * ```
 * if (!fed_once) return true;  // Never fed → allow feeding
 *
 * unsigned long TWELVE_HOURS_MS = 43200000;
 * return (millis() - last_fed_ms) >= TWELVE_HOURS_MS;
 * ```
 *
 * **Wraparound Handling:**
 * - millis() wraps every ~49.7 days on ESP8266
 * - Unsigned arithmetic handles this correctly
 * - Feeding interval remains accurate across wraparound
 *
 * Example:
 * @code
 * if (jop.can_feed()) {
 *     jop.mark_fed();
 *     servo_motor->open();
 *     delay(2000);
 *     servo_motor->close();
 * } else {
 *     String when = jop.since_fed();
 *     lcd->display("Jop already fed");
 *     lcd->display(when.c_str());
 * }
 * @endcode
 *
 * @see mark_fed(), since_fed()
 */
bool dispenser_lib::dogs::dog::can_feed()
{
    if (!fed_once)
    {
        return true;
    }
    const unsigned long TWELVE_HOURS_MS = 12UL * 60UL * 60UL * 1000UL; // 43 200 000 ms
    return (unsigned long)(millis() - last_fed_ms) >= TWELVE_HOURS_MS;
}

/**
 * @brief Format time since last feeding as a human-readable string.
 *
 * Generates a formatted string describing how long ago this dog was last fed.
 *
 * @return String
 *         - "never" if dog has never been fed
 *         - Otherwise: formatted time string (e.g., "2h30m15s ago")
 *
 * @details
 * **Format Examples:**
 * ```
 * "never"           → Never fed
 * "5m30s ago"       → Less than 1 hour
 * "2h30m15s ago"    → 2+ hours
 * "1d5h30m ago"     → 1+ days (includes day, hour, minute)
 * "10d0h5m20s ago"  → 10+ days
 * ```
 *
 * **Calculation:**
 * - Base time: time_since_fed() in milliseconds
 * - Breaks down: days (86400 sec), hours, minutes, seconds
 * - Conditional formatting: omits days if < 1 day
 *
 * **Buffer & Format:**
 * - Uses snprintf() with 48-byte buffer
 * - Format specifiers: %lu for unsigned long
 * - Safe for display on 16-character LCD (may truncate)
 *
 * Example Output Sequence:
 * @code
 * mark_fed() at time 0
 *
 * At 30 seconds:    since_fed() → "30s ago"
 * At 5 minutes:     since_fed() → "5m0s ago"
 * At 2.5 hours:     since_fed() → "2h30m0s ago"
 * At 1.2 days:      since_fed() → "1d4h48m0s ago"
 * At 12 hours:      can_feed() → true (allows feeding)
 * @endcode
 *
 * @note
 * - Output varies based on elapsed time
 * - Used when showing "dog already fed" message
 * - Formatting is consistent across all elapsed times
 * - Safe for use on small displays due to truncation handling
 *
 * @see can_feed(), time_since_fed()
 */
String dispenser_lib::dogs::dog::since_fed()
{
    if (!fed_once)
    {
        return "never";
    }

    unsigned long sec = (unsigned long)(millis() - last_fed_ms) / 1000UL;

    unsigned long days = sec / 86400UL;
    sec %= 86400UL;
    unsigned long hrs = sec / 3600UL;
    sec %= 3600UL;
    unsigned long mins = sec / 60UL;
    sec %= 60UL;

    char buf[48];
    if (days > 0)
    {
        snprintf(buf, sizeof(buf), "%lud%luh%lum%lus ago", days, hrs, mins, sec);
    }
    else if (hrs > 0)
    {
        snprintf(buf, sizeof(buf), "%luh%lum%lus ago", hrs, mins, sec);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%lum%lus ago", mins, sec);
    }

    return String(buf);
}