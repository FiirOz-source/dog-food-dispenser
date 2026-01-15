/**
 * @file  dispenser.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 2024
 * @brief Dispenser system implementation - main control logic.
 *
 * This file contains the implementation of the core food dispenser functionality,
 * including dog detection via IR sensor, RFID identification, feeding schedules,
 * food level monitoring, and both automatic and web-based food dispensing.
 */

#include "dispenser.hpp"

// ============================================================================
// Global Objects - Dogs
// ============================================================================

/** Dog "Jop" with RFID tag 0080D552 */
dispenser_lib::dogs::dog Jop("Jop", "0080D552");
/** Dog "Manouk" with RFID tag 002E2989 */
dispenser_lib::dogs::dog Manouk("Manouk", "002E2989");

// ============================================================================
// Global Objects - Hardware Actuators
// ============================================================================

/** Pointer to the LCD screen actuator (allocated dynamically in init_dispenser) */
dispenser_lib::actuators::lcd_screen *lcd_screen = nullptr;
/** Pointer to the servo motor actuator (allocated dynamically in init_dispenser) */
dispenser_lib::actuators::servo_motor *servo_motor = nullptr;

// ============================================================================
// Global Objects - Hardware Sensors
// ============================================================================

/** Ultrasonic distance sensor for measuring food level */
dispenser_lib::sensors::ultrasonic_sensor ultrasonic_sensor(ULTRASONIC_SENSOR_PIN);
/** Infrared motion detector for dog presence detection */
dispenser_lib::sensors::infrared_sensor infrared_sensor(IR_PIN);
/** RFID reader for dog tag identification (RX pin, no TX, 9600 baud) */
dispenser_lib::sensors::rfid_sensor rfid_sensor(RFID_RX_PIN, -1, 9600);

// ============================================================================
// Global Objects - Logging
// ============================================================================

/** Application event logger with automatic timestamps */
dispenser_lib::logs::logger app_log;

// ============================================================================
// Global State Variables - Interrupt Handling
// ============================================================================

/** IR interrupt event flag - set to true when motion detected */
volatile bool ir_event = false;
/** Timestamp of last IR interrupt in microseconds (for debouncing) */
volatile uint32_t last_isr_us = 0;

// ============================================================================
// Global State Variables - System Status
// ============================================================================

/** Description of the last event that occurred in the system */
String last_event = "Boot";
/** RFID tag value of the last dog detected (empty string if none) */
String last_rfid = "";
/** Last measured food level in centimeters (-1 if sensor error) */
long last_distance_cm = -1;

/** Uptime (ms) when Jop was last fed (used for 12-hour interval tracking) */
uint32_t jop_last_fed_ms = 0;
/** Uptime (ms) when Manouk was last fed (used for 12-hour interval tracking) */
uint32_t manouk_last_fed_ms = 0;

// ============================================================================
// Global State Flags - Web API
// ============================================================================

/** Web API /dispense endpoint request flag (set by WiFi server) */
volatile bool web_dispense_request = false;

// ============================================================================
// Forward Declaration for Interrupt Handler
// ============================================================================

/**
 * @brief Wrapper function for IR interrupt handler.
 *
 * Required by Arduino interrupt system. Calls the actual handler function
 * in the dispenser namespace.
 */
void IRAM_ATTR on_IR_falling_wrapper();

// ============================================================================
// Function Implementations
// ============================================================================

/**
 * @brief Display a message on the LCD and log it to the application log.
 *
 * Convenience function that outputs a message to both the LCD display and
 * the application event log.
 *
 * @param message C-string message to display (const char*)
 * @param row LCD row index (0 or 1 for 2-row display)
 * @param column LCD column index (0-15 for 16-column display)
 *
 * @return void
 *
 * @details
 * - Message is displayed at (row, column) on the LCD
 * - Message is appended to the application log with timestamp
 * - Does nothing if lcd_screen pointer is nullptr (graceful degradation)
 *
 * @see dispenser_lib::actuators::lcd_screen::display_message()
 * @see dispenser_lib::logs::logger::operator+=()
 */
void dispenser_lib::dispenser::lcd_print_and_log(const char *message, int row, int column)
{
    if (lcd_screen)
    {
        lcd_screen->display_message(message, row, column);
    }
    app_log += String(message);
}

/**
 * @brief Display a message on the LCD and log it to the application log.
 *
 * Convenience function that outputs a message to both the LCD display and
 * the application event log. Overload for String objects.
 *
 * @param message String message to display
 * @param row LCD row index (0 or 1 for 2-row display)
 * @param column LCD column index (0-15 for 16-column display)
 *
 * @return void
 *
 * @see lcd_print_and_log(const char*, int, int)
 */
