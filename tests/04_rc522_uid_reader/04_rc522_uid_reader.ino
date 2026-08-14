/*
  TEST 04 - RC522 RFID UID READER

  Use the FINAL RC522 wiring.

  RC522 power:
    VCC -> UNO 3.3V
    GND -> common GND

  RC522 signals:
    UNO D10 -> CD4050 -> RC522 SDA/SS
    UNO D11 -> CD4050 -> RC522 MOSI
    UNO D12 ----------> RC522 MISO directly
    UNO D13 -> CD4050 -> RC522 SCK
    UNO A0  -> CD4050 -> RC522 RST

  Open Serial Monitor at 9600 baud.
  Tap your card and copy the UID.

  TEST INPUT / ACTION:
    Open Serial Monitor at 9600 baud and place each RFID card/tag near
    the RC522 antenna, one at a time.

  EXPECTED OUTPUT:
    Startup prints readiness text. Each detected card prints "UID:"
    followed by its hexadecimal UID bytes separated by spaces.

  PASS CRITERIA / WHAT TO CHECK:
    The same card produces the same complete UID on repeated taps, and
    each intended card is detected. Record authorized UIDs for firmware.*/

#include <SPI.h>
#include <MFRC522.h>


const byte PIN_RFID_SS = 10;
const byte PIN_RFID_RST = A0;

MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);


void setup()
{
    Serial.begin(9600);

    SPI.begin();

    rfid.PCD_Init();

    Serial.println("RC522 UID reader ready.");
    Serial.println("Tap a card or tag.");
}


void loop()
{
    if (rfid.PICC_IsNewCardPresent() == false)
    {
        return;
    }

    if (rfid.PICC_ReadCardSerial() == false)
    {
        return;
    }

    Serial.print("UID: ");

    byte i = 0;

    for (i = 0; i < rfid.uid.size; i++)
    {
        if (rfid.uid.uidByte[i] < 0x10)
        {
            Serial.print('0');
        }

        Serial.print(rfid.uid.uidByte[i], HEX);

        if (i + 1 < rfid.uid.size)
        {
            Serial.print(' ');
        }
    }

    Serial.println();

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    delay(800);
}
