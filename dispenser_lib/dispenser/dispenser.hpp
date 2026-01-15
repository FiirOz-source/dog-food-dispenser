/**
 * @file  dispenser.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 2024
 * @brief Dispenser system header file - main control logic.
 *
 * This file contains the interface for the core dispenser functionality,
 * including dog detection, RFID identification, feeding logic, and system
 * status management.
 */

#ifndef DISPENSER_HPP
#define DISPENSER_HPP

#include <Arduino.h>
#include <actuators.hpp>
#include <sensors.hpp>
#include <dogs.hpp>
#include <logs.hpp>
#include <wifi_server.hpp>

// ============================================================================
// Hardware Pin Configuration
// ============================================================================

/** @brief I2C SDA pin for LCD display */
static const uint8_t SDA_PIN = 4;
/** @brief I2C SCL pin for LCD display */
static const uint8_t SCL_PIN = 5;
/** @brief GPIO pin for servo motor control */
static const uint8_t SERVO_PIN = 15;
/** @brief GPIO pin for ultrasonic distance sensor */
static const uint8_t ULTRASONIC_SENSOR_PIN = 12;
/** @brief GPIO pin for infrared motion detector */
static const uint8_t IR_PIN = 13;
/** @brief RX pin for RFID reader (software serial) */
static const uint8_t RFID_RX_PIN = 14;

// ============================================================================
// Global Objects - Dogs
// ============================================================================

/** @brief Jop - First dog with RFID tag "0080D552" */
extern dispenser_lib::dogs::dog Jop;
/** @brief Manouk - Second dog with RFID tag "002E2989" */
extern dispenser_lib::dogs::dog Manouk;

// ============================================================================
// Global Objects - Hardware Actuators
// ============================================================================

/** @brief LCD screen actuator (16x2 character display with RGB backlight) */
extern dispenser_lib::actuators::lcd_screen *lcd_screen;
/** @brief Servo motor actuator for food chute control */
extern dispenser_lib::actuators::servo_motor *servo_motor;

// ============================================================================
// Global Objects - Hardware Sensors
// ============================================================================

/** @brief Ultrasonic sensor for measuring food level */
extern dispenser_lib::sensors::ultrasonic_sensor ultrasonic_sensor;
/** @brief Infrared motion detector for dog presence detection */
extern dispenser_lib::sensors::infrared_sensor infrared_sensor;
/** @brief RFID reader for dog tag identification */
extern dispenser_lib::sensors::rfid_sensor rfid_sensor;

// ============================================================================
// Global Objects - Logging
// ============================================================================

/** @brief Application event logger with timestamps */
extern dispenser_lib::logs::logger app_log;

// ============================================================================
// Global State Flags - Interrupt Handling
// ============================================================================

/** @brief Flag set by IR sensor interrupt handler when motion detected */
extern volatile bool ir_event;
/** @brief Timestamp (in microseconds) of last IR sensor interrupt */
extern volatile uint32_t last_isr_us;
/** @brief IR sensor debounce threshold in microseconds (200ms) */
static const uint32_t IR_DEBOUNCE_US = 200000;

// ============================================================================
// Global State Variables - System Status
// ============================================================================

/** @brief Description of the last event that occurred in the system */
extern String last_event;
/** @brief RFID tag of the last dog detected */
extern String last_rfid;
/** @brief Distance in centimeters measured by ultrasonic sensor (-1 if error) */
extern long last_distance_cm;

/** @brief Uptime (in milliseconds) when Jop was last fed */
extern uint32_t jop_last_fed_ms;
/** @brief Uptime (in milliseconds) when Manouk was last fed */
extern uint32_t manouk_last_fed_ms;

// ============================================================================
// Thresholds and Constants
// ============================================================================

/** @brief Food empty threshold: if distance >= this, food dispenser is empty */
static const long FOOD_EMPTY_THRESHOLD_CM = 100;

// ============================================================================
// Global State Flags - Web API
// ============================================================================

/** @brief Flag set by web API /dispense endpoint to request manual feeding */
extern volatile bool web_dispense_request;

/**
 * @namespace dispenser_lib::dispenser
 * @brief Core dispenser control functions.
 *
 * This namespace contains all functions for managing the food dispenser
 * operation, including dog detection, RFID validation, food dispensing,
 * and system initialization.
 */
