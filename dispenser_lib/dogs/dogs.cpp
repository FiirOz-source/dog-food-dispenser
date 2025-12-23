/*********************************************************************
 * @file  dogs.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Dogs source file
 *********************************************************************/

#include "dogs.hpp"

bool dispenser_lib::dogs::dog::matches_tag(const String tag)
{
    return tag == rfid_tag;
}

void dispenser_lib::dogs::dog::mark_fed()
{
    last_fed_ms = millis();
    fed_once = true;
}

unsigned long dispenser_lib::dogs::dog::time_since_fed()
{
    if (!fed_once)
    {
        return 0UL;
    }
    return (unsigned long)(millis() - last_fed_ms);
}

// TRUE if we can feed
bool dispenser_lib::dogs::dog::can_feed()
{
    if (!fed_once)
    {
        return true;
    }
    const unsigned long TWELVE_HOURS_MS = 12UL * 60UL * 60UL * 1000UL; // 43 200 000 ms
    return (unsigned long)(millis() - last_fed_ms) >= TWELVE_HOURS_MS;
}

String dispenser_lib::dogs::dog::since_fed()
{
    if (!fed_once)
    {
        return "never";
    }

    unsigned long sec = (unsigned long)(millis() - last_fed_ms) / 1000UL;

    unsigned long days = sec / 86400UL;
    sec %= 86400UL;
    unsigned long hrs = sec / 3600UL;
    sec %= 3600UL;
    unsigned long mins = sec / 60UL;
    sec %= 60UL;

    char buf[48];
    if (days > 0)
    {
        snprintf(buf, sizeof(buf), "%lud%luh%lum%lus ago", days, hrs, mins, sec);
    }
    else if (hrs > 0)
    {
        snprintf(buf, sizeof(buf), "%luh%lum%lus ago", hrs, mins, sec);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%lum%lus ago", mins, sec);
    }

    return String(buf);
}
