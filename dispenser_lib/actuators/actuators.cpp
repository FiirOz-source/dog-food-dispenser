/*********************************************************************
 * @file  actuators.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Actuators source file
 *********************************************************************/

#include "actuators.hpp"

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