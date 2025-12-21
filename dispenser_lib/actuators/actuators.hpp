/*********************************************************************
 * @file  actuators.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Actuators header file
 *********************************************************************/

#ifndef ACTUATORS_HPP
#define ACTUATORS_HPP

#include <Arduino.h>
#include <Wire.h>
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
            lcd_screen(int sda, int scl, unsigned long speed, int columns, int rows);
            ~lcd_screen() = default;
            virtual void init_actuator();
            void display_message(const char *message, int row, int column);

        private:
            rgb_lcd lcd;
            int nbr_columns;
            int nbr_rows;
        };

    } // namespace actuators
} // namespace dispenser_lib
#endif // ACTUATORS_HPP