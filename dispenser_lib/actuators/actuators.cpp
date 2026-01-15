/**
 * @file  actuators.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 2024
 * @brief Actuators implementation - hardware output device control.
 *
 * This file contains the implementation for LCD display and servo motor
 * control, including I2C communication and GPIO PWM generation.
 */

#include "actuators.hpp"
#include <stdexcept>

// ============================================================================
// LCD Screen Implementation
// ============================================================================

/**
 * @brief LCD Screen Constructor.
 *
 * Initializes the LCD display configuration with I2C communication parameters.
 * Does not perform hardware initialization; call init_actuator() for that.
 *
 * @param sda I2C SDA (data) pin number for the display
 * @param scl I2C SCL (clock) pin number for the display
 * @param speed I2C clock frequency in Hz (default: 100000 = 100kHz)
 * @param columns Number of character columns on display (default: 16)
 * @param rows Number of character rows on display (default: 2)
 *
 * @throw std::invalid_argument if SDA or SCL pins are negative
 * @throw std::invalid_argument if columns or rows are not positive integers
 * @throw std::invalid_argument if I2C speed is zero
 *
 * @note
 * - Standard 2-line LCD: columns=16, rows=2
 * - Standard I2C frequencies: 100000 (100kHz) or 400000 (400kHz)
 * - Pin numbers are platform-dependent (ESP8266: GPIO 4=SDA, GPIO 5=SCL)
 */
dispenser_lib::actuators::lcd_screen::lcd_screen(int sda, int scl, unsigned long speed, int columns, int rows)
{
    if (sda < 0 || scl < 0)
    {
        throw std::invalid_argument("SDA and SCL pins must be non-negative");
    }
    if (columns <= 0 || rows <= 0)
    {
        throw std::invalid_argument("Columns and rows must be positive integers");
    }
    if (speed == 0)
    {
        throw std::invalid_argument("I2C speed cannot be zero");
    }

    sda_pin = sda;
    scl_pin = scl;
    i2c_speed = speed;
    nbr_columns = columns;
    nbr_rows = rows;
}

/**
 * @brief Initialize LCD hardware and I2C interface.
 *
 * Configures I2C communication and initializes the LCD module with
 * blue backlight color for visual feedback.
 *
 * @return void
 *
 * @details
 * - Initializes Wire (I2C) with configured SDA/SCL pins
 * - Sets I2C clock to configured speed
 * - Initializes LCD library with configured dimensions
 * - Sets backlight color to blue (R=0, G=128, B=255)
 * - Clears display
 *
 * @throw std::exception if I2C initialization fails
 *
 * @note Must be called before calling display_message() or clear()
 */
void dispenser_lib::actuators::lcd_screen::init_actuator()
{
    // Initialize I2C communication
    Wire.begin(sda_pin, scl_pin);
    Wire.setClock(i2c_speed);

    // Initialize the LCD
    lcd.begin(nbr_columns, nbr_rows);

    lcd.setRGB(0, 128, 255); // Set blue backlight

    lcd.clear();
}

/**
 * @brief Display a message at specified row and column on the LCD.
 *
 * Writes a text message to the LCD at the given position.
 * Text will be displayed starting at the specified coordinates.
 *
 * @param message C-string containing the message to display
 * @param row Row index on display (0-indexed: 0 is top row)
 * @param column Column index on display (0-indexed: 0 is leftmost)
 *
 * @return void
 *
 * @throw std::invalid_argument if message pointer is nullptr
 * @throw std::out_of_range if row index >= nbr_rows
 * @throw std::out_of_range if column index >= nbr_columns
 *
 * @note
 * - Overwrites existing text; does not auto-clear line
 * - Text wrapping depends on display behavior
 * - For 16x2 display: rows are 0-1, columns are 0-15
 *
 * Example:
 * @code
 * lcd_screen->display_message("Dog Feeder", 0, 0);  // Top-left
 * lcd_screen->display_message("Ready", 1, 0);       // Bottom-left
 * @endcode
 */
void dispenser_lib::actuators::lcd_screen::display_message(const char *message, int row, int column)
{
    if (message == nullptr)
    {
        throw std::invalid_argument("Message pointer cannot be null");
    }
    if (row < 0 || row >= nbr_rows)
    {
        throw std::out_of_range("Row index out of range");
    }
    if (column < 0 || column >= nbr_columns)
    {
        throw std::out_of_range("Column index out of range");
    }

    lcd.setCursor(column, row);
    lcd.print(message);
}

/**
 * @brief Clear the entire LCD display.
 *
 * Erases all characters from the display and resets cursor to (0,0).
 *
 * @return void
 *
 * @note Should be called before displaying a new screen to avoid text overlap
 */
void dispenser_lib::actuators::lcd_screen::clear()
{
    lcd.clear();
}

