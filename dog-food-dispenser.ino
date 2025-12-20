#include <Arduino.h>
#include <Wire.h>
#include <rgb_lcd.h>

rgb_lcd lcd;

// Pour ESP8266 : SDA=GPIO4, SCL=GPIO5 (NodeMCU: D2=4, D1=5)
static const uint8_t SDA_PIN = 4;
static const uint8_t SCL_PIN = 5;

void setup()
{
    Serial.begin(115200);
    delay(200);

    // I2C (important sur ESP8266 : préciser les pins)
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000); // 100 kHz (safe)

    // Init LCD 16x2
    lcd.begin(16, 2);

    // Couleur du backlight (R,G,B)
    lcd.setRGB(0, 128, 255);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hello ESP8266!");

    lcd.setCursor(0, 1);
    lcd.print("Grove RGB LCD");
}

void loop()
{
    static uint32_t last = 0;
    static uint32_t counter = 0;

    if (millis() - last >= 1000)
    {
        last = millis();
        counter++;

        // Affiche un compteur sur la 2e ligne
        lcd.setCursor(0, 1);
        lcd.print("Count: ");
        lcd.print(counter);
        lcd.print("    "); // efface les restes si le nombre raccourcit
    }
}
