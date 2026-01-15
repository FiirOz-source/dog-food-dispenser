/*********************************************************************
 * @file  dispenser.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Dispenser header file
 *********************************************************************/

#ifndef DISPENSER_HPP
#define DISPENSER_HPP

#include <Arduino.h>
#include <actuators.hpp>
#include <sensors.hpp>
#include <dogs.hpp>
#include <logs.hpp>
#include <wifi_server.hpp>

static const uint8_t SDA_PIN = 4;
static const uint8_t SCL_PIN = 5;
static const uint8_t SERVO_PIN = 15;
static const uint8_t ULTRASONIC_SENSOR_PIN = 12;
static const uint8_t IR_PIN = 13;
static const uint8_t RFID_RX_PIN = 14;

extern dispenser_lib::dogs::dog Jop;
extern dispenser_lib::dogs::dog Manouk;

extern dispenser_lib::actuators::lcd_screen *lcd_screen;
extern dispenser_lib::actuators::servo_motor *servo_motor;
extern dispenser_lib::sensors::ultrasonic_sensor ultrasonic_sensor;
extern dispenser_lib::sensors::infrared_sensor infrared_sensor;
extern dispenser_lib::sensors::rfid_sensor rfid_sensor;

extern dispenser_lib::logs::logger app_log;

extern volatile bool ir_event;
extern volatile uint32_t last_isr_us;
static const uint32_t IR_DEBOUNCE_US = 200000;

extern String last_event;
extern String last_rfid;
extern long last_distance_cm;

extern uint32_t jop_last_fed_ms;
extern uint32_t manouk_last_fed_ms;

static const long FOOD_EMPTY_THRESHOLD_CM = 100;

extern volatile bool web_dispense_request;

namespace dispenser_lib
{
    namespace dispenser
    {
        void lcd_print_and_log(const char *message, int row, int column);
        void lcd_print_and_log(const String &message, int row, int column);
        void show_error(const char *line1, const char *line2);

        int food_percent_from_distance(long cm);
        void show_waiting_screen();
        void handle_dog_detected();
        void web_dispense();

        void on_IR_falling();
        void init_dispenser();
    } // namespace dispenser
} // namespace dispenser_lib
#endif // DISPENSER_HPP