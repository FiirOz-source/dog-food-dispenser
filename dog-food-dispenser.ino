#include <Arduino.h>
#include <Wire.h>
#include <rgb_lcd.h>
#include <Servo.h>

rgb_lcd lcd;
Servo servo;

// I2C
static const uint8_t SDA_PIN = 4; // D2 sur NodeMCU
static const uint8_t SCL_PIN = 5; // D1 sur NodeMCU

// Pin servo
static const uint8_t SERVO_PIN = 14;

static const int SERVO_CLOSE_ANGLE = 0;
static const int SERVO_OPEN_ANGLE = 180;

// Timings
static const uint32_t COUNTER_PERIOD_MS = 1000;
static const uint32_t SERVO_OPEN_TIME_MS = 700;   // temps ouvert
static const uint32_t SERVO_AFTER_CLOSE_MS = 300; // petite pause après fermeture

enum ServoState
{
    IDLE,
    OPENING,
    WAIT_OPEN,
    CLOSING,
    WAIT_CLOSE
};

ServoState servoState = IDLE;
uint32_t servoStateTs = 0;

uint32_t lastCounterTs = 0;
uint32_t counter = 0;

void updateLCDCounter(uint32_t value)
{
    lcd.setCursor(0, 1);
    lcd.print("Count: ");
    lcd.print(value);
    lcd.print("     ");
}

void triggerServoCycle()
{
    if (servoState != IDLE)
        return;

    servo.write(SERVO_OPEN_ANGLE);
    servoState = WAIT_OPEN;
    servoStateTs = millis();
}

void servoTask()
{
    const uint32_t now = millis();

    switch (servoState)
    {
    case IDLE:
        // rien
        break;

    case WAIT_OPEN:
        if (now - servoStateTs >= SERVO_OPEN_TIME_MS)
        {
            servo.write(SERVO_CLOSE_ANGLE);
            servoState = WAIT_CLOSE;
            servoStateTs = now;
        }
        break;

    case WAIT_CLOSE:
        if (now - servoStateTs >= SERVO_AFTER_CLOSE_MS)
        {
            servoState = IDLE;
        }
        break;

    default:
        servoState = IDLE;
        break;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(200);

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);

    lcd.begin(16, 2);
    lcd.setRGB(0, 128, 255);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dog feeder");

    updateLCDCounter(counter);

    // Servo
    servo.attach(SERVO_PIN);
    servo.write(SERVO_CLOSE_ANGLE);
}

void loop()
{
    const uint32_t now = millis();

    if (now - lastCounterTs >= COUNTER_PERIOD_MS)
    {
        lastCounterTs = now;
        counter++;

        updateLCDCounter(counter);

        if (counter == 10)
        {
            triggerServoCycle();
            counter = 0;
            updateLCDCounter(counter);
        }
    }

    servoTask();
}
