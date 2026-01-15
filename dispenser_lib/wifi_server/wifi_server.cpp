/*********************************************************************
 * @file  wifi_server.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief WiFi server source file
 *********************************************************************/

#include "wifi_server.hpp"
#include "dispenser.hpp"

const char *ssid = "iPhone de Timothée";
const char *password = "TimAuThe";

ESP8266WebServer server(80);

void dispenser_lib::wifi_server::init_time_ntp()
{
    configTime(0, 0, "pool.ntp.org", "time.google.com", "time.nist.gov");

    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

    time_t now = time(nullptr);
    while (now < 1609459200)
    {
        delay(200);
        yield();
        now = time(nullptr);
    }
}

void dispenser_lib::wifi_server::init_wifi_server()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connexion WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(300);
        Serial.print(".");
    }
    Serial.println("\nConnecté !");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    if (!LittleFS.begin())
    {
        Serial.println("Erreur LittleFS !");
        return;
    }

    server.serveStatic("/", LittleFS, "/index.html");

    server.on("/status", HTTP_GET, []()
              {
        uint32_t up = millis();
        long dist = last_distance_cm;
        int pct = dispenser_lib::dispenser::food_percent_from_distance(dist);

        String json = "{";
        json += "\"event\":\"" + last_event + "\",";
        json += "\"distance_cm\":" + String(dist) + ",";
        json += "\"food_percent\":" + String(pct) + ",";
        json += "\"last_rfid\":\"" + last_rfid + "\",";
        json += "\"uptime_ms\":" + String(up) + ",";
        json += "\"jop_last_fed_uptime_ms\":" + String(jop_last_fed_ms) + ",";
        json += "\"manouk_last_fed_uptime_ms\":" + String(manouk_last_fed_ms);
        json += "}";
        server.send(200, "application/json; charset=utf-8", json); });
    server.on("/logs", HTTP_GET, []()
              {
    size_t from = 0;
    if (server.hasArg("from"))
    {
        long f = server.arg("from").toInt();
        if (f > 0) from = (size_t)f;
    }

    const std::vector<String>& v = app_log.entries();
    const size_t total = v.size();
    if (from > total) from = total;

    String json = "{";
    json += "\"total\":" + String((unsigned long)total) + ",";
    json += "\"from\":" + String((unsigned long)from) + ",";
    json += "\"entries\":[";

    bool first = true;
    for (size_t i = from; i < total; ++i)
    {
        String s = v[i];
        s.replace("\\", "\\\\");
        s.replace("\"", "\\\"");
        s.replace("\n", "\\n");
        s.replace("\r", "\\r");

        if (!first) json += ",";
        first = false;
        json += "\"" + s + "\"";
    }

    json += "]}";
    server.send(200, "application/json; charset=utf-8", json); });

    server.on("/dispense", HTTP_POST, []()
              {
        noInterrupts();
        web_dispense_request = true;
        interrupts();
        server.send(200, "text/plain; charset=utf-8", "ok"); });

    server.onNotFound([]()
                      { server.send(404, "text/plain; charset=utf-8", "404 - introuvable"); });

    server.begin();
    Serial.println("Serveur OK.");
    dispenser_lib::wifi_server::init_time_ntp();
}