/*********************************************************************
 * @file  wifi_server.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief WiFi server header file
 *********************************************************************/

#ifndef WIFI_SERVER_HPP
#define WIFI_SERVER_HPP

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

extern const char *ssid;
extern const char *password;

extern ESP8266WebServer server;

namespace dispenser_lib
{
    namespace wifi_server
    {
        void init_time_ntp();
        void init_wifi_server();

    } // namespace wifi_server
} // namespace dispenser_lib
#endif // WIFI_SERVER_HPP