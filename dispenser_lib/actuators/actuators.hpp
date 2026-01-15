/**
 * @file  actuators.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 2024
 * @brief Actuators header file - hardware output control interfaces.
 *
 * This file defines base classes and concrete implementations for all
 * output actuators in the system: LCD display and servo motor.
 */

#ifndef ACTUATORS_HPP
#define ACTUATORS_HPP

#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>
#include <rgb_lcd.h>

/**
 * @namespace dispenser_lib::actuators
 * @brief Hardware actuator (output device) classes and interfaces.
 *
 * Provides abstraction layers for controlling:
 * - LCD display via I2C
 * - Servo motor for food chute control
 *
 * Base classes organize actuators by communication protocol.
 */
namespace dispenser_lib
{
    namespace actuators
    {
        /**
         * @class actuator
         * @brief Base class for all actuators.
         *
         * Abstract base class defining the common interface for all
         * actuators (output devices).
         */
        class actuator
        {
        public:
            /// Default constructor
            actuator() = default;
            /// Default destructor
            ~actuator() = default;

            /**
             * @brief Initialize the actuator hardware.
             *
             * Virtual method that should be overridden by derived classes
             * to configure pins, protocols, and hardware state.
             *
             * @return void
             */
            virtual void init_actuator();
        };

        /**
         * @class digital_actuator
         * @brief Base class for GPIO-based actuators.
         *
         * Actuators that use standard GPIO pins for control.
         * Extends @ref actuator base class.
         */
        class digital_actuator : public actuator
        {
        public:
            /// Default constructor
            digital_actuator() = default;
            /// Default destructor
            ~digital_actuator() = default;

        protected:
            /// GPIO control pin number
            int ctrl_pin;
        };

        /**
         * @class i2c_actuator
         * @brief Base class for I2C-based actuators.
         *
         * Actuators that communicate via I2C (TWI) protocol.
         * Extends @ref actuator base class.
         *
         * Handles I2C initialization and configuration for derived classes.
         */
        class i2c_actuator : public actuator
        {
        public:
            /// Default constructor
            i2c_actuator() = default;
            /// Default destructor
            ~i2c_actuator() = default;

            /**
             * @brief Initialize I2C communication.
             *
             * Configures Wire library with SDA/SCL pins and clock speed.
             *
             * @return void
             */
            virtual void init_actuator() override;

        protected:
            /// I2C SDA (data) pin number
            int sda_pin;
            /// I2C SCL (clock) pin number
            int scl_pin;
            /// I2C clock frequency in Hz (typically 100000 or 400000)
            unsigned long i2c_speed;
        };

        /**
         * @class lcd_screen
         * @brief RGB LCD display controlled via I2C.
         *
         * 16x2 character LCD with RGB backlight connected via I2C interface.
         * Uses Grove-compatible RGB LCD module with address 0x7C.
         *
         * Extends @ref i2c_actuator for I2C communication.
         *
         * @see rgb_lcd library
         */
        class lcd_screen : public i2c_actuator
        {
        public:
            /// Default constructor
            lcd_screen() = default;

            /**
             * @brief Constructor with full configuration.
             *
             * @param sda I2C SDA (data) pin number
             * @param scl I2C SCL (clock) pin number
             * @param speed I2C clock frequency in Hz (default: 100000)
             * @param columns Number of character columns (default: 16)
             * @param rows Number of character rows (default: 2)
             *
             * @throw std::invalid_argument if pins are negative or dimensions are invalid
             * @throw std::invalid_argument if I2C speed is zero
             */
            lcd_screen(int sda, int scl, unsigned long speed = 100000, int columns = 16, int rows = 2);

            /// Default destructor
            ~lcd_screen() = default;

            /**
             * @brief Initialize LCD hardware and I2C communication.
             *
             * Configures I2C interface and initializes LCD display.
             * Sets backlight color to blue (0, 128, 255).
             *
             * @return void
             * @throw std::exception if initialization fails
             *
             * @details
             * - Calls Wire.begin() with configured SDA/SCL pins
             * - Sets I2C clock speed
             * - Initializes LCD library
             * - Clears display and sets blue backlight
             */
            virtual void init_actuator();

