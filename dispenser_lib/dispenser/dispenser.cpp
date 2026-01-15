/*********************************************************************
 * @file  dispenser.cpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief dispenser source file
 *********************************************************************/

#include "dispenser.hpp"

dispenser_lib::dogs::dog Jop("Jop", "0080D552");
dispenser_lib::dogs::dog Manouk("Manouk", "002E2989");

dispenser_lib::actuators::lcd_screen *lcd_screen = nullptr;
dispenser_lib::actuators::servo_motor *servo_motor = nullptr;
dispenser_lib::sensors::ultrasonic_sensor ultrasonic_sensor(ULTRASONIC_SENSOR_PIN);
dispenser_lib::sensors::infrared_sensor infrared_sensor(IR_PIN);
dispenser_lib::sensors::rfid_sensor rfid_sensor(RFID_RX_PIN, -1, 9600);

dispenser_lib::logs::logger app_log;

volatile bool ir_event = false;
volatile uint32_t last_isr_us = 0;

String last_event = "Boot";
String last_rfid = "";
long last_distance_cm = -1;

uint32_t jop_last_fed_ms = 0;
uint32_t manouk_last_fed_ms = 0;

volatile bool web_dispense_request = false;

void IRAM_ATTR on_IR_falling_wrapper();

void dispenser_lib::dispenser::lcd_print_and_log(const char *message, int row, int column)
{
    if (lcd_screen)
    {
        lcd_screen->display_message(message, row, column);
    }
    app_log += String(message);
}

void dispenser_lib::dispenser::lcd_print_and_log(const String &message, int row, int column)
{
    if (lcd_screen)
    {
        lcd_screen->display_message(message.c_str(), row, column);
    }
    app_log += message;
}

void dispenser_lib::dispenser::show_error(const char *line1, const char *line2)
{
    Serial.print("[ERROR] ");
    Serial.print(line1);
    Serial.print(" | ");
    Serial.println(line2);

    last_event = String("ERR: ") + line1;
    if (lcd_screen)
    {
        lcd_screen->clear();
    }
    dispenser_lib::dispenser::lcd_print_and_log(line1, 0, 0);
    dispenser_lib::dispenser::lcd_print_and_log(line2, 1, 0);
    delay(2000);
}

int dispenser_lib::dispenser::food_percent_from_distance(long cm)
{
    const int full_cm = 10;
    const int empty_cm = 100;

    if (cm < 0)
    {
        return -1;
    }
    if (cm <= full_cm)
    {
        return 100;
    }
    if (cm >= empty_cm)
    {
        return 0;
    }

    float pct = 100.0f * (float)(empty_cm - cm) / (float)(empty_cm - full_cm);
    if (pct < 0)
    {
        pct = 0;
    }
    if (pct > 100)
    {
        pct = 100;
    }
    return (int)(pct + 0.5f);
}

void dispenser_lib::dispenser::show_waiting_screen()
{
    try
    {
        if (lcd_screen)
        {
            lcd_screen->clear();
            lcd_screen->display_message("Dog Feeder", 0, 0);
            lcd_screen->display_message("Waiting for dog", 1, 0);
        }
        app_log += "Waiting for dog";
    }
    catch (const std::exception &e)
    {
        Serial.print("[ERROR] Display error: ");
        Serial.println(e.what());
    }
    last_event = "Waiting for dog";
}

