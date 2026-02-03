#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32 // Mets 64 si ton écran est en 128x64

// D1 --> Brancher le SCK
// D2 --> Brancher le SDA

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup()
{
    // I2C sur ESP8266 (NodeMCU / D1 mini) : SDA=D2(GPIO4), SCL=D1(GPIO5)
    Wire.begin(D2, D1);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        while (true)
        {
            delay(1000);
        }
    }

    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    display.println("Bonjour OLED !");
    display.println("ESP8266 + SSD1306");
    display.println("I2C OK :)");

    display.display();
}

void loop()
{
}
