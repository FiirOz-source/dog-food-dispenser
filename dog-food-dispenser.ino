#include <Arduino.h>

const uint8_t PIN_TRIG = D5; // GPIO14
const uint8_t PIN_ECHO = D6; // GPIO12

float readDistanceCm(uint32_t timeout_us = 30000)
{
    // Assure un TRIG bas stable
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);

    // Impulsion TRIG de 10µs
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    // Mesure la durée du pulse HIGH sur ECHO
    // timeout_us ~ 30000 -> ~5m max (aller-retour), mais en pratique HC-SR04 ~ 4m
    uint32_t duration = pulseIn(PIN_ECHO, HIGH, timeout_us);

    if (duration == 0)
    {
        // timeout -> pas de mesure valide
        return -1.0f;
    }

    // Distance = (durée_us * vitesse_du_son_cm/us) / 2
    // vitesse du son ~ 343 m/s => 0.0343 cm/us
    float distance = (duration * 0.0343f) / 2.0f;
    return distance;
}

void setup()
{
    Serial.begin(115200);
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);

    Serial.println("HC-SR04 + ESP8266 OK");
}

void loop()
{
    // Petite moyenne sur 3 mesures pour stabiliser
    float sum = 0;
    int ok = 0;

    for (int i = 0; i < 3; i++)
    {
        float d = readDistanceCm();
        if (d > 0)
        {
            sum += d;
            ok++;
        }
        delay(50);
    }

    if (ok == 0)
    {
        Serial.println("Distance: (timeout)");
    }
    else
    {
        float avg = sum / ok;
        Serial.print("Distance: ");
        Serial.print(avg, 1);
        Serial.println(" cm");
    }

    delay(200);
}
