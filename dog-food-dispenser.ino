#include <Arduino.h>

const uint8_t PIN_IR = D7; // GPIO13

bool irDetectedFiltered(uint8_t samples = 5, uint16_t gap_ms = 3)
{
    uint8_t lowCount = 0;
    for (uint8_t i = 0; i < samples; i++)
    {
        if (digitalRead(PIN_IR) == LOW)
            lowCount++;
        delay(gap_ms);
    }
    return lowCount > (samples / 2);
}

void setup()
{
    Serial.begin(115200);

    pinMode(PIN_IR, INPUT);

    Serial.println("IR obstacle sensor ready");
}

void loop()
{
    bool detected = irDetectedFiltered();

    static bool last = false;
    if (detected != last)
    {
        last = detected;
        Serial.println(detected ? "OBSTACLE: DETECTED (OUT=LOW)" : "OBSTACLE: CLEAR (OUT=HIGH)");
    }

    delay(50);
}
