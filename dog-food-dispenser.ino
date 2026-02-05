#include <Arduino.h>
#include <SoftwareSerial.h>

static const uint8_t RFID_RX_PIN = D5;
static const int RFID_TX_PIN = -1; // RX only
static const uint32_t BAUD = 9600;

static const uint16_t FRAME_LEN = 30;
static const uint16_t QUIET_MS = 60;
static const uint16_t OVERALL_MS = 400;

// RX=D5, TX non utilisé
SoftwareSerial rfidSerial(RFID_RX_PIN, RFID_TX_PIN);

// Draine l'entrée jusqu'à une période "quiet" sans octets
static void drain_input(Stream &s, uint32_t quiet_ms)
{
    uint32_t last = millis();
    while (millis() - last < quiet_ms)
    {
        while (s.available() > 0)
        {
            s.read();
            last = millis();
        }
        yield();
    }
}

// Lit une trame “burst” : on attend 1er octet, puis on collecte jusqu’au silence (QUIET_MS) ou timeout global.
static int readFrame(uint8_t *buf, int maxLen)
{
    uint32_t start = millis();
    uint32_t last = start;
    int n = 0;

    // Attendre le 1er octet
    while (!rfidSerial.available())
    {
        if (millis() - start > OVERALL_MS)
            return 0;
        yield();
    }

    // Collecte
    while (millis() - start < OVERALL_MS)
    {
        while (rfidSerial.available())
        {
            uint8_t b = (uint8_t)rfidSerial.read();
            if (n < maxLen)
                buf[n++] = b;
            last = millis();
        }
        if (n > 0 && (millis() - last) > QUIET_MS)
            break;
        yield();
    }
    return n;
}

// Extrait le payload ASCII (26 chars) si la trame correspond au format STX...ETX
static bool extractTagFromFrame30(const uint8_t *buf, int n, char outTag[27], uint8_t &crc1, uint8_t &crc2)
{
    if (n != FRAME_LEN)
        return false;
    if (buf[0] != 0x02)
        return false;
    if (buf[29] != 0x03)
        return false;

    // Copie les 26 caractères ASCII (1..26)
    for (int i = 0; i < 26; i++)
        outTag[i] = (char)buf[1 + i];
    outTag[26] = '\0';

    // 2 octets binaires
    crc1 = buf[27];
    crc2 = buf[28];

    return true;
}

void setup()
{
    Serial.begin(115200);
    delay(50);

    rfidSerial.begin(BAUD);
    pinMode(RFID_RX_PIN, INPUT);

    Serial.println();
    Serial.println("RFID reader ready. Present a tag...");
}

void loop()
{
    uint8_t frame[FRAME_LEN];
    int n = readFrame(frame, FRAME_LEN);
    if (n <= 0)
        return;

    char tag[27];
    uint8_t c1 = 0, c2 = 0;

    if (extractTagFromFrame30(frame, n, tag, c1, c2))
    {
        Serial.print("Tag=");
        Serial.println(tag);

        Serial.print("CRC/Status=");
        Serial.print(c1, HEX);
        Serial.print(" ");
        Serial.println(c2, HEX);
    }
    else
    {
        Serial.printf("Unexpected frame (%d bytes)\nHEX: ", n);
        for (int i = 0; i < n; i++)
        {
            Serial.printf("%02X ", frame[i]);
        }
        Serial.print("\nASCII: ");
        for (int i = 0; i < n; i++)
        {
            char c = (char)frame[i];
            Serial.print((c >= 32 && c <= 126) ? c : '.');
        }
        Serial.println();

        drain_input(rfidSerial, 20);
    }
}
