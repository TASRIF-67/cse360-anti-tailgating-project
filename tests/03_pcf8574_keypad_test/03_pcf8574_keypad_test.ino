/*
  TEST 03 - PCF8574 + 4x4 KEYPAD

  Purpose:
  Verify the keypad wiring and every key.

  Wiring:
    PCF8574 SDA -> UNO A4
    PCF8574 SCL -> UNO A5
    PCF8574 VCC -> UNO 5V
    PCF8574 GND -> GND

    P0 -> R1
    P1 -> R2
    P2 -> R3
    P3 -> R4

    P4 -> C1
    P5 -> C2
    P6 -> C3
    P7 -> C4

  First run TEST 01 and replace PCF_ADDRESS.
*/

#include <Wire.h>


const byte PCF_ADDRESS = 0x20;


const char KEY_MAP[4][4] =
{
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};


void writePCF(byte value);
byte readPCF();
char scanRawKey();
char readDebouncedKey();


void setup()
{
    Serial.begin(9600);

    Wire.begin();

    writePCF(0xFF);

    Serial.println("PCF8574 keypad test.");
    Serial.println("Press each key.");
}


void loop()
{
    char key = readDebouncedKey();

    if (key != 0)
    {
        Serial.print("Pressed: ");
        Serial.println(key);
    }
}


void writePCF(byte value)
{
    Wire.beginTransmission(PCF_ADDRESS);

    Wire.write(value);

    Wire.endTransmission();
}


byte readPCF()
{
    byte value = 0xFF;

    Wire.requestFrom((int)PCF_ADDRESS, 1);

    if (Wire.available() > 0)
    {
        value = Wire.read();
    }

    return value;
}


char scanRawKey()
{
    byte row = 0;
    byte column = 0;

    for (row = 0; row < 4; row++)
    {
        byte outputState = 0xFF;

        bitClear(outputState, row);

        writePCF(outputState);

        delayMicroseconds(120);

        byte inputState = readPCF();

        for (column = 0; column < 4; column++)
        {
            byte columnBit = 4 + column;

            if (bitRead(inputState, columnBit) == 0)
            {
                writePCF(0xFF);

                return KEY_MAP[row][column];
            }
        }
    }

    writePCF(0xFF);

    return 0;
}


char readDebouncedKey()
{
    static char previousRaw = 0;
    static char stableKey = 0;
    static unsigned long changedAt = 0;

    char raw = scanRawKey();

    if (raw != previousRaw)
    {
        previousRaw = raw;

        changedAt = millis();
    }

    if (millis() - changedAt >= 40)
    {
        if (raw != stableKey)
        {
            stableKey = raw;

            if (stableKey != 0)
            {
                return stableKey;
            }
        }
    }

    return 0;
}
