#include <iostream>
#include <dispenser.hpp>
#include <actuators.hpp>
#include <sensors.hpp>

static const uint8_t SDA_PIN = 4;
static const uint8_t SCL_PIN = 5;
static const uint8_t SERVO_PIN = 14;             // D5 on NODEMCU
static const uint8_t ULTRASONIC_SENSOR_PIN = 12; // D6 on NODEMCU

// food_dispenser dispenser;
dispenser_lib::actuators::lcd_screen lcd_screen(SDA_PIN, SCL_PIN);
dispenser_lib::actuators::servo_motor servo_motor(SERVO_PIN);
dispenser_lib::sensors::ultrasonic_sensor ultrasonic_sensor(ULTRASONIC_SENSOR_PIN);

void setup()
{
    // dispenser.init();
    lcd_screen.init_actuator();
    lcd_screen.display_message("Dog Feeder", 0, 0);
    static int init = 0;
    char buf[32];

    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen.display_message(buf, 1, 0);
    init += 10;
    delay(500);

    servo_motor.init_actuator();
    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen.display_message(buf, 1, 0);
    init += 10;
    delay(500);

    servo_motor.toggle_position();
    delay(300);
    servo_motor.toggle_position();
    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen.display_message(buf, 1, 0);
    init += 10;
    delay(500);

    ultrasonic_sensor.init_sensor();
    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen.display_message(buf, 1, 0);
    init += 10;
    delay(500);

    for (init; init < 100; init += 10)
    {
        snprintf(buf, sizeof(buf), "Init %d%%", init);
        lcd_screen.display_message(buf, 1, 0);
        delay(500);
    }
    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen.display_message(buf, 1, 0);
    delay(500);
    lcd_screen.display_message("Init Complete", 1, 0);
}

void loop()
{
    // dispenser.run();
    long distance = ultrasonic_sensor.get_distance();
    char buf[32];

    lcd_screen.display_message("                ", 1, 0);
    snprintf(buf, sizeof(buf), "Distance %ld mm", distance);
    lcd_screen.display_message(buf, 1, 0);
    delay(500);
}