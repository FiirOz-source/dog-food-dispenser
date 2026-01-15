/**
 * @file  wifi_server.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 2024
 * @brief WiFi server implementation - web API for remote monitoring and control.
 *
 * This file contains the implementation for WiFi connectivity, web server,
 * REST API endpoints, and NTP time synchronization.
 */

#include "wifi_server.hpp"
#include "dispenser.hpp"

// ============================================================================
// WiFi Configuration
// ============================================================================

/** WiFi network name (SSID) */
const char *ssid = "iPhone de Timothée";

/** WiFi network password */
const char *password = "TimAuThe";

// ============================================================================
// Web Server Instance
// ============================================================================

/** Global web server instance listening on port 80 (HTTP) */
ESP8266WebServer server(80);

// ============================================================================
// Function Implementations
// ============================================================================

/**
 * @brief Initialize time synchronization via NTP (Network Time Protocol).
 *
 * Synchronizes the ESP8266 system clock with NTP servers to obtain accurate
 * current date and time. This is essential for event logging with proper timestamps.
 *
 * @return void
 *
 * @details
 * **NTP Server Configuration:**
 * - Primary: pool.ntp.org (primary NTP pool)
 * - Secondary: time.google.com (Google's public NTP)
 * - Tertiary: time.nist.gov (NIST public NTP)
 *
 * **Timezone Configuration:**
 * - Format: "CET-1CEST,M3.5.0/2,M10.5.0/3"
 * - CET: Central European Time (UTC+1 in winter)
 * - CEST: Central European Summer Time (UTC+2 in summer)
 * - M3.5.0/2: Change to CEST on last Sunday of March at 2:00 AM
 * - M10.5.0/3: Change to CET on last Sunday of October at 3:00 AM
 *
 * **Synchronization Process:**
 * 1. Call configTime() with NTP servers and timezone
 * 2. Call tzset() to apply timezone settings
 * 3. Loop until time() returns a valid timestamp (>= Jan 1, 2021)
 * 4. Check every 200ms with yield() to prevent watchdog timeout
 * 5. Return when valid time obtained
 *
 * **Time Validation:**
 * - Minimum valid timestamp: 1609459200 (January 1, 2021)
 * - Boot timestamp starts at ~1000 (too low)
 * - NTP sync raises timestamp to ~1700000000+ (actual current time)
 *
 * @note
 * - Blocking call: waits indefinitely until NTP sync complete
 * - Typical sync time: 1-5 seconds after WiFi connection
 * - Called automatically by init_wifi_server()
 * - Must be called AFTER WiFi is connected
 * - Sets global time for all system operations
 *
 * @warning
 * - If called before WiFi is ready, will hang indefinitely
 * - Ensure WiFi.status() == WL_CONNECTED before calling
 *
 * @see init_wifi_server()
 */
void dispenser_lib::wifi_server::init_time_ntp()
{
    // Configure time with NTP servers (0, 0 = UTC offset handled by tzset)
    configTime(0, 0, "pool.ntp.org", "time.google.com", "time.nist.gov");

    // Set timezone to Central European Time with daylight saving
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

    // Wait for NTP synchronization
    time_t now = time(nullptr);
    while (now < 1609459200) // Before Jan 1, 2021 = not synchronized
    {
        delay(200);
        yield(); // Allow ESP8266 to handle WiFi and other tasks
        now = time(nullptr);
    }
}

/**
 * @brief Initialize WiFi connectivity and web server with REST API.
 *
 * Performs complete system initialization:
 * - Connects to WiFi network
 * - Initializes file system
 * - Sets up REST API endpoints
 * - Synchronizes time via NTP
 * - Starts HTTP web server
 *
 * @return void
 *
 * @details
 * **WiFi Connection Process:**
 * 1. Set WiFi mode to STA (Station mode)
 * 2. Connect to configured SSID and password
 * 3. Wait for connection (show progress dots on Serial)
 * 4. Print connection status and IP address
 *
 * **File System Initialization:**
 * - Initialize LittleFS (Flash file system)
 * - Serves static files from /data/ directory
 * - Specifically: /data/index.html for web UI
 * - Prints error if initialization fails but continues
 *
 * **Static File Serving:**
 * - Route: GET / → /index.html
 * - Returns HTML/CSS/JavaScript web interface
 * - Allows remote monitoring via web browser
 *
 * **REST API Endpoints:**
 *
 * **GET /status**
 * - Returns current system status as JSON
 * - Fields:
 *   - event: Last event description
 *   - distance_cm: Food level distance in cm
 *   - food_percent: Calculated food percentage (0-100)
 *   - last_rfid: Last detected RFID tag ID
 *   - uptime_ms: System uptime in milliseconds
 *   - jop_last_fed_uptime_ms: Jop's last feeding uptime
 *   - manouk_last_fed_uptime_ms: Manouk's last feeding uptime
 *
 * **GET /logs?from=N**
 * - Returns event log entries as JSON array
 * - Query parameter: from (optional, default 0)
 * - Returns: total, from, entries array
 * - Supports pagination of large logs
 * - Escapes special characters in entries (quotes, newlines, etc.)
 *
 * **POST /dispense**
 * - Triggers manual food dispensing
 * - Sets web_dispense_request flag for main loop
 * - No authentication required (local network only)
 * - Returns: "ok" on success
 * - Used for maintenance and testing
 *
 * **404 Handler**
 * - All other routes return 404 status
 * - Response: "404 - introuvable" (French for "not found")
 *
 * **Server Startup:**
 * 1. Register all route handlers (static, status, logs, dispense, 404)
 * 2. Call server.begin() to start listening on port 80
 * 3. Call init_time_ntp() to synchronize clock via NTP
 * 4. Print status messages to Serial console
 *
 * **JSON Response Examples:**
 *
 * ```json
 * // GET /status response:
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
 * // GET /logs?from=0 response:
 * {
 *   "total": 42,
 *   "from": 0,
 *   "entries": [
 *     "[Uptime 00:00] Dog Feeder",
 *     "[Uptime 00:02] Waiting for dog",
 *     "[Uptime 00:15] Dog detected",
 *     "[Uptime 00:15] Reading RFID",
 *     "[Uptime 00:16] Feeding Jop!"
 *   ]
 * }
 * ```
 *
 * @note
 * - Blocking WiFi connection: may take 5-10 seconds
 * - Blocks NTP sync: typically 1-5 seconds (called at end)
 * - LittleFS errors print warning but don't block startup
 * - Web requests handled in main loop via server.handleClient()
 * - All endpoints return Content-Type: application/json or text/plain
 * - POST /dispense uses critical section (noInterrupts/interrupts)
 *
 * @warning
 * - WiFi credentials are hardcoded (security consideration)
 * - Open network access (no authentication on web API)
 * - Assumes LittleFS contains /index.html
 *
 * @see init_time_ntp()
 */
