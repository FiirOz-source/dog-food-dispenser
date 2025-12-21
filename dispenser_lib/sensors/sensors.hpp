/*********************************************************************
 * @file  sensors.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Sensors header file
 *********************************************************************/

#ifndef SENSORS_HPP
#define SENSORS_HPP

#include <Arduino.h>
#include <Wire.h>

namespace dispenser_lib
{
    namespace sensors
    {
        class sensor
        {
        public:
            sensor() = default;
            ~sensor() = default;
            virtual void read_sensor();
        };

        class serial_sensor : public sensor
        {
        public:
            serial_sensor() = default;
            ~serial_sensor() = default;
            void init_sensor(int rx_pin, int tx_pin, unsigned long baud_rate);
            void read_sensor() override;

        private:
            SoftwareSerial *serial_port;
            unsigned long baud_rate_;
        };

        class digital_sensor : public sensor
        {
        public:
            digital_sensor() = default;
            ~digital_sensor() = default;
            void init_sensor(int pin);
            void read_sensor() override;

        private:
            int sensor_pin;
        };

        class ultrasonic_sensor : public digital_sensor
        {
        public:
            ultrasonic_sensor() = default;
            ~ultrasonic_sensor() = default;
            long get_distance(uint32_t timeout);
        };

    } // namespace sensors
} // namespace dispenser_lib
#endif // SENSORS_HPP