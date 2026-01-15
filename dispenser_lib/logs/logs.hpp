/**
 * @file  logs.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 2024
 * @brief Logs header file - event logging with automatic timestamps.
 *
 * This file defines the logger class for recording system events with
 * automatic timestamps (date/time or uptime).
 */

#ifndef LOGS_HPP
#define LOGS_HPP

#include <Arduino.h>
#include <vector>

/**
 * @namespace dispenser_lib::logs
 * @brief Application event logging with timestamps.
 *
 * Provides simple event logging with automatic timestamp generation.
 * Timestamps are formatted in French locale (day, month, time).
 * Available via web API endpoint (/logs) for remote monitoring.
 */
namespace dispenser_lib
{
    namespace logs
    {
        /**
         * @class logger
         * @brief Application event logger with automatic timestamps.
         *
         * Records system events (dog detected, fed, errors, etc.) with
         * automatic timestamps. Timestamps are:
         * - In French locale (day/month format, French month names)
         * - Either real date/time (via NTP) or uptime (before NTP sync)
         * - Automatically added to each log entry
         *
         * Entries are stored in memory and accessible via web API.
         */
        class logger
        {
        public:
            /// Default constructor
            logger() = default;

            /// Default destructor
            ~logger() = default;

            /**
             * @brief Add a message to the application log.
             *
             * Appends a message to the event log with automatic timestamp.
             * Uses the += operator for convenient logging.
             *
             * @param message String message to log
             *
             * @return Reference to this logger object (for chaining)
             *
             * @details
             * **Timestamp Format:**
             * - Before NTP sync: "[Uptime HH:MM] message"
             * - After NTP sync: "[Day Date Month HH:MM] message"
             *
             * **French Locale Examples:**
             * - "[Uptime 00:05] Waiting for dog"
             * - "[Lun. 15 janv. 12:30] Feeding Jop!"
             * - "[Dim. 1 fevr. 09:45] Init Complete"
             *
             * **Month Names (French):**
             * janv., fevr., mars, avr., mai, juin, juil., aout, sept., oct., nov., dec.
             *
             * **Day Names (French):**
             * Dim., Lun., Mar., Mer., Jeu., Ven., Sam.
             *
             * @note
             * - Timestamps are added automatically
             * - Suitable for monitoring system behavior
             * - Accessible via web API /logs endpoint
             *
             * Example:
             * @code
             * app_log += "Dog detected";
             * app_log += String("RFID: ") + rfid_tag;
             * app_log += "Feeding Jop!";
             * @endcode
             *
             * @see entries()
             */
            logger &operator+=(const String &message);

            /**
             * @brief Get all log entries recorded so far.
             *
             * Returns a const reference to the vector of all logged messages.
             *
             * @return const reference to std::vector<String> containing all entries
             *
             * @details
             * - Returns all messages with timestamps
             * - Read-only access (const reference)
             * - Suitable for iterating through log history
             * - Used by web API /logs endpoint to send entries as JSON
             *
             * Example:
             * @code
             * const auto& log_entries = app_log.entries();
             * for (size_t i = 0; i < log_entries.size(); ++i) {
             *     Serial.println(log_entries[i]);
             * }
             * @endcode
             *
             * @note
             * - Web API pagination: can request entries from index N onward
             * - Total entry count available for pagination support
             * - Logs persist in RAM until power loss
             */
            const std::vector<String> &entries() const;

        private:
            /// Vector storing all log entries with timestamps
            std::vector<String> log_entries;
        };

    } // namespace logs

} // namespace dispenser_lib

#endif // LOGS_HPP