void dispenser_lib::wifi_server::init_wifi_server()
{
    // ========================================================================
    // WiFi Connection
    // ========================================================================
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connexion WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(300);
        Serial.print(".");
    }
    Serial.println("\nConnecté !");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // ========================================================================
    // File System Initialization
    // ========================================================================
    if (!LittleFS.begin())
    {
        Serial.println("Erreur LittleFS !");
        return;
    }

    // ========================================================================
    // Static File Serving
    // ========================================================================
    server.serveStatic("/", LittleFS, "/index.html");

    // ========================================================================
    // REST API: GET /status
    // ========================================================================
    /**
     * @brief REST endpoint: GET /status
     *
     * Returns current system status and sensor readings as JSON.
     * Called periodically by web frontend to update display.
     */
    server.on("/status", HTTP_GET, []()
              {
        uint32_t up = millis();
        long dist = last_distance_cm;
        int pct = dispenser_lib::dispenser::food_percent_from_distance(dist);

        String json = "{";
        json += "\"event\":\"" + last_event + "\",";
        json += "\"distance_cm\":" + String(dist) + ",";
        json += "\"food_percent\":" + String(pct) + ",";
        json += "\"last_rfid\":\"" + last_rfid + "\",";
        json += "\"uptime_ms\":" + String(up) + ",";
        json += "\"jop_last_fed_uptime_ms\":" + String(jop_last_fed_ms) + ",";
        json += "\"manouk_last_fed_uptime_ms\":" + String(manouk_last_fed_ms);
        json += "}";
        server.send(200, "application/json; charset=utf-8", json); });

    // ========================================================================
    // REST API: GET /logs?from=N
    // ========================================================================
    /**
     * @brief REST endpoint: GET /logs?from=N
     *
     * Returns event log entries as JSON array.
     * Supports pagination via optional "from" query parameter.
     * Escapes special characters for JSON compatibility.
     */
    server.on("/logs", HTTP_GET, []()
              {
        // Get pagination start index from query parameter
        size_t from = 0;
        if (server.hasArg("from"))
        {
            long f = server.arg("from").toInt();
            if (f > 0) from = (size_t)f;
        }

        // Get log entries and validate pagination
        const std::vector<String>& v = app_log.entries();
        const size_t total = v.size();
        if (from > total) from = total;

        // Build JSON response
        String json = "{";
        json += "\"total\":" + String((unsigned long)total) + ",";
        json += "\"from\":" + String((unsigned long)from) + ",";
        json += "\"entries\":[";

        bool first = true;
        for (size_t i = from; i < total; ++i)
        {
            // Get entry and escape special characters for JSON
            String s = v[i];
            s.replace("\\", "\\\\");  
            s.replace("\"", "\\\"");  
            s.replace("\n", "\\n");   
            s.replace("\r", "\\r");   

            if (!first) json += ",";
            first = false;
            json += "\"" + s + "\"";
        }

        json += "]}";
        server.send(200, "application/json; charset=utf-8", json); });

    // ========================================================================
    // REST API: POST /dispense
    // ========================================================================
    /**
     * @brief REST endpoint: POST /dispense
     *
     * Manually trigger food dispensing via web interface.
     * Sets web_dispense_request flag for main loop processing.
     * No RFID checking or feeding interval enforcement.
     */
    server.on("/dispense", HTTP_POST, []()
              {
        noInterrupts();
        web_dispense_request = true;
        interrupts();
        server.send(200, "text/plain; charset=utf-8", "ok"); });

    // ========================================================================
    // REST API: 404 Not Found Handler
    // ========================================================================
    /**
     * @brief Default handler for undefined routes
     *
     * Returns 404 Not Found with French message.
     */
    server.onNotFound([]()
                      { server.send(404, "text/plain; charset=utf-8", "404 - introuvable"); });

    // ========================================================================
    // Start Server and Synchronize Time
    // ========================================================================
    server.begin();
    Serial.println("Serveur OK.");
    dispenser_lib::wifi_server::init_time_ntp();
}
