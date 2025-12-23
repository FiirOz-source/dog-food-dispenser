/*********************************************************************
 * @file  dogs.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Dogs header file
 *********************************************************************/
#ifndef DOGS_HPP
#define DOGS_HPP

#include <Arduino.h>

namespace dispenser_lib
{
    namespace dogs
    {
        class dog
        {
        public:
            dog() = default;
            ~dog() = default;
            dog(const String &dog_name, const String &tag_rfid)
                : name(dog_name), rfid_tag(tag_rfid) {}

            bool matches_tag(const String tag);
            void mark_fed();
            unsigned long time_since_fed();
            bool can_feed();
            String since_fed();

        private:
            String name;
            String rfid_tag;

            unsigned long last_fed_ms = 0;
            bool fed_once = false;
        };

    } // namespace dogs
} // namespace dispenser_lib

#endif // DOGS_HPP
