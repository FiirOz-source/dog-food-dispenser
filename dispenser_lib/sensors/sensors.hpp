/*********************************************************************
 * @file  sensors.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Sensors header file
 *********************************************************************/

#ifndef SENSORS_HPP
#define SENSORS_HPP

#include <Arduino.h>
#include <Wire.h>
#include <Ultrasonic.h>
#include <SoftwareSerial.h>
#include <vector>

namespace dispenser_lib
{
    namespace sensors
    {
        class sensor
        {
        public:
            sensor() = default;
            ~sensor() = default;
            virtual void init_sensor();
        };

        class serial_sensor : public sensor
        {
        public:
            serial_sensor() = default;
            serial_sensor(int rx_pin, int tx_pin, unsigned long baud_rate);
            ~serial_sensor() = default;
            void init_sensor() override;

        private:
            SoftwareSerial *serial_port;
            unsigned long baud_rate_;
        };

        class digital_sensor : public sensor
        {
        public:
            digital_sensor() = default;
            ~digital_sensor() = default;

        protected:
            int sensor_pin;
        };

        class ultrasonic_sensor : public digital_sensor
        {
        public:
            ultrasonic_sensor() = default;
            ultrasonic_sensor(int pin);
            ~ultrasonic_sensor() = default;
            long get_distance();
            void init_sensor() override;

        private:
            Ultrasonic ultrasonic;
        };

        class infrared_sensor : public digital_sensor
        {
        public:
            infrared_sensor() = default;
            infrared_sensor(int pin);
            ~infrared_sensor() = default;
            bool get_state();
            void init_sensor() override;
        };

        class rfid_sensor : public serial_sensor
        {
        public:
            rfid_sensor() = default;
            rfid_sensor(int rx_pin, int tx_pin, unsigned long baud_rate);
            ~rfid_sensor() = default;
            std::vector<char> read_sensor();
            void init_sensor() override;
        };

    } // namespace sensors
} // namespace dispenser_lib
#endif // SENSORS_HPP