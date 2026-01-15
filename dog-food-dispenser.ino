/**
 * @file  dog-food-dispenser.ino
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 15/01/2026
 * @brief Main sketch for the automatic dog food dispenser system.
 *
 * This sketch implements the main control loop for an ESP8266-based automatic
 * dog food dispenser with RFID identification, ultrasonic level detection,
 * infrared motion sensing, and web server control capabilities.
 *
 * @details
 * The system manages:
 * - WiFi connectivity and web server for remote monitoring
 * - RFID tag recognition for dog identification (Jop and Manouk)
 * - Servo motor control for food dispensing
 * - Ultrasonic sensor for food level detection
 * - Infrared sensor for dog presence detection
 * - LCD display for user interface
 * - Feeding schedule enforcement (12-hour intervals per dog)
 * - Event logging with timestamps
 *
 * @see dispenser_lib::dispenser
 */

#include "dispenser.hpp"

#include <Arduino.h>
#include <time.h>
#include <stdexcept>

/**
 * @brief Arduino setup function - called once at startup.
 *
 * Initializes:
 * - Serial communication (115200 baud)
 * - All hardware components via dispenser initialization
 * - Sensors, actuators, WiFi, and web server
 *
 * @return void
 */
void setup()
{
    delay(100);
    Serial.begin(115200);
    delay(100);

    dispenser_lib::dispenser::init_dispenser();
}

/**
 * @brief Arduino main loop - called repeatedly after setup.
 *
 * Handles:
 * - Web server client requests
 * - Idle screen display when no activity
 * - IR motion sensor events for automatic dog detection and feeding
 * - Web API requests for manual food dispensing
 *
 * Uses state flags to prevent concurrent operations:
 * - @c busy: Prevents handling new events while processing current one
 * - @c waiting_msg: Tracks if idle message is currently displayed
 *
 * @return void
 *
 * @note All event flags are protected by interrupt disable/enable to ensure
 *       atomic read-modify-write operations in multi-threaded context.
 */
void loop()
{
    static bool busy = false;
    static bool waiting_msg = false;

    server.handleClient();

    if (!waiting_msg && !busy)
    {
        dispenser_lib::dispenser::show_waiting_screen();
        waiting_msg = true;
    }

    if (!busy)
    {
        if (web_dispense_request)
        {
            noInterrupts();
            web_dispense_request = false;
            interrupts();

            busy = true;
            waiting_msg = false;
            dispenser_lib::dispenser::web_dispense();
            busy = false;
        }
        else if (ir_event)
        {
            noInterrupts();
            ir_event = false;
            interrupts();

            busy = true;
            waiting_msg = false;
            dispenser_lib::dispenser::handle_dog_detected();
            busy = false;
        }
    }

    yield();
}