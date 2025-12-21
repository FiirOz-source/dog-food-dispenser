#include <dispenser.hpp>
#include <actuators.hpp>

static const uint8_t SDA_PIN = 4;
static const uint8_t SCL_PIN = 5;
static const uint8_t SERVO_PIN = 14; // D5 on NODEMCU

// food_dispenser dispenser;
dispenser_lib::actuators::lcd_screen lcd_screen(SDA_PIN, SCL_PIN);
dispenser_lib::actuators::servo_motor servo_motor(SERVO_PIN);

void setup()
{
    // dispenser.init();
    lcd_screen.init_actuator();
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

    lcd_screen.display_message("Dog Feeder", 0, 0);

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
}