void dispenser_lib::dispenser::handle_dog_detected()
{
    last_event = "Dog detected";

    long dist_mm = ultrasonic_sensor.get_distance();
    last_distance_cm = dist_mm / 10;

    if (last_distance_cm < FOOD_EMPTY_THRESHOLD_CM)
    {
        last_event = "Reading RFID";

        last_rfid = rfid_sensor.read_rfid(100);

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

                    try
                    {
                        if (lcd_screen)
                        {
                            lcd_screen->clear();
                            lcd_screen->display_message("Feeding Jop!", 0, 0);
                        }
                        app_log += "Feeding Jop!";
                        servo_motor->open();
                        delay(2000);
                        servo_motor->close();
                    }
                    catch (const std::exception &e)
                    {
                        show_error("Servo Error", e.what());
                    }
                }
                else
                {
                    last_event = "Jop already fed";

                    try
                    {
                        if (lcd_screen)
                        {
                            lcd_screen->clear();
                            lcd_screen->display_message("Jop already fed", 0, 0);
                            String since = Jop.since_fed();
                            lcd_screen->display_message(since.c_str(), 1, 0);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        Serial.print("[ERROR] Display error: ");
                        Serial.println(e.what());
                    }
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

                    try
                    {
                        if (lcd_screen)
                        {
                            lcd_screen->clear();
                            lcd_screen->display_message("Feeding Manouk!", 0, 0);
                        }
                        app_log += "Feeding Manouk!";
                        servo_motor->open();
                        delay(2000);
                        servo_motor->close();
                    }
                    catch (const std::exception &e)
                    {
                        show_error("Servo Error", e.what());
                    }
                }
                else
                {
                    last_event = "Manouk already fed";

                    try
                    {
                        if (lcd_screen)
                        {
                            lcd_screen->clear();
                            lcd_screen->display_message("Manouk alrdy fed", 0, 0);
                            String since = Manouk.since_fed();
                            lcd_screen->display_message(since.c_str(), 1, 0);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        Serial.print("[ERROR] Display error: ");
                        Serial.println(e.what());
                    }
                    delay(2000);
                }
            }
        }
        else
        {
            if (last_rfid != "")
            {
                last_event = "Unknown dog";

                try
                {
                    if (lcd_screen)
                    {
                        lcd_screen->clear();
                        lcd_screen->display_message("Unknown dog...", 0, 0);
                        char buf[32];
                        snprintf(buf, sizeof(buf), "Tag: %s", last_rfid.c_str());
                        lcd_screen->display_message(buf, 1, 0);
                    }
                }
                catch (const std::exception &e)
                {
                    Serial.print("[ERROR] Display error: ");
                    Serial.println(e.what());
                }
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
        try
        {
            if (lcd_screen)
            {
                lcd_screen->clear();
                lcd_screen->display_message("No more food ...", 0, 0);
            }
        }
        catch (const std::exception &e)
        {
            Serial.print("[ERROR] Display error: ");
            Serial.println(e.what());
        }
        delay(500);
    }
}

void dispenser_lib::dispenser::web_dispense()
{
    long dist_mm = ultrasonic_sensor.get_distance();
    last_distance_cm = dist_mm / 10;

    if (last_distance_cm >= FOOD_EMPTY_THRESHOLD_CM)
    {
        last_event = "Web dispense blocked (empty)";
        try
        {
            if (lcd_screen)
            {
                lcd_screen->clear();
                lcd_screen->display_message("No more food ...", 0, 0);
            }
        }
        catch (const std::exception &e)
        {
            Serial.print("[ERROR] Display error: ");
            Serial.println(e.what());
        }
        delay(500);
        return;
    }

    last_event = "Web dispense";
    try
    {
        if (lcd_screen)
        {
            lcd_screen->clear();
            lcd_screen->display_message("Web dispense", 0, 0);
            lcd_screen->display_message("Dispensing...", 1, 0);
        }
        app_log += "Web dispense";
        servo_motor->open();
        delay(2000);
        servo_motor->close();
    }
    catch (const std::exception &e)
    {
        show_error("Servo Error", e.what());
    }

    last_event = "Web dispense done";
}

void IRAM_ATTR on_IR_falling_wrapper()
{
    dispenser_lib::dispenser::on_IR_falling();
}

void IRAM_ATTR dispenser_lib::dispenser::on_IR_falling()
{
    uint32_t now = micros();
    if (now - last_isr_us < IR_DEBOUNCE_US)
        return;
    last_isr_us = now;
    ir_event = true;
}

void dispenser_lib::dispenser::init_dispenser()
{
    try
    {
        lcd_screen = new dispenser_lib::actuators::lcd_screen(SDA_PIN, SCL_PIN);
        lcd_screen->init_actuator();
        lcd_screen->display_message("Dog Feeder", 0, 0);
    }
    catch (const std::exception &e)
    {
        Serial.print("[FATAL] LCD initialization failed: ");
        Serial.println(e.what());
        while (1)
            delay(1000);
    }

    static int init = 0;
    char buf[32];

    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen->display_message(buf, 1, 0);
    app_log += String(buf);
    init = 22;
    delay(500);

    try
    {
        servo_motor = new dispenser_lib::actuators::servo_motor(SERVO_PIN);
        servo_motor->init_actuator();
        snprintf(buf, sizeof(buf), "Init %d%%", init);
        lcd_screen->display_message(buf, 1, 0);
        app_log += String(buf);
        init += 23;
        delay(500);

        servo_motor->toggle_position();
        delay(300);
        servo_motor->toggle_position();
    }
    catch (const std::exception &e)
    {
        Serial.print("[FATAL] Servo initialization failed: ");
        Serial.println(e.what());
        if (lcd_screen)
        {
            lcd_screen->clear();
            lcd_screen->display_message("Servo Error", 0, 0);
        }
        while (1)
            delay(1000);
    }

    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen->display_message(buf, 1, 0);
    app_log += String(buf);
    init += 22;
    delay(500);

    ultrasonic_sensor.init_sensor();
    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen->display_message(buf, 1, 0);
    app_log += String(buf);
    init += 24;
    delay(500);

    infrared_sensor.init_sensor();
    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen->display_message(buf, 1, 0);
    app_log += String(buf);

    rfid_sensor.init_sensor();
    init = 100;
    delay(500);
    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen->display_message(buf, 1, 0);
    app_log += String(buf);
    delay(500);
    lcd_screen->display_message("Init Complete", 1, 0);
    app_log += "Init Complete";

    pinMode(IR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(IR_PIN), on_IR_falling_wrapper, FALLING);

    show_waiting_screen();

    dispenser_lib::wifi_server::init_wifi_server();
}