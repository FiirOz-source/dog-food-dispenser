/**
 * @file  sensors.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 2024
 * @brief Sensors header file - hardware input measurement interfaces.
 *
 * This file defines base classes and concrete implementations for all
 * input sensors: ultrasonic distance, infrared motion, and RFID identification.
 */

#ifndef SENSORS_HPP
#define SENSORS_HPP

#include <Arduino.h>
#include <Wire.h>
#include <Ultrasonic.h>
#include <SoftwareSerial.h>
#include <vector>

/**
 * @namespace dispenser_lib::sensors
 * @brief Hardware sensor (input device) classes and interfaces.
 *
 * Provides abstraction layers for measuring:
 * - Ultrasonic distance to detect food level
 * - Infrared motion to detect dog presence
 * - RFID tags to identify individual dogs
 *
 * Base classes organize sensors by communication protocol.
 */
namespace dispenser_lib
{
    namespace sensors
    {
        /**
         * @class sensor
         * @brief Base class for all sensors.
         *
         * Abstract base class defining the common interface for all
         * sensors (input devices).
         */
        class sensor
        {
        public:
            /// Default constructor
            sensor() = default;
            /// Default destructor
            ~sensor() = default;

            /**
             * @brief Initialize the sensor hardware.
             *
             * Virtual method that should be overridden by derived classes
             * to configure pins, protocols, and sensor state.
             *
             * @return void
             */
            virtual void init_sensor();
        };

        /**
         * @class serial_sensor
         * @brief Base class for serial port-based sensors.
         *
         * Sensors that communicate via UART/serial protocol using SoftwareSerial.
         * Extends @ref sensor base class.
         *
         * @see SoftwareSerial library
         */
        class serial_sensor : public sensor
        {
        public:
            /// Default constructor (creates serial on pins 13/TX, 9600 baud)
            serial_sensor()
                : serial_port(13, -1), baud_rate(9600), rx_pin(13), tx_pin(-1) {}

            /**
             * @brief Constructor with full configuration.
             *
             * @param rx RX pin number (connects to sensor TX)
             * @param tx TX pin number (connects to sensor RX), or -1 for RX-only
             * @param baud Serial baud rate (typically 9600)
             */
            serial_sensor(int rx, int tx, unsigned long baud)
                : serial_port(rx, tx), baud_rate(baud), rx_pin(rx), tx_pin(tx) {}

            /// Default destructor
            ~serial_sensor() = default;

            /**
             * @brief Initialize serial communication.
             *
             * Starts SoftwareSerial with configured baud rate.
             *
             * @return void
             */
            void init_sensor() override
            {
                serial_port.begin(baud_rate);
            }

        protected:
            /// SoftwareSerial instance for communication
            SoftwareSerial serial_port;
            /// Serial baud rate (typically 9600 for RFID readers)
            unsigned long baud_rate;
            /// RX (receive) pin number
            int rx_pin;
            /// TX (transmit) pin number, or -1 for RX-only
            int tx_pin;
        };

        /**
         * @class digital_sensor
         * @brief Base class for GPIO digital input sensors.
         *
         * Sensors that use standard GPIO pins for input (typically digital HIGH/LOW).
         * Extends @ref sensor base class.
         */
        class digital_sensor : public sensor
        {
        public:
            /// Default constructor
            digital_sensor() = default;
            /// Default destructor
            ~digital_sensor() = default;

        protected:
            /// GPIO input pin number
            int sensor_pin;
        };

        /**
         * @class ultrasonic_sensor
         * @brief Ultrasonic distance measurement sensor.
         *
         * Measures distance to objects using ultrasonic waves (sound).
         * Used to detect food level in the dispenser container.
         *
         * Extends @ref digital_sensor for GPIO communication.
         *
         * @see Ultrasonic library (Seeed Studio)
         */
        class ultrasonic_sensor : public digital_sensor
        {
        public:
            /// Default constructor
            ultrasonic_sensor() = default;

            /**
             * @brief Constructor with pin configuration.
             *
             * @param pin GPIO pin connected to ultrasonic sensor signal
             */
            ultrasonic_sensor(int pin);

            /// Default destructor
            ~ultrasonic_sensor() = default;

            /**
             * @brief Get the distance measured by the sensor.
             *
             * Performs one measurement and returns the distance in millimeters.
             *
             * @return Distance in millimeters as signed long integer
             *
             * @details
             * - Typical range: 2-400 cm (20-4000 mm)
             * - Returns negative value on error or out-of-range
             * - Measurement takes ~60ms to complete
             * - Non-blocking call (does not wait for measurement)
             *
             * @note
             * Convert to centimeters: divide by 10
             * Example: get_distance() / 10 = distance in cm
             */
            long get_distance();

