#include <actuators.hpp>
#include <sensors.hpp>
#include <dogs.hpp>
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include <stdexcept>

const char *ssid = "iPhone de Timothée";
const char *password = "TimAuThe";

ESP8266WebServer server(80);

static const uint8_t SDA_PIN = 4;
static const uint8_t SCL_PIN = 5;
static const uint8_t SERVO_PIN = 15;
static const uint8_t ULTRASONIC_SENSOR_PIN = 12;
static const uint8_t IR_PIN = 13;
static const uint8_t RFID_RX_PIN = 14;

dispenser_lib::dogs::dog Jop("Jop", "0080D552");
dispenser_lib::dogs::dog Manouk("Manouk", "002E2989");

dispenser_lib::actuators::lcd_screen lcd_screen(SDA_PIN, SCL_PIN);
dispenser_lib::actuators::servo_motor servo_motor(SERVO_PIN);
dispenser_lib::sensors::ultrasonic_sensor ultrasonic_sensor(ULTRASONIC_SENSOR_PIN);
dispenser_lib::sensors::infrared_sensor infrared_sensor(IR_PIN);
dispenser_lib::sensors::rfid_sensor rfid_sensor(RFID_RX_PIN, -1, 9600);

volatile bool ir_event = false;
volatile uint32_t last_isr_us = 0;
static const uint32_t IR_DEBOUNCE_US = 200000;

String last_event = "Boot";
String last_rfid = "";
long last_distance_cm = -1;

uint32_t jop_last_fed_ms = 0;
uint32_t manouk_last_fed_ms = 0;

static const long FOOD_EMPTY_THRESHOLD_CM = 100;

static volatile bool web_dispense_request = false;

static void show_error(const char *line1, const char *line2)
{
    Serial.print("[ERROR] ");
    Serial.print(line1);
    Serial.print(" | ");
    Serial.println(line2);

    last_event = String("ERR: ") + line1;
    lcd_screen.clear();
    lcd_screen.display_message(line1, 0, 0);
    lcd_screen.display_message(line2, 1, 0);
    delay(2000);
}

void IRAM_ATTR on_IR_falling()
{
    uint32_t now = micros();
    if (now - last_isr_us < IR_DEBOUNCE_US)
        return;
    last_isr_us = now;
    ir_event = true;
}

int food_percent_from_distance(long cm)
{
    const int full_cm = 10;
    const int empty_cm = 100;

    if (cm < 0)
        return -1;
    if (cm <= full_cm)
        return 100;
    if (cm >= empty_cm)
        return 0;

    float pct = 100.0f * (float)(empty_cm - cm) / (float)(empty_cm - full_cm);
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    return (int)(pct + 0.5f);
}

void show_waiting_screen()
{
    lcd_screen.clear();
    lcd_screen.display_message("Dog Feeder", 0, 0);
    lcd_screen.display_message("Waiting for dog", 1, 0);
    last_event = "Waiting for dog";
}

void handle_dog_detected()
{
    last_event = "Dog detected";

    try
    {
        long dist_mm = ultrasonic_sensor.get_distance();
        last_distance_cm = dist_mm / 10;

        if (last_distance_cm < FOOD_EMPTY_THRESHOLD_CM)
        {
            last_event = "Reading RFID";

            try
            {
                last_rfid = rfid_sensor.read_rfid(100);
            }
            catch (const dispenser_lib::sensors::rfid_timeout &)
            {
                last_rfid = "";
                last_event = "RFID timeout";
            }

            bool jop_matched = Jop.matches_tag(last_rfid);
            bool manouk_matched = Manouk.matches_tag(last_rfid);

            if (jop_matched || manouk_matched)
            {
                if (jop_matched)
                {
                    if (Jop.can_feed())
                    {
                        Jop.mark_fed();
                        jop_last_fed_ms = millis();
                        last_event = "Feeding Jop";

                        lcd_screen.clear();
                        lcd_screen.display_message("Feeding Jop!", 0, 0);
                        servo_motor.open();
                        delay(2000);
                        servo_motor.close();
                    }
                    else
                    {
                        last_event = "Jop already fed";

                        lcd_screen.clear();
                        lcd_screen.display_message("Jop already fed", 0, 0);
                        String since = Jop.since_fed();
                        lcd_screen.display_message(since.c_str(), 1, 0);
                        delay(2000);
                    }
                }
                else
                {
                    if (Manouk.can_feed())
                    {
                        Manouk.mark_fed();
                        manouk_last_fed_ms = millis();
                        last_event = "Feeding Manouk";

                        lcd_screen.clear();
                        lcd_screen.display_message("Feeding Manouk!", 0, 0);
                        servo_motor.open();
                        delay(2000);
                        servo_motor.close();
                    }
                    else
                    {
                        last_event = "Manouk already fed";

                        lcd_screen.clear();
                        lcd_screen.display_message("Manouk alrdy fed", 0, 0);
                        String since = Manouk.since_fed();
                        lcd_screen.display_message(since.c_str(), 1, 0);
                        delay(2000);
                    }
                }
            }
            else
            {
                if (last_rfid != "")
                {
                    last_event = "Unknown dog";

                    lcd_screen.clear();
                    lcd_screen.display_message("Unknown dog...", 0, 0);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Tag: %s", last_rfid.c_str());
                    lcd_screen.display_message(buf, 1, 0);
                    delay(2000);
                }
                else
                {
                    last_event = "RFID empty";
                }
            }
        }
        else
        {
            last_event = "No more food";
            lcd_screen.clear();
            lcd_screen.display_message("No more food ...", 0, 0);
            delay(500);
        }
    }
    catch (const dispenser_lib::sensors::not_initialized &e)
    {
        show_error("Sensor not init", e.what());
    }
    catch (const dispenser_lib::sensors::sensor_error &e)
    {
        show_error("Sensor error", e.what());
    }
    catch (const dispenser_lib::actuators::actuator_error &e)
    {
        Serial.println(e.what());
    }
    catch (const std::exception &e)
    {
        show_error("Std exception", e.what());
    }
}

