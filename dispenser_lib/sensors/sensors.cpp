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

/* Infrared sensor */
dispenser_lib::sensors::infrared_sensor::infrared_sensor(int pin)
    : digital_sensor()
{
    sensor_pin = pin;
}

void dispenser_lib::sensors::infrared_sensor::init_sensor()
{
    pinMode(sensor_pin, INPUT);
}

bool dispenser_lib::sensors::infrared_sensor::get_state()
{
    return digitalRead(sensor_pin);
}