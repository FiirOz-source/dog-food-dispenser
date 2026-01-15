/**
 * @file  dogs.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 2024
 * @brief Dogs data model header file - dog object and feeding schedule tracking.
 *
 * This file defines the dog class for representing individual dogs with their
 * RFID tag identification and feeding schedule tracking.
 */

#ifndef DOGS_HPP
#define DOGS_HPP

#include <Arduino.h>

/**
 * @namespace dispenser_lib::dogs
 * @brief Dog object representation and feeding schedule management.
 *
 * Contains the dog class for tracking individual dog identification (RFID tags),
 * feeding history, and enforcing feeding interval constraints (12-hour minimum).
 */
namespace dispenser_lib
{
    namespace dogs
    {
        /**
         * @class dog
         * @brief Represents a single dog with RFID tag and feeding schedule.
         *
         * This class tracks:
         * - Dog's name (display purposes)
         * - RFID tag ID for automatic identification
         * - Last feeding time and interval enforcement
         * - Convenience methods for feeding schedule queries
         */
        class dog
        {
        public:
            /// Default constructor
            dog() = default;

            /// Default destructor
            ~dog() = default;

            /**
             * @brief Constructor with dog identification.
             *
             * @param dog_name Name of the dog (e.g., "Jop", "Manouk")
             * @param tag_rfid 8-character RFID tag ID (e.g., "0080D552")
             *
             * @note
             * - Name is stored but currently only used for logging
             * - RFID tag is used for automatic dog identification
             * - Initial state: never fed (fed_once = false)
             */
            dog(const String &dog_name, const String &tag_rfid)
                : name(dog_name), rfid_tag(tag_rfid) {}

            /**
             * @brief Check if a given RFID tag matches this dog's tag.
             *
             * @param tag RFID tag string to compare (typically 8 characters)
             *
             * @return true if tag matches this dog's rfid_tag, false otherwise
             *
             * @details
             * - Simple string equality comparison
             * - Case-sensitive
             * - Empty tags will not match
             *
             * Example:
             * @code
             * dog jop("Jop", "0080D552");
             * if (jop.matches_tag("0080D552")) {
             *     Serial.println("Jop detected!");
             * }
             * @endcode
             */
            bool matches_tag(const String tag);

            /**
             * @brief Mark this dog as fed at the current time.
             *
             * Records the current system uptime (millis()) as the last feeding time.
             * Used after dispensing food to enforce the 12-hour feeding interval.
             *
             * @return void
             *
             * @details
             * - Sets last_fed_ms to current millis()
             * - Sets fed_once to true (dog has been fed at least once)
             * - Should be called immediately after food dispensing
             *
             * @note
             * - Timing: relies on system millis() accuracy
             * - Overflow: millis() wraps around every ~50 days on ESP8266
             * - Can_feed() handles this correctly via unsigned integer arithmetic
             *
             * Example:
             * @code
             * // After successfully dispensing food:
             * jop.mark_fed();  // Record the feeding time
             * @endcode
             */
            void mark_fed();

            /**
             * @brief Get time elapsed since this dog was last fed.
             *
             * Returns the duration in milliseconds since the last feeding.
             *
             * @return Milliseconds since last feeding
             *         - 0 if dog has never been fed (fed_once == false)
             *         - Otherwise: (millis() - last_fed_ms) as unsigned long
             *
             * @details
             * - Uses unsigned long arithmetic to handle millis() overflow
             * - Returns 0 if dog has never been fed
             * - Safe for use in feeding interval calculations
             *
             * @note
             * - Can overflow after ~50 days on ESP8266 (handled by unsigned arithmetic)
             * - Used internally by can_feed() for interval checking
             *
             * Example:
             * @code
             * unsigned long ms_since = jop.time_since_fed();
             * unsigned long hours_since = ms_since / (3600UL * 1000UL);
             * @endcode
             */
            unsigned long time_since_fed();

            /**
             * @brief Check if this dog is allowed to eat (12-hour interval met).
             *
             * Returns true if the dog can be fed based on the feeding interval constraint.
             *
             * @return bool
             *         - true if dog has never been fed OR 12+ hours have elapsed since last feeding
             *         - false if less than 12 hours since last feeding
             *
             * @details
             * **Feeding Interval Constraint:**
             * - Each dog can eat at most once per 12 hours
             * - 12 hours = 43,200,000 milliseconds
             * - Uses unsigned long arithmetic to safely compare time differences
             * - Handles millis() wraparound correctly (~50 days)
             *
             * **Logic:**
             * ```cpp
             * if (!fed_once) return true;  // Never fed → can feed
             *
             * uint32_t elapsed = millis() - last_fed_ms;
             * return elapsed >= 43_200_000;  // 12 hours in ms
             * ```
             *
             * @note
             * - Used in handle_dog_detected() to enforce feeding constraints
             * - Prevents overfeeding dogs
             * - Time basis: system uptime (millis), wraps every ~50 days
             *
             * Example:
             * @code
             * if (jop.can_feed()) {
             *     // Dispense food and mark_fed()
             *     servo_motor->open();
             *     delay(2000);
             *     servo_motor->close();
             *     jop.mark_fed();
             * } else {
             *     // Show "already fed" message
             *     String time_left = jop.since_fed();
             *     lcd->display(time_left);
             * }
             * @endcode
             *
             * @see mark_fed(), since_fed()
             */
            bool can_feed();

            /**
             * @brief Get a human-readable string of time since last feeding.
             *
             * Returns a formatted string describing when the dog was last fed.
             *
             * @return String
             *         - "never" if dog has never been fed
             *         - Otherwise: formatted duration string (e.g., "2h30m15s ago")
             *
             * @details
             * **Format Examples:**
             * - "never" → Dog has never been fed
             * - "5m30s ago" → 5 minutes 30 seconds (less than 1 hour)
             * - "2h30m15s ago" → 2 hours 30 minutes 15 seconds
             * - "1d5h30m ago" → 1 day 5 hours 30 minutes (days shown if >= 1)
             * - "2d0h0m1s ago" → 2 days (with zero-padded fields)
             *
             * **Calculation:**
             * - Time base: millis() since last_fed_ms
             * - Breaks down into days, hours, minutes, seconds
             * - Formats with snprintf() for consistency
             * - Omits days field if < 1 day has elapsed
             *
             * @note
             * - Output is suitable for 16-character LCD display (may truncate)
             * - Used when dog cannot feed yet (e.g., "Jop already fed" message)
             * - Maximum buffer: 48 characters (plenty for format string)
             *
             * Example Output:
             * @code
             * String when = jop.since_fed();
             * Serial.println("Jop fed: " + when);
             * // Output: "Jop fed: 2h30m15s ago"
             * @endcode
             *
             * @see time_since_fed(), can_feed()
             */
            String since_fed();

        private:
            /// Dog's name (display purposes, e.g., "Jop", "Manouk")
            String name;
            /// 8-character RFID tag ID for dog identification (e.g., "0080D552")
            String rfid_tag;

            /// System uptime (millis) when dog was last fed
            unsigned long last_fed_ms = 0;
            /// Flag: true if dog has been fed at least once
            bool fed_once = false;
        };

    } // namespace dogs
} // namespace dispenser_lib

#endif // DOGS_HPP