void dispenser_lib::dispenser::lcd_print_and_log(const String &message, int row, int column)
{
    if (lcd_screen)
    {
        lcd_screen->display_message(message.c_str(), row, column);
    }
    app_log += message;
}

/**
 * @brief Display an error message on the LCD and serial console.
 *
 * Shows a 2-line error message for 2 seconds and logs the error. Typically used
 * for exception messages and hardware failures.
 *
 * @param line1 First line of error message (main error description)
 * @param line2 Second line of error message (usually exception message)
 *
 * @return void
 *
 * @details
 * Output:
 * - Prints "[ERROR] line1 | line2" to Serial console
 * - Updates last_event to "ERR: line1"
 * - Clears LCD display and shows both lines
 * - Delays 2000ms for user to read message
 *
 * Example: show_error("Servo Error", "Invalid pin number")
 *
 * @see last_event
 */
void dispenser_lib::dispenser::show_error(const char *line1, const char *line2)
{
    Serial.print("[ERROR] ");
    Serial.print(line1);
    Serial.print(" | ");
    Serial.println(line2);

    last_event = String("ERR: ") + line1;
    if (lcd_screen)
    {
        lcd_screen->clear();
    }
    dispenser_lib::dispenser::lcd_print_and_log(line1, 0, 0);
    dispenser_lib::dispenser::lcd_print_and_log(line2, 1, 0);
    delay(2000);
}

/**
 * @brief Convert ultrasonic distance measurement to food percentage.
 *
 * Maps physical distance from ultrasonic sensor to estimated remaining food
 * percentage in the dispenser container.
 *
 * @param cm Distance in centimeters measured by ultrasonic sensor
 *
 * @return Food percentage (0-100):
 *         - -1 if input cm < 0 (sensor error)
 *         - 100% if cm <= 10 (container full)
 *         -  0% if cm >= 100 (container empty)
 *         - Linearly interpolated percentage for 10 < cm < 100
 *
 * @details
 * Calibration parameters:
 * - full_cm = 10: Distance when container is full
 * - empty_cm = 100: Distance when container is empty
 *
 * Formula: pct = 100 * (empty_cm - cm) / (empty_cm - full_cm)
 *
 * Example:
 * - food_percent_from_distance(5) → 100 (full)
 * - food_percent_from_distance(55) → ~50 (half)
 * - food_percent_from_distance(105) → 0 (empty)
 *
 * @note Results are clamped to [0, 100] range to handle edge cases
 */
int dispenser_lib::dispenser::food_percent_from_distance(long cm)
{
    const int full_cm = 10;
    const int empty_cm = 100;

    if (cm < 0)
    {
        return -1;
    }
    if (cm <= full_cm)
    {
        return 100;
    }
    if (cm >= empty_cm)
    {
        return 0;
    }

    float pct = 100.0f * (float)(empty_cm - cm) / (float)(empty_cm - full_cm);
    if (pct < 0)
    {
        pct = 0;
    }
    if (pct > 100)
    {
        pct = 100;
    }
    return (int)(pct + 0.5f);
}

/**
 * @brief Display the idle "waiting for dog" screen on the LCD.
 *
 * Shows the system is ready and waiting for a dog to approach the feeder.
 * Should be called repeatedly in the main loop when the system is idle.
 *
 * @return void
 *
 * @details
 * Displays:
 * - Line 0: "Dog Feeder"
 * - Line 1: "Waiting for dog"
 *
 * Updates:
 * - last_event to "Waiting for dog"
 *
 * @throw std::exception if LCD display fails
 *
 * @see last_event
 */
void dispenser_lib::dispenser::show_waiting_screen()
{
    try
    {
        if (lcd_screen)
        {
            lcd_screen->clear();
            lcd_screen->display_message("Dog Feeder", 0, 0);
            lcd_screen->display_message("Waiting for dog", 1, 0);
        }
        app_log += "Waiting for dog";
    }
    catch (const std::exception &e)
    {
        Serial.print("[ERROR] Display error: ");
        Serial.println(e.what());
    }
    last_event = "Waiting for dog";
}

