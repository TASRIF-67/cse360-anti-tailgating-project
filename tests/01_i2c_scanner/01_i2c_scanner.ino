/*
  TEST 01 - I2C SCANNER

  Purpose:
  Find the real I2C addresses of:
    1. 16x2 LCD backpack
    2. PCF8574 keypad expander

  Wiring:
    UNO A4 -> LCD SDA + PCF8574 SDA
    UNO A5 -> LCD SCL + PCF8574 SCL
    UNO 5V -> LCD VCC + PCF8574 VCC
    UNO GND -> LCD GND + PCF8574 GND

  Open Serial Monitor at 9600 baud.

  TEST INPUT / ACTION:
    Power the wired LCD backpack and PCF8574, then open Serial Monitor
    at 9600 baud. No keyboard input is required.

  EXPECTED OUTPUT:
    "Scanning I2C bus...", one "Found device at 0xNN" line per
    responding module, and "Total devices found: N" every cycle.

  PASS CRITERIA / WHAT TO CHECK:
    Both modules appear consistently at stable, different addresses.
    "No I2C devices found" or changing addresses means wiring/power failed.*/

#include <Wire.h>


void setup()
{
    Serial.begin(9600);

    Wire.begin();

    Serial.println("I2C scanner started.");
}


void loop()
{
    byte address = 0;
    byte foundCount = 0;

    Serial.println();
    Serial.println("Scanning I2C bus...");

    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);

        byte error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.print("Found device at 0x");

            if (address < 16)
            {
                Serial.print('0');
            }

            Serial.println(address, HEX);

            foundCount++;
        }
    }

    if (foundCount == 0)
    {
        Serial.println("No I2C devices found.");
    }
    else
    {
        Serial.print("Total devices found: ");
        Serial.println(foundCount);
    }

    delay(3000);
}