// ============================================================================
// Servo Motor Implementation
// ============================================================================

/**
 * @brief Servo Motor Constructor.
 *
 * Initializes servo configuration with control pin and position angles.
 * Does not perform hardware initialization; call init_actuator() for that.
 *
 * @param ctrl_pin GPIO PWM-capable pin connected to servo signal line
 * @param closed_angle Servo angle (0-180°) when food chute is closed (default: 0°)
 * @param opened_angle Servo angle (0-180°) when food chute is open (default: 90°)
 *
 * @throw std::invalid_argument if ctrl_pin is negative
 * @throw std::out_of_range if closed_angle is not in 0-180° range
 * @throw std::out_of_range if opened_angle is not in 0-180° range
 *
 * @details
 * - Angles are stored internally as 2x their value for library compatibility
 * - Initial state is set to 'C' (closed)
 * - Servo communication uses 50Hz PWM standard
 *
 * @note
 * - For ESP8266: GPIO pins 0, 2, 4, 5, 12, 13, 14, 15 support PWM
 * - Typical angles: closed=0°, open=90° or closed=30°, open=120° (adjustable per mechanical setup)
 */
dispenser_lib::actuators::servo_motor::servo_motor(int ctrl_pin, int closed_angle, int opened_angle)
{
    if (ctrl_pin < 0)
    {
        throw std::invalid_argument("Control pin must be non-negative");
    }
    if (closed_angle < 0 || closed_angle > 180)
    {
        throw std::out_of_range("Closed angle must be between 0 and 180 degrees");
    }
    if (opened_angle < 0 || opened_angle > 180)
    {
        throw std::out_of_range("Opened angle must be between 0 and 180 degrees");
    }

    pin = ctrl_pin;
    close_angle = 2 * closed_angle;
    open_angle = 2 * opened_angle;
    current_state = 'C'; // Start with closed position
}

/**
 * @brief Initialize servo hardware and move to closed position.
 *
 * Attaches the servo to the configured GPIO pin and commands it to the
 * closed position.
 *
 * @return void
 *
 * @details
 * - Calls servo.attach() with configured pin and default timing (1000-2000µs)
 * - Sets initial position to close_angle (closed/blocking)
 *
 * @throw std::exception if servo attachment fails
 *
 * @note
 * - Must be called before calling open(), close(), or toggle_position()
 * - Servo will immediately move to closed position after this call
 * - Delay servo after init to allow mechanical movement
 */
void dispenser_lib::actuators::servo_motor::init_actuator()
{
    servo.attach(pin);
    servo.write(close_angle);
}

/**
 * @brief Toggle servo position between open and closed states.
 *
 * Convenience method that switches the servo to the opposite position.
 * If currently open, moves to closed; if closed, moves to open.
 *
 * @return void
 *
 * @details
 * Checks current_state:
 * - If 'C' (closed): calls open()
 * - If 'O' (open): calls close()
 *
 * @note Used during initialization to test servo movement
 *
 * @see open(), close()
 */
void dispenser_lib::actuators::servo_motor::toggle_position()
{
    if (current_state == 'C')
    {
        open();
    }
    else
    {
        close();
    }
}

/**
 * @brief Move servo to open position (allows food dispensing).
 *
 * Commands the servo to the open angle, allowing food to flow from
 * the dispenser chute.
 *
 * @return void
 *
 * @details
 * - Calls servo.write(open_angle)
 * - Updates current_state to 'O'
 * - Motion is non-blocking (returns immediately)
 *
 * @note
 * - Mechanical movement takes time; add delay() after calling if needed
 * - Open angle is typically 90° but depends on physical servo setup
 * - Should be followed by close() after food has dispensed
 *
 * Example:
 * @code
 * servo_motor->open();
 * delay(2000);  // Allow 2 seconds for food to dispense
 * servo_motor->close();
 * @endcode
 *
 * @see close(), toggle_position()
 */
void dispenser_lib::actuators::servo_motor::open()
{
    servo.write(open_angle);
    current_state = 'O';
}

/**
 * @brief Move servo to closed position (blocks food dispensing).
 *
 * Commands the servo to the closed angle, blocking the dispenser chute
 * to prevent food flow.
 *
 * @return void
 *
 * @details
 * - Calls servo.write(close_angle)
 * - Updates current_state to 'C'
 * - Motion is non-blocking (returns immediately)
 *
 * @note
 * - Mechanical movement takes time; add delay() after calling if needed
 * - Closed angle is typically 0° but depends on physical servo setup
 * - Initial state on init_actuator() is always closed
 *
 * @see open(), toggle_position()
 */
void dispenser_lib::actuators::servo_motor::close()
{
    servo.write(close_angle);
    current_state = 'C';
}
