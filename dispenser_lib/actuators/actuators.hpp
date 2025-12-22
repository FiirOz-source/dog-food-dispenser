/*********************************************************************
 * @file  actuators.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Actuators header file
 *********************************************************************/

#ifndef ACTUATORS_HPP
#define ACTUATORS_HPP

#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>
#include <rgb_lcd.h>

namespace dispenser_lib
{
    namespace actuators
    {
        class actuator
        {
        public:
            actuator() = default;
            ~actuator() = default;
            virtual void init_actuator();
        };

        class digital_actuator : public actuator
        {
        public:
            digital_actuator() = default;
            ~digital_actuator() = default;

        protected:
            int ctrl_pin;
        };

        class i2c_actuator : public actuator
        {
        public:
            i2c_actuator() = default;
            ~i2c_actuator() = default;
            virtual void init_actuator() override;

        protected:
            int sda_pin;
            int scl_pin;
            unsigned long i2c_speed;
        };

        class lcd_screen : public i2c_actuator
        {
        public:
            lcd_screen() = default;
            lcd_screen(int sda, int scl, unsigned long speed = 100000, int columns = 16, int rows = 2);
            ~lcd_screen() = default;
            virtual void init_actuator();
            void display_message(const char *message, int row, int column);

        private:
            rgb_lcd lcd;
            int nbr_columns;
            int nbr_rows;
        };

        class servo_motor : public actuator
        {
        public:
            servo_motor() = default;
            servo_motor(int ctrl_pin, int closed_angle = 0, int opened_angle = 90);
            ~servo_motor() = default;
            virtual void init_actuator() override;
            void toggle_position();
            void open();
            void close();

        private:
            int pin;
            int close_angle;
            int open_angle;
            Servo servo;
            char current_state; // 'O' for open, 'C' for closed
        };

    } // namespace actuators
} // namespace dispenser_lib
#endif // ACTUATORS_HPP