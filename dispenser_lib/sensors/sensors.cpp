/*********************************************************************
 * @file  sensors.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Sensors source file
 *********************************************************************/

#include "sensors.hpp"

/* Ultrasonic ranger */

dispenser_lib::sensors::ultrasonic_sensor::ultrasonic_sensor(int pin)
    : ultrasonic(pin)
{
    sensor_pin = pin;
}

void dispenser_lib::sensors::ultrasonic_sensor::init_sensor()
{
    ultrasonic = Ultrasonic(sensor_pin);
}

long dispenser_lib::sensors::ultrasonic_sensor::get_distance()
{
    return ultrasonic.MeasureInMillimeters();
}
