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


/* RFID sensor */
dispenser_lib::sensors::rfid_sensor::rfid_sensor(int rx_pin, int tx_pin, unsigned long baud_rate)
{
    SoftwareSerial rfid(rx_pin, tx_pin);
    rfid.begin(baud_rate);
}

void dispenser_lib::sensors::rfid_sensor::rfid_sensor()
{
    rfid_sensor(13,-1,9600)
}

std::vector<char> dispenser_lib::sensors::rfid_sensor::read_sensor()
{
    std::vector<char> tableau() //tableau vide
    tableau.push_back('\n')
    if (rfid.available())
    {
        char c = rfid.read();
        if (c != ' ') {
            tableau.clear()
            tableau.push_back(c)
            while(c != '\n'){
                c = rfid.read();
                tableau.push_back(c)
            }
        }
    }
    return tableau ;
}
