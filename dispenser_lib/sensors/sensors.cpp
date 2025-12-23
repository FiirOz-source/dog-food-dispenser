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
void dispenser_lib::sensors::rfid_sensor::init_sensor()
{
    serial_port.begin(baud_rate);
}

String dispenser_lib::sensors::rfid_sensor::read_rfid(uint32_t timeout_ms)
{
    const uint32_t start = millis();

    // Wait STX (0x02)
    while (millis() - start < timeout_ms)
    {
        while (serial_port.available() > 0)
        {
            uint8_t b = (uint8_t)serial_port.read();
            if (b != 0x02)
            {
                continue; // if not STX, keep waiting}
            }
            char tag[9];
            tag[8] = '\0';

            for (int i = 0; i < 2; i++)
            {
                while (serial_port.available() == 0)
                {
                    if (millis() - start >= timeout_ms)
                    {
                        return "";
                    }
                    yield();
                }
                serial_port.read();
            }

            for (int i = 0; i < 8; i++)
            {
                while (serial_port.available() == 0)
                {
                    if (millis() - start >= timeout_ms)
                    {
                        return "";
                    }
                    yield();
                }
                tag[i] = (char)serial_port.read();
            }

            for (int i = 0; i < 2; i++)
            {
                while (serial_port.available() == 0)
                {
                    if (millis() - start >= timeout_ms)
                    {
                        return "";
                    }
                    yield();
                }
                serial_port.read();
            }

            // wait ETX (0x03)
            while (serial_port.available() == 0)
            {
                if (millis() - start >= timeout_ms)
                {
                    return "";
                }
                yield();
            }
            uint8_t etx = (uint8_t)serial_port.read();
            if (etx != 0x03)
            {
                return "";
            }

            return String(tag);
        }
        yield();
    }

    return "";
}
