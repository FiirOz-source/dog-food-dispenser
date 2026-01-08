/*********************************************************************
 * @file  actuators.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Actuators source file
 *********************************************************************/

#include "actuators.hpp"
#include <stdexcept>

/* LCD Screen */
dispenser_lib::actuators::lcd_screen::lcd_screen(int sda, int scl, unsigned long speed, int columns, int rows)
{
    if (sda < 0 || scl < 0)
    {
        throw dispenser_lib::actuators::invalid_pin("lcd_screen: pin SDA/SCL invalide");
    }
    if (columns <= 0 || rows <= 0)
    {
        throw dispenser_lib::actuators::invalid_param("lcd_screen: dimensions LCD invalides");
    }
    if (speed == 0)
    {
        throw dispenser_lib::actuators::invalid_param("lcd_screen: i2c_speed invalide");
    }

    sda_pin = sda;
    scl_pin = scl;
    i2c_speed = speed;
    nbr_columns = columns;
    nbr_rows = rows;
}

void dispenser_lib::actuators::lcd_screen::init_actuator()
{
    // Initialize I2C communication
    Wire.begin(sda_pin, scl_pin);
    Wire.setClock(i2c_speed);

    // Initialize the LCD
    lcd.begin(nbr_columns, nbr_rows);

    lcd.setRGB(0, 128, 255); // Set blue backlight
    lcd.clear();

    initialized = true;
}

void dispenser_lib::actuators::lcd_screen::display_message(const char *message, int row, int column)
{
    if (!initialized)
    {
        throw dispenser_lib::actuators::not_initialized("lcd_screen: non initialise");
    }
    if (message == nullptr)
    {
        throw dispenser_lib::actuators::invalid_param("lcd_screen: message nul");
    }
    if (row < 0 || row >= nbr_rows || column < 0 || column >= nbr_columns)
    {
        throw dispenser_lib::actuators::invalid_param("lcd_screen: coordonnees hors ecran");
    }

    lcd.setCursor(column, row);
    lcd.print(message);
}

void dispenser_lib::actuators::lcd_screen::clear()
{
    if (!initialized)
    {
        throw dispenser_lib::actuators::not_initialized("lcd_screen: non initialise");
    }
    lcd.clear();
}

/* Servo Motor */
dispenser_lib::actuators::servo_motor::servo_motor(int ctrl_pin, int closed_angle, int opened_angle)
{
    if (ctrl_pin < 0)
    {
        throw dispenser_lib::actuators::invalid_pin("servo_motor: pin invalide");
    }
    if (closed_angle < 0 || closed_angle > 180 || opened_angle < 0 || opened_angle > 180)
    {
        throw dispenser_lib::actuators::invalid_param("servo_motor: angles doivent etre entre 0 et 180");
    }

    pin = ctrl_pin;

    close_angle = 2 * closed_angle;
    open_angle = 2 * opened_angle;

    if (close_angle < 0)
    {
        close_angle = 0;
    }
    if (open_angle < 0)
    {
        open_angle = 0;
    }
    if (close_angle > 180)
    {
        close_angle = 180;
    }
    if (open_angle > 180)
    {
        open_angle = 180;
    }

    current_state = 'C'; // Start with closed position
}

void dispenser_lib::actuators::servo_motor::init_actuator()
{
    servo.attach(pin);
    servo.write(close_angle);
    initialized = true;
}

void dispenser_lib::actuators::servo_motor::toggle_position()
{
    if (!initialized)
    {
        throw dispenser_lib::actuators::not_initialized("servo_motor: non initialise");
    }

    if (current_state == 'C')
    {
        open();
    }
    else
    {
        close();
    }
}

void dispenser_lib::actuators::servo_motor::open()
{
    if (!initialized)
    {
        throw dispenser_lib::actuators::not_initialized("servo_motor: non initialise");
    }
    servo.write(open_angle);
    current_state = 'O';
}

void dispenser_lib::actuators::servo_motor::close()
{
    if (!initialized)
    {
        throw dispenser_lib::actuators::not_initialized("servo_motor: non initialise");
    }
    servo.write(close_angle);
    current_state = 'C';
}