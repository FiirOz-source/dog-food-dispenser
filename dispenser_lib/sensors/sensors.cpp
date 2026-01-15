/**
 * @file  sensors.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @date 2024
 * @brief Sensors implementation - hardware input device measurement.
 *
 * This file contains the implementation for ultrasonic distance, infrared motion,
 * and RFID tag reading sensors.
 */

#include "sensors.hpp"

// ============================================================================
// Ultrasonic Sensor Implementation
// ============================================================================

/**
 * @brief Ultrasonic Sensor Constructor.
 *
 * Initializes the ultrasonic sensor with the specified GPIO pin.
 *
 * @param pin GPIO pin connected to the ultrasonic sensor signal
 *
 * @details
 * - Pin configuration is stored in sensor_pin
 * - Ultrasonic object is created but not yet initialized
 * - Call init_sensor() to complete hardware initialization
 */
dispenser_lib::sensors::ultrasonic_sensor::ultrasonic_sensor(int pin)
    : ultrasonic(pin)
{
    sensor_pin = pin;
}

/**
 * @brief Initialize the ultrasonic sensor hardware.
 *
 * Creates a new Ultrasonic object with the configured GPIO pin.
 *
 * @return void
 *
 * @note Must be called once during system initialization before reading distances
 */
void dispenser_lib::sensors::ultrasonic_sensor::init_sensor()
{
    ultrasonic = Ultrasonic(sensor_pin);
}

/**
 * @brief Get the distance measured by the ultrasonic sensor.
 *
 * Performs a single ultrasonic distance measurement and returns the result
 * in millimeters.
 *
 * @return Distance in millimeters (long integer)
 *         - Positive values: valid distance measurement
 *         - Negative values: measurement error or out-of-range
 *
 * @details
 * - Uses the Seeed Studio Ultrasonic library MeasureInMillimeters() method
 * - Typical measurement range: 20-4000 mm (2-400 cm)
 * - Measurement time: approximately 60ms per reading
 * - Non-blocking call (does not wait)
 *
 * @note
 * - Convert to centimeters: divide by 10
 * - Check for negative return values to detect errors
 * - Allow for ~100ms delay between measurements for accurate readings
 *
 * Example:
 * @code
 * long dist_mm = ultrasonic_sensor.get_distance();
 * long dist_cm = dist_mm / 10;
 * if (dist_cm < 100) {
 *     Serial.println("Food level: " + String(dist_cm) + " cm");
 * }
 * @endcode
 */
long dispenser_lib::sensors::ultrasonic_sensor::get_distance()
{
    return ultrasonic.MeasureInMillimeters();
}

// ============================================================================
// Infrared Sensor Implementation
// ============================================================================

/**
 * @brief Infrared Sensor Constructor.
 *
 * Initializes the infrared motion detector with the specified GPIO pin.
 *
 * @param pin GPIO digital input pin connected to the IR sensor module
 *
 * @details
 * - Pin configuration is stored in sensor_pin
 * - Inherits from digital_sensor base class
 * - Call init_sensor() to complete hardware pin configuration
 */
dispenser_lib::sensors::infrared_sensor::infrared_sensor(int pin)
    : digital_sensor()
{
    sensor_pin = pin;
}

/**
 * @brief Initialize the infrared sensor GPIO pin.
 *
 * Configures the GPIO pin as a digital input for IR sensor reading.
 *
 * @return void
 *
 * @details
 * - Sets pin mode to INPUT via Arduino pinMode()
 * - Must be called once during system initialization
 *
 * @note The actual motion detection uses an interrupt handler (FALLING edge)
 *       for low-latency event detection. This init just sets up the pin mode.
 */
void dispenser_lib::sensors::infrared_sensor::init_sensor()
{
    pinMode(sensor_pin, INPUT);
}

/**
 * @brief Read the current state of the infrared sensor.
 *
 * Returns the instantaneous digital state of the IR sensor GPIO pin.
 *
 * @return bool
 *         - true if motion is currently detected (pin is HIGH)
 *         - false if no motion (pin is LOW)
 *
 * @details
 * - Performs a raw GPIO digital read (no debouncing)
 * - Returns instantaneous state, not smoothed or filtered
 * - Non-blocking call
 *
 * @note
 * - This reads the raw GPIO state
 * - The actual motion detection and debouncing happens in the interrupt handler
 * - This method is useful for polling if needed, but interrupts are preferred
 *
 * @see dispenser_lib::dispenser::on_IR_falling() for debounced interrupt handling
 */
