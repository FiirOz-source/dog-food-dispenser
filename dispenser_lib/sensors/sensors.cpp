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
    if (pin < 0)
    {
        throw dispenser_lib::sensors::invalid_pin("ultrasonic_sensor: pin invalide");
    }
    sensor_pin = pin;
}

void dispenser_lib::sensors::ultrasonic_sensor::init_sensor()
{
    ultrasonic = Ultrasonic(sensor_pin);
    initialized = true;
}

long dispenser_lib::sensors::ultrasonic_sensor::get_distance()
{
    if (!initialized)
    {
        throw dispenser_lib::sensors::not_initialized("ultrasonic_sensor: capteur non initialise");
    }

    long d = ultrasonic.MeasureInMillimeters();

    if (d < 0)
    {
        throw dispenser_lib::sensors::sensor_error("ultrasonic_sensor: mesure invalide");
    }
    return d;
}

/* Infrared sensor */
dispenser_lib::sensors::infrared_sensor::infrared_sensor(int pin)
    : digital_sensor()
{
    if (pin < 0)
    {
        throw dispenser_lib::sensors::invalid_pin("infrared_sensor: pin invalide");
    }
    sensor_pin = pin;
}

void dispenser_lib::sensors::infrared_sensor::init_sensor()
{
    pinMode(sensor_pin, INPUT);
    initialized = true;
}

bool dispenser_lib::sensors::infrared_sensor::get_state()
{
    if (!initialized)
    {
        throw dispenser_lib::sensors::not_initialized("infrared_sensor: capteur non initialise");
    }
    return digitalRead(sensor_pin);
}

/* RFID sensor */
void dispenser_lib::sensors::rfid_sensor::init_sensor()
{
    serial_port.begin(baud_rate);
    initialized = true;
}

static void drain_input(Stream &s, uint32_t quiet_ms = 20)
{
    uint32_t last = millis();
    while (millis() - last < quiet_ms)
    {
        while (s.available() > 0)
        {
            (void)s.read();
            last = millis();
        }
        yield();
    }
}

String dispenser_lib::sensors::rfid_sensor::read_rfid(uint32_t timeout_ms)
{
    if (!initialized)
    {
        throw dispenser_lib::sensors::not_initialized("rfid_sensor: capteur non initialise");
    }

    const uint32_t start = millis();

    while (millis() - start < timeout_ms)
    {
        while (serial_port.available() > 0)
        {
            uint8_t b = (uint8_t)serial_port.read();
            if (b != 0x02)
            {
                continue;
            }
            char tag[9];
            tag[8] = '\0';

            for (int i = 0; i < 2; i++)
            {
                while (serial_port.available() == 0)
                {
                    if (millis() - start >= timeout_ms)
                    {
                        throw dispenser_lib::sensors::rfid_timeout("rfid_sensor: timeout pendant l'entete");
                    }
                    yield();
                }
                (void)serial_port.read();
            }

            for (int i = 0; i < 8; i++)
            {
                while (serial_port.available() == 0)
                {
                    if (millis() - start >= timeout_ms)
                    {
                        throw dispenser_lib::sensors::rfid_timeout("rfid_sensor: timeout pendant la lecture du tag");
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
                        throw dispenser_lib::sensors::rfid_timeout("rfid_sensor: timeout pendant le checksum");
                    }
                    yield();
                }
                serial_port.read();
            }

            while (serial_port.available() == 0)
            {
                if (millis() - start >= timeout_ms)
                {
                    throw dispenser_lib::sensors::rfid_timeout("rfid_sensor: timeout en attente de ETX");
                }
                yield();
            }
            uint8_t etx = (uint8_t)serial_port.read();
            if (etx != 0x03)
            {
                drain_input(serial_port, 20);
                throw dispenser_lib::sensors::rfid_protocol_error("rfid_sensor: trame invalide (ETX manquant)");
            }

            String out(tag);
            drain_input(serial_port, 20);

            if (out.length() == 0)
            {
                throw dispenser_lib::sensors::rfid_protocol_error("rfid_sensor: tag vide");
            }

            return out;
        }
        yield();
    }

    throw dispenser_lib::sensors::rfid_timeout("rfid_sensor: aucun tag recu avant timeout");
}
