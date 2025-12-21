#include <dispenser.hpp>
#include <actuators.hpp>

static const uint8_t SDA_PIN = 4; // D2 sur NodeMCU
static const uint8_t SCL_PIN = 5; // D1 sur NodeMCU
static const uint8_t SERVO_PIN = 14;

// food_dispenser dispenser;
dispenser_lib::actuators::lcd_screen lcd_screen(SDA_PIN, SCL_PIN);
dispenser_lib::actuators::servo_motor servo_motor(SERVO_PIN);

void setup()
{
    // dispenser.init();
    lcd_screen.init_actuator();
    servo_motor.init_actuator();

    lcd_screen.display_message("Dog Feeder", 0, 0);

    static int init = 0;
    for (int i = 0; i < 10; i++)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "Init %d%%", init);
        lcd_screen.display_message(buf, 1, 0);
        delay(500);
        init += 10;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "Init %d%%", init);
    lcd_screen.display_message(buf, 1, 0);
    delay(500);
    lcd_screen.display_message("Init Complete", 1, 0);
}

void loop()
{
    // dispenser.run();
    servo_motor.toggle_position();
    delay(1000);
}