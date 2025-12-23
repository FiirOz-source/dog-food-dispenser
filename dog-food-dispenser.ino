#include <iostream>
#include <dispenser.hpp>
#include <actuators.hpp>
#include <sensors.hpp>

static const uint8_t SDA_PIN = 4;
static const uint8_t SCL_PIN = 5;
static const uint8_t SERVO_PIN = 14;             // D5 on NODEMCU
static const uint8_t ULTRASONIC_SENSOR_PIN = 12; // D6 on NODEMCU
static const uint8_t IR_PIN = 13;                // D7 on NODEMCU
static const uint8_t RFID_RX_PIN = 15;           // D8 on NODEMCU

// food_dispenser dispenser;
dispenser_lib::actuators::lcd_screen lcd_screen(SDA_PIN, SCL_PIN);
dispenser_lib::actuators::servo_motor servo_motor(SERVO_PIN);
dispenser_lib::sensors::ultrasonic_sensor ultrasonic_sensor(ULTRASONIC_SENSOR_PIN);
dispenser_lib::sensors::infrared_sensor infrared_sensor(IR_PIN);
dispenser_lib::sensors::rfid_sensor rfid_sensor(RFID_RX_PIN, -1, 9600); // RX, TX, Baudrate
void setup()
{
    // dispenser.init();
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
    static int previous_state = 1;
    int current_state = infrared_sensor.get_state();
    if ((current_state == LOW) && (previous_state == HIGH))
    {
        long distance = ultrasonic_sensor.get_distance();
        char buf[32];
        snprintf(buf, sizeof(buf), "Dist %ld mm", distance);
        lcd_screen.display_message("                ", 1, 0);
        lcd_screen.display_message(buf, 1, 0);
    }
    previous_state = current_state;

    String rfid_tag = rfid_sensor.read_rfid(100);
    if (rfid_tag.length() > 0)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "RFID: %s", rfid_tag.c_str());
        lcd_screen.display_message("                ", 0, 0);
        lcd_screen.display_message(buf, 0, 0);
    }
}
