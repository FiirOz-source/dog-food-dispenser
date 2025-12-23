#include <iostream>
#include <actuators.hpp>
#include <sensors.hpp>
#include <dogs.hpp>

static const uint8_t SDA_PIN = 4;
static const uint8_t SCL_PIN = 5;
static const uint8_t SERVO_PIN = 14;             // D5 on NODEMCU
static const uint8_t ULTRASONIC_SENSOR_PIN = 12; // D6 on NODEMCU
static const uint8_t IR_PIN = 13;                // D7 on NODEMCU
static const uint8_t RFID_RX_PIN = 15;           // D8 on NODEMCU

dispenser_lib::dogs::dog Jop("Jop", "0080D552");
dispenser_lib::dogs::dog Manouk("Manouk", "002E2989");

dispenser_lib::actuators::lcd_screen lcd_screen(SDA_PIN, SCL_PIN);
dispenser_lib::actuators::servo_motor servo_motor(SERVO_PIN);
dispenser_lib::sensors::ultrasonic_sensor ultrasonic_sensor(ULTRASONIC_SENSOR_PIN);
dispenser_lib::sensors::infrared_sensor infrared_sensor(IR_PIN);
dispenser_lib::sensors::rfid_sensor rfid_sensor(RFID_RX_PIN, -1, 9600); // RX, TX, Baudrate
void setup()
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
}

void loop()
{
    static int previous_state = LOW;
    static bool waiting_msg = true;
    int current_state = infrared_sensor.get_state();
    if ((current_state == LOW) && (previous_state == HIGH))
    {
        waiting_msg = true;
        long distance = ultrasonic_sensor.get_distance();
        if (distance < (long)100)
        {
            String rfid_tag = rfid_sensor.read_rfid(100);
            bool jop_matched = Jop.matches_tag(rfid_tag);
            bool manouk_matched = Manouk.matches_tag(rfid_tag);
            if ((jop_matched) || (manouk_matched))
            {
                if (jop_matched)
                {
                    if (Jop.can_feed())
                    {
                        Jop.mark_fed();
                        lcd_screen.clear();
                        lcd_screen.display_message("Feeding Jop!", 0, 0);
                        servo_motor.open();
                        delay(2000);
                        servo_motor.close();
                    }
                    else
                    {
                        lcd_screen.clear();
                        lcd_screen.display_message("Jop already fed", 0, 0);
                        String since = Jop.since_fed();
                        lcd_screen.display_message(since.c_str(), 1, 0);
                        delay(2000);
                    }
                }
                else if (manouk_matched)
                {
                    if (Manouk.can_feed())
                    {
                        Manouk.mark_fed();
                        lcd_screen.clear();
                        lcd_screen.display_message("Feeding Manouk!", 0, 0);
                        servo_motor.open();
                        delay(2000);
                        servo_motor.close();
                    }
                    else
                    {
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
                if (rfid_tag != "")
                {
                    lcd_screen.clear();
                    lcd_screen.display_message("Unknown dog...", 0, 0);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Tag: %s", rfid_tag.c_str());
                    lcd_screen.display_message(buf, 1, 0);
                    previous_state = current_state;
                    delay(2000);
                }
            }
        }
        else
        {
            lcd_screen.clear();
            lcd_screen.display_message("No more food ...", 0, 0);
            delay(500);
        }
    }
    else
    {
        if (waiting_msg)
        {
            lcd_screen.clear();
            lcd_screen.display_message("Dog Feeder", 0, 0);
            lcd_screen.display_message("Waiting for dog", 1, 0);
            waiting_msg = false;
        }
    }
    previous_state = current_state;
}
