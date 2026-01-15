#include "dispenser.hpp"

#include <Arduino.h>
#include <time.h>
#include <stdexcept>

void setup()
{
    delay(100);
    Serial.begin(115200);
    delay(100);

    dispenser_lib::dispenser::init_dispenser();
}

void loop()
{
    static bool busy = false;
    static bool waiting_msg = false;

    server.handleClient();

    if (!waiting_msg && !busy)
    {
        dispenser_lib::dispenser::show_waiting_screen();
        waiting_msg = true;
    }

    if (!busy)
    {
        if (web_dispense_request)
        {
            noInterrupts();
            web_dispense_request = false;
            interrupts();

            busy = true;
            waiting_msg = false;
            dispenser_lib::dispenser::web_dispense();
            busy = false;
        }
        else if (ir_event)
        {
            noInterrupts();
            ir_event = false;
            interrupts();

            busy = true;
            waiting_msg = false;
            dispenser_lib::dispenser::handle_dog_detected();
            busy = false;
        }
    }

    yield();
}