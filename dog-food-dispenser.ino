#include "dispenser_lib/dispenser.hpp"
#include <SoftwareSerial.h>

#define LED_PIN 2
#define LED_PIN2 12

SoftwareSerial rfid(13, -1);

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_PIN2, OUTPUT);
    Serial.begin(115200);
    Serial.println("\nRFID prêt (lecture via Serial RX0 / GPIO3)");
    rfid.begin(9600);
}

void loop()
{
    static int i = 0;
    if (i == 0)
    {
        Serial.print("Test loop : ");
        i++;
    }

    if (rfid.available())
    {
        byte c = rfid.read();
        Serial.print(c);
        Serial.print(" ");
    }
}