namespace dispenser_lib
{
    namespace dispenser
    {
        /**
         * @brief Display a message on the LCD and add to the application log.
         *
         * @param message C-string message to display and log
         * @param row LCD row index (0 or 1 for 2-line display)
         * @param column LCD column index (0-15 for 16-character display)
         * @throw std::invalid_argument if message is nullptr or row/column out of range
         */
        void lcd_print_and_log(const char *message, int row, int column);

        /**
         * @brief Display a message on the LCD and add to the application log.
         *
         * @param message String message to display and log
         * @param row LCD row index (0 or 1 for 2-line display)
         * @param column LCD column index (0-15 for 16-character display)
         * @throw std::invalid_argument if row/column out of range
         */
        void lcd_print_and_log(const String &message, int row, int column);

        /**
         * @brief Display an error message on LCD and log it.
         *
         * Shows a 2-line error message with 2-second display time.
         *
         * @param line1 First line of error message
         * @param line2 Second line of error message (typically exception message)
         * @return void
         *
         * @note Automatically updates @ref last_event with error description
         */
        void show_error(const char *line1, const char *line2);

        /**
         * @brief Convert ultrasonic distance to food percentage.
         *
         * Maps distance measurements to remaining food percentage:
         * - 0-10 cm: 100% (container full)
         * - 10-100 cm: linearly interpolated percentage
         * - 100+ cm: 0% (container empty)
         *
         * @param cm Distance in centimeters (-1 indicates error)
         * @return Food percentage (0-100) or -1 if input is negative
         */
        int food_percent_from_distance(long cm);

        /**
         * @brief Display the idle waiting screen on the LCD.
         *
         * Shows "Dog Feeder" title and "Waiting for dog" status message.
         * Updates last_event to "Waiting for dog".
         *
         * @return void
         * @throw std::exception if LCD display fails
         */
        void show_waiting_screen();

        /**
         * @brief Handle dog detection event from IR sensor.
         *
         * Process flow:
         * 1. Measure food level via ultrasonic sensor
         * 2. Read RFID tag from detected dog
         * 3. Check if dog is identified (Jop or Manouk)
         * 4. Verify feeding interval (12 hours minimum between feedings)
         * 5. Dispense food if conditions are met, or display appropriate message
         *
         * @return void
         *
         * @details Updates global variables:
         * - @ref last_distance_cm with ultrasonic measurement
         * - @ref last_rfid with RFID tag value
         * - @ref last_event with status message
         * - @ref jop_last_fed_ms / @ref manouk_last_fed_ms on successful feeding
         *
         * @see can_feed(), Jop, Manouk
         */
        void handle_dog_detected();

        /**
         * @brief Dispense food via web API request.
         *
         * Similar to handle_dog_detected() but:
         * - Does not require RFID identification
         * - Only checks food level
         * - Displays feeding message on LCD
         * - Does not update dog feeding timestamps
         *
         * @return void
         *
         * @details Used for manual feeding via web interface.
         * Updates @ref last_distance_cm and @ref last_event.
         */
        void web_dispense();

        /**
         * @brief Interrupt handler for IR sensor falling edge.
         *
         * Called when dog approaches (IR sensor detects motion).
         * Sets @ref ir_event flag and debounces multiple triggers.
         *
         * @return void
         *
         * @note Should be called from IRAM (interrupt RAM) context.
         * Implements 200ms debounce via @ref IR_DEBOUNCE_US.
         */
        void on_IR_falling();

        /**
         * @brief Initialize the entire dispenser system.
         *
         * Initializes:
         * - All sensors (ultrasonic, IR, RFID)
         * - All actuators (LCD screen, servo motor)
         * - WiFi connectivity
         * - Web server with API endpoints
         * - NTP time synchronization
         * - Interrupt handlers
         *
         * @return void
         *
         * @see dispenser_lib::actuators, dispenser_lib::sensors,
         *      dispenser_lib::wifi_server::init_wifi_server()
         */
        void init_dispenser();

    } // namespace dispenser
} // namespace dispenser_lib

#endif // DISPENSER_HPP