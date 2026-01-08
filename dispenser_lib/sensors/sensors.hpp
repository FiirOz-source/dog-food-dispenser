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
#include <stdexcept>

namespace dispenser_lib
{
    namespace sensors
    {
        struct sensor_error : public std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };

        struct not_initialized : public sensor_error
        {
            using sensor_error::sensor_error;
        };

        struct invalid_pin : public sensor_error
        {
            using sensor_error::sensor_error;
        };

        struct rfid_timeout : public sensor_error
        {
            using sensor_error::sensor_error;
        };

        struct rfid_protocol_error : public sensor_error
        {
            using sensor_error::sensor_error;
        };

        class sensor
        {
        public:
            sensor() = default;
            ~sensor() = default;
            virtual void init_sensor();

        protected:
            bool initialized = false;
        };

        class serial_sensor : public sensor
        {
        public:
            serial_sensor()
                : serial_port(13, -1), baud_rate(9600), rx_pin(13), tx_pin(-1) {}

            serial_sensor(int rx, int tx, unsigned long baud)
                : serial_port(rx, tx), baud_rate(baud), rx_pin(rx), tx_pin(tx) {}

            ~serial_sensor() = default;

            void init_sensor() override
            {
                serial_port.begin(baud_rate);
            }

        protected:
            SoftwareSerial serial_port;
            unsigned long baud_rate;
            int rx_pin;
            int tx_pin;
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
            rfid_sensor(int rx, int tx, unsigned long baud)
                : serial_sensor(rx, tx, baud) {}
            ~rfid_sensor() = default;
            void init_sensor() override;
            String read_rfid(uint32_t timeout_ms = 80);
        };

    } // namespace sensors
} // namespace dispenser_lib
#endif // SENSORS_HPP