void web_dispense()
{
    try
    {
        long dist_mm = ultrasonic_sensor.get_distance();
        last_distance_cm = dist_mm / 10;

        if (last_distance_cm >= FOOD_EMPTY_THRESHOLD_CM)
        {
            last_event = "Web dispense blocked (empty)";
            lcd_screen.clear();
            lcd_screen.display_message("No more food ...", 0, 0);
            delay(500);
            return;
        }

        last_event = "Web dispense";
        lcd_screen.clear();
        lcd_screen.display_message("Web dispense", 0, 0);
        lcd_screen.display_message("Dispensing...", 1, 0);

        servo_motor.open();
        delay(2000);
        servo_motor.close();

        last_event = "Web dispense done";
    }
    catch (const dispenser_lib::sensors::sensor_error &e)
    {
        show_error("Web sensor err", e.what());
    }
    catch (const dispenser_lib::actuators::actuator_error &e)
    {
        Serial.println(e.what());
    }
    catch (const std::exception &e)
    {
        show_error("Web exception", e.what());
    }
}

void setup()
{
    delay(100);
    Serial.begin(115200);
    delay(100);
    try
    {
        lcd_screen.init_actuator();
        lcd_screen.display_message("Dog Feeder", 0, 0);

        static int init = 0;
        char buf[32];

        snprintf(buf, sizeof(buf), "Init %d%%", init);
        lcd_screen.display_message(buf, 1, 0);
        init = 22;
        delay(500);

        servo_motor.init_actuator();
        snprintf(buf, sizeof(buf), "Init %d%%", init);
        lcd_screen.display_message(buf, 1, 0);
        init += 23;
        delay(500);

        servo_motor.toggle_position();
        delay(300);
        servo_motor.toggle_position();
        snprintf(buf, sizeof(buf), "Init %d%%", init);
        lcd_screen.display_message(buf, 1, 0);
        init += 22;
        delay(500);

        ultrasonic_sensor.init_sensor();
        snprintf(buf, sizeof(buf), "Init %d%%", init);
        lcd_screen.display_message(buf, 1, 0);
        init += 24;
        delay(500);

        infrared_sensor.init_sensor();
        snprintf(buf, sizeof(buf), "Init %d%%", init);
        lcd_screen.display_message(buf, 1, 0);

        rfid_sensor.init_sensor();
        init = 100;
        delay(500);
        snprintf(buf, sizeof(buf), "Init %d%%", init);
        lcd_screen.display_message(buf, 1, 0);
        delay(500);
        lcd_screen.display_message("Init Complete", 1, 0);

        pinMode(IR_PIN, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(IR_PIN), on_IR_falling, FALLING);

        show_waiting_screen();
    }
    catch (const dispenser_lib::sensors::sensor_error &e)
    {
        show_error("Sensor init fail", e.what());
        while (true)
        {
            delay(1000);
            yield();
        }
    }
    catch (const dispenser_lib::actuators::actuator_error &e)
    {
        Serial.println(e.what());
    }
    catch (const std::exception &e)
    {
        show_error("Init exception", e.what());
        while (true)
        {
            delay(1000);
            yield();
        }
    }
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
        int pct = food_percent_from_distance(dist);

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
}

void loop()
{
    static bool busy = false;
    static bool waiting_msg = false;

    server.handleClient();

    if (!waiting_msg && !busy)
    {
        show_waiting_screen();
        waiting_msg = true;
    }

    if (!busy)
    {
        if (web_dispense_request)
        {
            noInterrupts();
            web_dispense_request = false;
            interrupts();

            busy = true;
            waiting_msg = false;
            web_dispense();
            busy = false;
        }
        else if (ir_event)
        {
            noInterrupts();
            ir_event = false;
            interrupts();

            busy = true;
            waiting_msg = false;
            handle_dog_detected();
            busy = false;
        }
    }

    yield();
}
