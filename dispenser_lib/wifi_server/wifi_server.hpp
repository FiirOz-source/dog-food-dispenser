/**
 * @file  wifi_server.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 2024
 * @brief WiFi server header file - web API for remote monitoring and control.
 *
 * This file defines the WiFi/web server functionality for remote monitoring
 * of the dog food dispenser system status, logs, and manual control.
 */

#ifndef WIFI_SERVER_HPP
#define WIFI_SERVER_HPP

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

// ============================================================================
// WiFi Credentials
// ============================================================================

/** @brief WiFi SSID (network name) to connect to */
extern const char *ssid;

/** @brief WiFi password for authentication */
extern const char *password;

// ============================================================================
// Web Server Instance
// ============================================================================

/** @brief Global ESP8266WebServer instance running on port 80 */
extern ESP8266WebServer server;

/**
 * @namespace dispenser_lib::wifi_server
 * @brief WiFi connectivity and web server for remote monitoring.
 *
 * Provides:
 * - WiFi network connection
 * - HTTP web server on port 80
 * - REST API endpoints for status, logs, and manual feeding
 * - Static file serving (HTML/CSS/JS frontend)
 * - NTP time synchronization for accurate timestamps
 */
namespace dispenser_lib
{
    namespace wifi_server
    {
        /**
         * @brief Initialize and synchronize time via NTP.
         *
         * Connects to NTP servers to obtain current date/time for accurate
         * event logging timestamps.
         *
         * @return void
         *
         * @details
         * **NTP Configuration:**
         * - Primary NTP server: pool.ntp.org
         * - Secondary: time.google.com
         * - Tertiary: time.nist.gov
         * - Timezone: CET (Central European Time, UTC+1 winter, UTC+2 summer)
         *
         * **Process:**
         * 1. Configure timezone to CET-1CEST (Central European Time with DST)
         * 2. Poll NTP servers until valid time obtained
         * 3. Validate time is >= Jan 1, 2021 (1609459200 Unix timestamp)
         * 4. Block loop until time synchronization complete
         *
         * **Time Synchronization:**
         * - Minimum value: Jan 1, 2021 (1609459200)
         * - Actual time obtained from NTP (2024+)
         * - Used for accurate log timestamps
         * - Timezone auto-adjusts for daylight saving time
         *
         * **Timezone Details:**
         * - "CET-1CEST,M3.5.0/2,M10.5.0/3"
         * - CET-1: Central European Time, UTC+1 (winter)
         * - CEST: Central European Summer Time, UTC+2
         * - M3.5.0/2: Switch to CEST on last Sunday of March at 2am
         * - M10.5.0/3: Switch back to CET on last Sunday of October at 3am
         *
         * @note
         * - Blocking call: waits until NTP sync complete
         * - Should be called after WiFi is connected
         * - Typical sync time: 1-5 seconds after WiFi connection
         * - Called automatically by init_wifi_server()
         *
         * @see init_wifi_server()
         */
        void init_time_ntp();

        /**
         * @brief Initialize WiFi connection and web server.
         *
         * Connects to WiFi network and starts HTTP web server with REST API
         * endpoints and static file serving.
         *
         * @return void
         *
         * @details
         * **Initialization Steps:**
         *
         * 1. **WiFi Connection:**
         *    - Set WiFi mode to STA (Station)
         *    - Connect to network with configured SSID and password
         *    - Wait for connection (shows dots on Serial)
         *    - Print IP address on success
         *
         * 2. **File System:**
         *    - Initialize LittleFS (Little Flash File System)
         *    - Serves static files from /data/index.html
         *    - Fatal error if LittleFS init fails
         *
         * 3. **Static File Serving:**
         *    - Route "/" serves /index.html (web interface)
         *    - HTML/CSS/JS frontend for remote monitoring
         *
         * 4. **REST API Endpoints:**
         *
         *    **GET /status** - System status JSON
         *    - event: Last event description
         *    - distance_cm: Current food level distance (cm)
         *    - food_percent: Calculated food percentage (0-100)
         *    - last_rfid: Last detected RFID tag
         *    - uptime_ms: System uptime in milliseconds
         *    - jop_last_fed_uptime_ms: Jop's last feeding timestamp
         *    - manouk_last_fed_uptime_ms: Manouk's last feeding timestamp
         *
         *    **GET /logs** - Event log entries (paginated)
         *    Query parameters:
         *    - from (optional): Start index for pagination (default: 0)
         *
         *    Response:
         *    - total: Total number of log entries
         *    - from: Starting index of returned entries
         *    - entries: Array of log entry strings
         *
         *    **POST /dispense** - Manual food dispensing
         *    - Triggers manual feeding via web_dispense()
         *    - Returns: "ok" on success
         *    - No RFID checking or feeding interval enforcement
         *    - For maintenance and testing
         *
         *    **404 Handling** - Not Found responses
         *    - Returns 404 status with "404 - introuvable" message
         *
         * 5. **NTP Time Synchronization:**
         *    - Calls init_time_ntp() to sync time for accurate logging
         *
         * 6. **Server Start:**
         *    - Starts listening on port 80
         *    - Prints "Serveur OK." to Serial on success
         *
         * **WebServer Handler Implementation:**
         * - GET /status: Builds and returns JSON with current system status
         * - GET /logs: Returns JSON array of log entries with pagination support
         * - POST /dispense: Sets web_dispense_request flag for main loop
         * - Other routes: Returns 404 Not Found
         *
         * **JSON Response Examples:**
         * ```json
         * // /status response:
         * {
         *   "event": "Waiting for dog",
         *   "distance_cm": 45,
         *   "food_percent": 55,
         *   "last_rfid": "",
         *   "uptime_ms": 12345678,
         *   "jop_last_fed_uptime_ms": 0,
         *   "manouk_last_fed_uptime_ms": 0
         * }
         *
         * // /logs response:
         * {
         *   "total": 25,
         *   "from": 0,
         *   "entries": [
         *     "[Uptime 00:00] Dog Feeder",
         *     "[Uptime 00:02] Waiting for dog",
         *     "[Lun. 15 janv. 12:30] Dog detected"
         *   ]
         * }
         * ```
         *
         * @note
         * - SSID and password must be configured before calling
         * - Blocking WiFi connection loop may take 5-10 seconds
         * - If LittleFS fails, prints error but continues
         * - Web server handles requests in main loop via server.handleClient()
         * - All JSON endpoints escape special characters properly
         *
         * **Network Configuration (Hardcoded):**
         * - SSID: "iPhone de Timothée"
         * - Password: "TimAuThe"
         * - Mode: WiFi Station (STA)
         * - Port: 80 (HTTP)
         *
         * @see init_time_ntp()
         */
        void init_wifi_server();

    } // namespace wifi_server
} // namespace dispenser_lib

#endif // WIFI_SERVER_HPP