/**
 * @brief Handle dog detection event from infrared sensor.
 *
 * This is the main automatic feeding routine triggered by the IR motion sensor.
 * It identifies the dog via RFID tag, checks feeding schedule constraints,
 * and dispenses food if all conditions are met.
 *
 * @return void
 *
 * @details
 * Process flow:
 *
 * 1. **Measure Food Level**
 *    - Get distance from ultrasonic sensor
 *    - Convert to centimeters and save to last_distance_cm
 *
 * 2. **Check if Food Available**
 *    - If distance >= FOOD_EMPTY_THRESHOLD_CM (100cm), container is empty
 *    - Display "No more food" and return
 *
 * 3. **Read RFID Tag**
 *    - Attempt to read RFID tag (100ms timeout)
 *    - Save to last_rfid
 *
 * 4. **Identify Dog**
 *    - Check if tag matches Jop (0080D552) or Manouk (002E2989)
 *
 * 5. **Check Feeding Interval**
 *    - Each dog has a 12-hour minimum interval between feedings
 *    - If interval not met, display "already fed" message with time since last feeding
 *    - Otherwise proceed to dispense food
 *
 * 6. **Dispense Food**
 *    - Display "Feeding [DogName]" on LCD
 *    - Update dog's last_fed timestamp
 *    - Open servo motor for 2 seconds (dispensing food)
 *    - Close servo motor
 *    - Log the feeding event
 *
 * Global Variables Updated:
 * - last_event: Status description
 * - last_rfid: RFID tag of detected dog
 * - last_distance_cm: Measured food level
 * - jop_last_fed_ms / manouk_last_fed_ms: Uptime when fed (if feeding occurred)
 *
 * @note
 * - All IR sensor status is handled by ir_event flag in main loop()
 * - RFID read may timeout silently (empty string returned)
 * - Unknown RFID tags are logged and displayed
 * - All exceptions are caught and logged gracefully
 *
 * @see Jop, Manouk, dispenser_lib::dogs::dog::can_feed()
 */
