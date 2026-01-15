/*********************************************************************
 * @file  logs.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Logs source file
 *********************************************************************/

#include "logs.hpp"
#include <time.h>
#include <stdio.h>

static bool time_is_valid()
{
    time_t now = time(nullptr);
    return now >= 1609459200;
}

static String make_timestamp_fr()
{
    static const char *kDays[] = {"Dim.", "Lun.", "Mar.", "Mer.", "Jeu.", "Ven.", "Sam."};
    static const char *kMonths[] = {"janv.", "fevr.", "mars", "avr.", "mai", "juin", "juil.", "aout", "sept.", "oct.", "nov.", "dec."};

    char buf[48];

    if (time_is_valid())
    {
        time_t now = time(nullptr);
        struct tm tm_now;
        localtime_r(&now, &tm_now);

        snprintf(
            buf, sizeof(buf),
            "[%s %d %s %02d:%02d] ",
            kDays[tm_now.tm_wday],
            tm_now.tm_mday,
            kMonths[tm_now.tm_mon],
            tm_now.tm_hour,
            tm_now.tm_min);
        return String(buf);
    }

    uint32_t total_min = millis() / 60000UL;
    uint32_t hh = (total_min / 60UL) % 100UL;
    uint32_t mm = total_min % 60UL;

    snprintf(buf, sizeof(buf), "[Uptime %02lu:%02lu] ", (unsigned long)hh, (unsigned long)mm);
    return String(buf);
}

dispenser_lib::logs::logger &dispenser_lib::logs::logger::operator+=(const String &message)
{
    String line = make_timestamp_fr() + message;
    log_entries.push_back(line);
    return *this;
}

const std::vector<String> &dispenser_lib::logs::logger::entries() const
{
    return log_entries;
}