            /**
             * @brief Display a message at specified position on LCD.
             *
             * Writes a C-string to LCD at the given row and column position.
             * Text will wrap or be truncated depending on remaining space.
             *
             * @param message C-string message to display (must not be nullptr)
             * @param row LCD row index (0-indexed, 0 or 1 for 2-row display)
             * @param column LCD column index (0-indexed, 0-15 for 16-column display)
             *
             * @return void
             *
             * @throw std::invalid_argument if message is nullptr
             * @throw std::out_of_range if row or column is out of valid range
             *
             * @details
             * - Position (0,0) is top-left corner
             * - Message can be partial (shorter than column space remaining)
             * - Existing text is not cleared before writing
             *
             * Example:
             * @code
             * lcd_screen->display_message("Dog Feeder", 0, 0);
             * lcd_screen->display_message("Ready", 1, 0);
             * @endcode
             */
            void display_message(const char *message, int row, int column);

            /**
             * @brief Clear the entire LCD display.
             *
             * Erases all text from the display and resets cursor to (0,0).
             *
             * @return void
             */
            void clear();

        private:
            /// RGB LCD library instance
            rgb_lcd lcd;
            /// Number of character columns (typically 16)
            int nbr_columns;
            /// Number of character rows (typically 2)
            int nbr_rows;
        };

        /**
         * @class servo_motor
         * @brief Servo motor controller for food chute.
         *
         * Standard position servo motor for opening/closing food dispenser chute.
         * Supports configurable open and close angles (0-180 degrees).
         *
         * Extends @ref digital_actuator for GPIO control.
         *
         * @see Servo library
         */
        class servo_motor : public digital_actuator
        {
        public:
            /// Default constructor
            servo_motor() = default;

            /**
             * @brief Constructor with pin and angle configuration.
             *
             * @param ctrl_pin GPIO PWM-capable pin for servo control
             * @param closed_angle Servo angle when chute is closed (default: 0°)
             * @param opened_angle Servo angle when chute is open (default: 90°)
             *
             * @throw std::invalid_argument if pin is negative
             * @throw std::out_of_range if angles are not in 0-180° range
             *
             * @note
             * - Angles are internally doubled (multiplied by 2) for compatibility with Servo library
             * - Servo positions are mapped: 0-180° range maps to 0-360 in library
             */
            servo_motor(int ctrl_pin, int closed_angle = 0, int opened_angle = 90);

            /// Default destructor
            ~servo_motor() = default;

            /**
             * @brief Initialize servo hardware and set to closed position.
             *
             * Attaches servo to configured GPIO pin and moves to closed position.
             *
             * @return void
             * @throw std::exception if servo attachment fails
             *
             * @note Must be called before calling open(), close(), or toggle_position()
             */
            virtual void init_actuator() override;

            /**
             * @brief Toggle servo position between open and closed.
             *
             * Convenience method that switches position based on current state.
             * If currently open, moves to close position and vice versa.
             *
             * @return void
             *
             * @see open(), close()
             */
            void toggle_position();

            /**
             * @brief Move servo to open position (food dispensing).
             *
             * Opens the food chute to allow food to dispense.
             * Updates current_state to 'O' (open).
             *
             * @return void
             *
             * @note Open angle is typically 90°, but depends on mechanical setup
             */
            void open();

            /**
             * @brief Move servo to closed position (blocking food flow).
             *
             * Closes the food chute to prevent dispensing.
             * Updates current_state to 'C' (closed).
             *
             * @return void
             *
             * @note Closed angle is typically 0°, but depends on mechanical setup
             */
            void close();

        private:
            /// GPIO PWM pin number
            int pin;
            /// Servo angle when closed (stored as 2x actual angle)
            int close_angle;
            /// Servo angle when open (stored as 2x actual angle)
            int open_angle;
            /// Servo library instance
            Servo servo;
            /// Current servo state: 'O' for open, 'C' for closed
            char current_state;
        };

    } // namespace actuators
} // namespace dispenser_lib

#endif // ACTUATORS_HPP