/*********************************************************************
 * @file  actuators.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Actuators source file
 *********************************************************************/

#include "actuators.hpp"

/* LCD Screen */
dispenser_lib::actuators::lcd_screen::lcd_screen(int sda, int scl, unsigned long speed, int columns, int rows)
{
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
}

void dispenser_lib::actuators::lcd_screen::display_message(const char *message, int row, int column)
{
    lcd.setCursor(column, row);
    lcd.print(message);
}

/* Servo Motor */
dispenser_lib::actuators::servo_motor::servo_motor(int ctrl_pin, int closed_angle, int opened_angle)
{
    pin = ctrl_pin;
    close_angle = 2 * closed_angle;
    open_angle = 2 * opened_angle;
    current_state = 'C'; // Start with closed position
}

void dispenser_lib::actuators::servo_motor::init_actuator()
{
    servo.attach(pin);
    servo.write(close_angle);
}

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

void dispenser_lib::actuators::servo_motor::open()
{
    servo.write(open_angle);
    current_state = 'O';
}

void dispenser_lib::actuators::servo_motor::close()
{
    servo.write(close_angle);
    current_state = 'C';
}