void dispenser_lib::dispenser::handle_dog_detected()
{
    last_event = "Dog detected";

    long dist_mm = ultrasonic_sensor.get_distance();
    last_distance_cm = dist_mm / 10;

    if (last_distance_cm < FOOD_EMPTY_THRESHOLD_CM)
    {
        last_event = "Reading RFID";

        last_rfid = rfid_sensor.read_rfid(100);

        bool jop_matched = Jop.matches_tag(last_rfid);
        bool manouk_matched = Manouk.matches_tag(last_rfid);

        if (jop_matched || manouk_matched)
        {
            if (jop_matched)
            {
                if (Jop.can_feed())
                {
                    Jop.mark_fed();
                    jop_last_fed_ms = millis();
                    last_event = "Feeding Jop";

                    try
                    {
                        if (lcd_screen)
                        {
                            lcd_screen->clear();
                            lcd_screen->display_message("Feeding Jop!", 0, 0);
                        }
                        app_log += "Feeding Jop!";
                        servo_motor->open();
                        delay(2000);
                        servo_motor->close();
                    }
                    catch (const std::exception &e)
                    {
                        show_error("Servo Error", e.what());
                    }
                }
                else
                {
                    last_event = "Jop already fed";

                    try
                    {
                        if (lcd_screen)
                        {
                            lcd_screen->clear();
                            lcd_screen->display_message("Jop already fed", 0, 0);
                            String since = Jop.since_fed();
                            lcd_screen->display_message(since.c_str(), 1, 0);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        Serial.print("[ERROR] Display error: ");
                        Serial.println(e.what());
                    }
                    delay(2000);
                }
            }
            else
            {
                if (Manouk.can_feed())
                {
                    Manouk.mark_fed();
                    manouk_last_fed_ms = millis();
                    last_event = "Feeding Manouk";

                    try
                    {
                        if (lcd_screen)
                        {
                            lcd_screen->clear();
                            lcd_screen->display_message("Feeding Manouk!", 0, 0);
                        }
                        app_log += "Feeding Manouk!";
                        servo_motor->open();
                        delay(2000);
                        servo_motor->close();
                    }
                    catch (const std::exception &e)
                    {
                        show_error("Servo Error", e.what());
                    }
                }
                else
                {
                    last_event = "Manouk already fed";

                    try
                    {
                        if (lcd_screen)
                        {
                            lcd_screen->clear();
                            lcd_screen->display_message("Manouk alrdy fed", 0, 0);
                            String since = Manouk.since_fed();
                            lcd_screen->display_message(since.c_str(), 1, 0);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        Serial.print("[ERROR] Display error: ");
                        Serial.println(e.what());
                    }
                    delay(2000);
                }
            }
        }
        else
        {
            if (last_rfid != "")
            {
                last_event = "Unknown dog";

                try
                {
                    if (lcd_screen)
                    {
                        lcd_screen->clear();
                        lcd_screen->display_message("Unknown dog...", 0, 0);
                        char buf[32];
                        snprintf(buf, sizeof(buf), "Tag: %s", last_rfid.c_str());
                        lcd_screen->display_message(buf, 1, 0);
                    }
                }
                catch (const std::exception &e)
                {
                    Serial.print("[ERROR] Display error: ");
                    Serial.println(e.what());
                }
                delay(2000);
            }
            else
            {
                last_event = "RFID empty";
            }
        }
    }
    else
    {
        last_event = "No more food";
        try
        {
            if (lcd_screen)
            {
                lcd_screen->clear();
                lcd_screen->display_message("No more food ...", 0, 0);
            }
        }
        catch (const std::exception &e)
        {
            Serial.print("[ERROR] Display error: ");
            Serial.println(e.what());
        }
        delay(500);
    }
}

/**
 * @brief Dispense food via web API request (manual dispensing).
 *
 * Similar to handle_dog_detected() but without RFID identification requirement.
 * Used for manual feeding operations triggered via the web server /dispense endpoint.
 *
 * @return void
 *
 * @details
 * Process flow:
 *
 * 1. **Check Food Level**
 *    - Measure food level via ultrasonic sensor
 *    - If empty (distance >= 100cm), display error and return
 *
 * 2. **Dispense Food**
 *    - Display "Web dispense" and "Dispensing..." on LCD
 *    - Log the web dispense event
 *    - Open servo motor for 2 seconds
 *    - Close servo motor
 *
 * Global Variables Updated:
 * - last_distance_cm: Measured food level
 * - last_event: "Web dispense done" on completion, or error message on failure
 *
 * Differences from handle_dog_detected():
 * - No RFID identification
 * - No dog-specific feeding interval checking
 * - No update to jop_last_fed_ms or manouk_last_fed_ms
 * - For maintenance/manual testing purposes
 *
 * @see handle_dog_detected()
 */
void dispenser_lib::dispenser::web_dispense()
{
    long dist_mm = ultrasonic_sensor.get_distance();
    last_distance_cm = dist_mm / 10;

    if (last_distance_cm >= FOOD_EMPTY_THRESHOLD_CM)
    {
        last_event = "Web dispense blocked (empty)";
        try
        {
            if (lcd_screen)
            {
                lcd_screen->clear();
                lcd_screen->display_message("No more food ...", 0, 0);
            }
        }
        catch (const std::exception &e)
        {
            Serial.print("[ERROR] Display error: ");
            Serial.println(e.what());
        }
        delay(500);
        return;
    }

    last_event = "Web dispense";
    try
    {
        if (lcd_screen)
        {
            lcd_screen->clear();
            lcd_screen->display_message("Web dispense", 0, 0);
            lcd_screen->display_message("Dispensing...", 1, 0);
        }
        app_log += "Web dispense";
        servo_motor->open();
        delay(2000);
        servo_motor->close();
    }
    catch (const std::exception &e)
    {
        show_error("Servo Error", e.what());
    }

    last_event = "Web dispense done";
}

/**
 * @brief Interrupt handler wrapper for IR sensor falling edge.
 *
 * This wrapper function is required by Arduino's interrupt system.
 * It delegates to the actual interrupt handler in the dispenser namespace.
 *
 * @return void
 *
 * @note Must be in IRAM (interrupt RAM) for ESP8266/ESP32.
 *
 * @see on_IR_falling()
 */
void IRAM_ATTR on_IR_falling_wrapper()
{
    dispenser_lib::dispenser::on_IR_falling();
}

/**
 * @brief Interrupt handler for IR sensor falling edge (motion detected).
 *
 * Called when the IR motion sensor detects a dog approaching the feeder.
 * Sets the ir_event flag for processing in the main loop.
 *
 * @return void
 *
 * @details
 * - Debounces multiple interrupts within 200ms window (IR_DEBOUNCE_US)
 * - Records timestamp in last_isr_us for debounce calculation
 * - Sets ir_event flag to trigger handle_dog_detected() in main loop
 *
 * Debounce logic:
 * If (now - last_isr_us) < 200000 microseconds (200ms), return early
 * This prevents false triggers from noisy sensor or bouncing contacts
 *
 * @note
 * - Runs in interrupt context (IRAM)
 * - Should be kept very short for low latency
 * - Uses micros() for precise timing
 * - Operates on volatile variables (ir_event, last_isr_us)
 *
 * @see IR_DEBOUNCE_US, ir_event, last_isr_us
 */
void IRAM_ATTR dispenser_lib::dispenser::on_IR_falling()
{
    uint32_t now = micros();
    if (now - last_isr_us < IR_DEBOUNCE_US)
        return;
    last_isr_us = now;
    ir_event = true;
}

/**
 * @brief Initialize the entire food dispenser system.
 *
 * Performs hardware initialization, configuration, and system startup.
 * This function should be called once from Arduino setup().
 *
 * @return void
 *
 * @details
 * Initialization sequence (with LCD progress feedback):
 *
 * 1. **LCD Screen**
 *    - Allocate and initialize LCD via I2C (SDA=4, SCL=5)
 *    - Display progress bar
 *    - Fatal error if initialization fails (hangs in infinite loop)
 *
 * 2. **Servo Motor**
 *    - Allocate and initialize servo on pin SERVO_PIN (15)
 *    - Test servo by toggling position (open/close)
 *    - Fatal error if initialization fails
 *
 * 3. **Ultrasonic Sensor**
 *    - Initialize distance measurement sensor on ULTRASONIC_SENSOR_PIN (12)
 *
 * 4. **Infrared Sensor**
 *    - Initialize motion detector on IR_PIN (13)
 *    - Set pin mode to INPUT_PULLUP
 *
 * 5. **RFID Sensor**
 *    - Initialize RFID reader on RFID_RX_PIN (14), 9600 baud
 *
 * 6. **System Ready**
 *    - Display "Init Complete" on LCD
 *    - Attach interrupt handler to IR sensor
 *    - Show idle "Waiting for dog" screen
 *
 * 7. **WiFi & Web Server**
 *    - Initialize WiFi connection to "iPhone de Timothée"
 *    - Start ESP8266 web server on port 80
 *    - Initialize NTP time synchronization (for timestamps)
 *
 * Global Variables Modified:
 * - lcd_screen: Allocated and initialized
 * - servo_motor: Allocated and initialized
 * - All sensors: Initialized
 * - IR interrupt handler: Attached
 *
 * @throw Catches and logs exceptions from hardware components
 *
 * @note
 * - Total initialization time: ~3-5 seconds (includes delays for user feedback)
 * - If LCD init fails: prints to Serial, hangs in infinite loop
 * - If Servo init fails: prints to Serial, hangs in infinite loop
 * - Other component failures are logged and ignored
 *
 * @see dispenser_lib::wifi_server::init_wifi_server()
 * @see SDA_PIN, SCL_PIN, SERVO_PIN, ULTRASONIC_SENSOR_PIN, IR_PIN, RFID_RX_PIN
 */
void dispenser_lib::dispenser::init_dispenser()
{
    try
    {
        lcd_screen = new dispenser_lib::actuators::lcd_screen(SDA_PIN, SCL_PIN);
        lcd_screen->init_actuator();
        lcd_screen->display_message("Dog Feeder", 0, 0);
    }
    catch (const std::exception &e)
    {
        Serial.print("[FATAL] LCD initialization failed: ");
        Serial.println(e.what());
        while (1)
            delay(1000);
    }

    static int init = 0;
    char buf[32];

    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen->display_message(buf, 1, 0);
    app_log += String(buf);
    init = 22;
    delay(500);

    try
    {
        servo_motor = new dispenser_lib::actuators::servo_motor(SERVO_PIN);
        servo_motor->init_actuator();
        snprintf(buf, sizeof(buf), "Init %d%%", init);
        lcd_screen->display_message(buf, 1, 0);
        app_log += String(buf);
        init += 23;
        delay(500);

        servo_motor->toggle_position();
        delay(300);
        servo_motor->toggle_position();
    }
    catch (const std::exception &e)
    {
        Serial.print("[FATAL] Servo initialization failed: ");
        Serial.println(e.what());
        if (lcd_screen)
        {
            lcd_screen->clear();
            lcd_screen->display_message("Servo Error", 0, 0);
        }
        while (1)
            delay(1000);
    }

    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen->display_message(buf, 1, 0);
    app_log += String(buf);
    init += 22;
    delay(500);

    ultrasonic_sensor.init_sensor();
    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen->display_message(buf, 1, 0);
    app_log += String(buf);
    init += 24;
    delay(500);

    infrared_sensor.init_sensor();
    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen->display_message(buf, 1, 0);
    app_log += String(buf);

    rfid_sensor.init_sensor();
    init = 100;
    delay(500);
    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen->display_message(buf, 1, 0);
    app_log += String(buf);
    delay(500);
    lcd_screen->display_message("Init Complete", 1, 0);
    app_log += "Init Complete";

    pinMode(IR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(IR_PIN), on_IR_falling_wrapper, FALLING);

    show_waiting_screen();

    dispenser_lib::wifi_server::init_wifi_server();
}