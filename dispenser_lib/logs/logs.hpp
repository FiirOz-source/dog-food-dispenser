/*********************************************************************
 * @file  logs.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Logs header file
 *********************************************************************/
#ifndef LOGS_HPP
#define LOGS_HPP

#include <Arduino.h>
#include <vector>

namespace dispenser_lib
{
    namespace logs
    {
        class logger
        {
        public:
            logger() = default;
            ~logger() = default;

            logger &operator+=(const String &message);

            const std::vector<String> &entries() const;

        private:
            std::vector<String> log_entries;
        };

    } // namespace logs

} // namespace dispenser_lib

#endif // DOGS_HPP