            /**
             * @brief Initialize the ultrasonic sensor.
             *
             * Creates Ultrasonic object with configured pin.
             *
             * @return void
             * @throw std::exception if initialization fails
             */
            void init_sensor() override;

        private:
            /// Ultrasonic library instance
            Ultrasonic ultrasonic;
        };

        /**
         * @class infrared_sensor
         * @brief Infrared motion detector.
         *
         * Detects motion/presence using passive infrared radiation.
         * Used to trigger dog detection and feeding attempts.
         *
         * Extends @ref digital_sensor for GPIO communication.
         *
         * Typically used with a PIR (Passive Infrared Receiver) module.
         */
        class infrared_sensor : public digital_sensor
        {
        public:
            /// Default constructor
            infrared_sensor() = default;

            /**
             * @brief Constructor with pin configuration.
             *
             * @param pin GPIO input pin connected to IR sensor
             */
            infrared_sensor(int pin);

            /// Default destructor
            ~infrared_sensor() = default;

            /**
             * @brief Read the current state of the IR sensor.
             *
             * Reads GPIO pin state - whether motion is currently detected.
             *
             * @return true if motion detected (pin is HIGH), false otherwise
             *
             * @details
             * - Returns digital GPIO state (HIGH=1, LOW=0)
             * - No debouncing applied; raw GPIO state
             * - Non-blocking call
             *
             * @note
             * - Actual motion detection uses interrupt handler (on_IR_falling)
             * - This method reads instantaneous GPIO state
             */
            bool get_state();

            /**
             * @brief Initialize the infrared sensor.
             *
             * Configures GPIO pin as digital input.
             *
             * @return void
             * @throw std::exception if initialization fails
             */
            void init_sensor() override;
        };

        /**
         * @class rfid_sensor
         * @brief RFID reader for dog identification tags.
         *
         * Reads 8-character alphanumeric RFID tag IDs from dogs' wearable tags.
         * Communicates via serial protocol (9600 baud).
         *
         * Extends @ref serial_sensor for serial communication.
         *
         * Protocol: Binary framing with STX (0x02) and ETX (0x03) markers
         */
        class rfid_sensor : public serial_sensor
        {
        public:
            /// Default constructor
            rfid_sensor() = default;

            /**
             * @brief Constructor with full configuration.
             *
             * @param rx RX pin number (connects to reader TX)
             * @param tx TX pin number (connects to reader RX), or -1 for RX-only
             * @param baud Serial baud rate (typically 9600)
             */
            rfid_sensor(int rx, int tx, unsigned long baud)
                : serial_sensor(rx, tx, baud) {}

            /// Default destructor
            ~rfid_sensor() = default;

            /**
             * @brief Initialize RFID reader serial communication.
             *
             * Starts SoftwareSerial at configured baud rate.
             *
             * @return void
             * @throw std::exception if initialization fails
             */
            void init_sensor() override;

            /**
             * @brief Read an RFID tag from a dog's wearable tag.
             *
             * Attempts to read and return the 8-character RFID tag ID.
             * Waits for complete tag transmission with timeout.
             *
             * @param timeout_ms Maximum time to wait for complete tag in milliseconds
             *
             * @return String containing the 8-character RFID tag ID,
             *         or empty string "" if:
             *         - Timeout expires before tag received
             *         - Checksum error in received frame
             *         - No tag available
             *
             * @details
             * Protocol:
             * - STX (0x02) - Start marker
             * - 2 bytes - Checksum or reserved
             * - 8 bytes - Tag ID (ASCII characters)
             * - 2 bytes - Checksum or reserved
             * - ETX (0x03) - End marker
             *
             * Total frame: 15 bytes
             *
             * Processing:
             * - Blocks until STX received or timeout
             * - Reads complete frame: 13 bytes + ETX
             * - Validates frame format and returns tag ID
             * - Drains input buffer after reading (20ms quiet time)
             * - Returns empty string on any error
             *
             * @note
             * - Timeout should be reasonable (50-200ms typical)
             * - Multiple consecutive calls may return empty if no new tags
             * - Tag reader should be positioned 2-5cm from tag for reliable read
             *
             * Example:
             * @code
             * String tag = rfid_sensor.read_rfid(100);  // Wait up to 100ms
             * if (tag != "") {
             *     Serial.println("Tag: " + tag);  // Prints "0080D552" etc.
             * }
             * @endcode
             */
            String read_rfid(uint32_t timeout_ms = 80);

        private:
            /// Buffer for frame data during RFID reading
            std::vector<char> frame;
            /// Flag indicating if currently inside frame reception
            bool in_frame = false;
        };

    } // namespace sensors
} // namespace dispenser_lib

#endif // SENSORS_HPP