bool dispenser_lib::sensors::infrared_sensor::get_state()
{
    return digitalRead(sensor_pin);
}

// ============================================================================
// RFID Sensor Implementation
// ============================================================================

/**
 * @brief Initialize the RFID reader serial communication.
 *
 * Starts the SoftwareSerial port at the configured baud rate (9600).
 *
 * @return void
 *
 * @details
 * - Calls serial_port.begin(baud_rate) inherited from serial_sensor
 * - Must be called once during system initialization
 * - After this, RFID tags can be read via read_rfid()
 *
 * @note The RFID reader should be powered on and properly wired before calling
 */
void dispenser_lib::sensors::rfid_sensor::init_sensor()
{
    serial_port.begin(baud_rate);
}

/**
 * @brief Helper function to drain input buffer after reading.
 *
 * Discards any remaining data in the serial input buffer to ensure
 * clean reads on subsequent RFID tag reads.
 *
 * @param s Reference to the serial stream to drain
 * @param quiet_ms Milliseconds of silence required to consider buffer drained (default: 20)
 *
 * @return void
 *
 * @details
 * - Reads and discards bytes until quiet_ms milliseconds of no data
 * - Calls yield() periodically to avoid blocking the ESP8266 watchdog
 * - Used internally to clean up after RFID frame reception
 */
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

/**
 * @brief Read an RFID tag from a dog's identification tag.
 *
 * Attempts to read and parse an RFID tag transmitted by the reader module.
 * Waits for complete frame reception with timeout.
 *
 * @param timeout_ms Maximum milliseconds to wait for a complete RFID frame (default: 80)
 *
 * @return String
 *         - 8-character RFID tag ID (e.g., "0080D552") on success
 *         - Empty string "" on timeout, error, or no tag available
 *
 * @details
 *
 * **RFID Protocol Frame Format:**
 * ```
 * Byte 0:      STX (0x02)          - Start marker
 * Bytes 1-2:   [reserved/checksum]
 * Bytes 3-10:  [8-char tag ID]     - The actual RFID tag value
 * Bytes 11-12: [reserved/checksum]
 * Byte 13:     ETX (0x03)          - End marker
 * Total:       14 bytes
 * ```
 *
 * **Processing Steps:**
 * 1. Wait for STX (0x02) byte or timeout
 * 2. Skip 2 bytes (reserved/checksum)
 * 3. Read 8 bytes of tag ID into buffer
 * 4. Skip 2 bytes (reserved/checksum)
 * 5. Verify ETX (0x03) byte
 * 6. Drain serial buffer (20ms quiet period)
 * 7. Return tag ID string or "" on any error
 *
 * **Error Handling:**
 * - Timeout during any byte read → returns ""
 * - ETX marker not received → drains buffer and returns ""
 * - Returns empty string on all error conditions
 *
 * @note
 * - Timeout should be 50-200ms depending on reader and tag proximity
 * - RFID reader should be within 2-5cm of tag for reliable reads
 * - Multiple consecutive calls may return "" if no new tags are present
 * - Buffer is drained after each read attempt (successful or not)
 *
 * Example Usage:
 * @code
 * // In the main loop or interrupt handler:
 * String tag = rfid_sensor.read_rfid(100);  // Wait up to 100ms
 *
 * if (tag == "0080D552") {
 *     Serial.println("Dog Jop detected!");
 * } else if (tag == "002E2989") {
 *     Serial.println("Dog Manouk detected!");
 * } else if (tag != "") {
 *     Serial.println("Unknown tag: " + tag);
 * }
 * // If tag == "", no tag was read in the timeout period
 * @endcode
 */
String dispenser_lib::sensors::rfid_sensor::read_rfid(uint32_t timeout_ms)
{
    const uint32_t start = millis();

    // Wait for STX (0x02)
    while (millis() - start < timeout_ms)
    {
        while (serial_port.available() > 0)
        {
            uint8_t b = (uint8_t)serial_port.read();
            if (b != 0x02)
            {
                continue; // if not STX, keep waiting
            }

            // STX found, now read the rest of the frame
            char tag[9];
            tag[8] = '\0'; // Null-terminate string

            // Skip 2 bytes (reserved/checksum)
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

            // Read 8 bytes of tag ID
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

            // Skip 2 bytes (reserved/checksum)
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

            // Wait for ETX (0x03)
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
                drain_input(serial_port, 20);
                return "";
            }

            // Success: return the tag ID
            String out(tag);
            drain_input(serial_port, 20);
            return out;
        }
        yield();
    }

    // Timeout: no STX found
    return "";
}