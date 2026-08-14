/*
  TEST 12A - UNO SIDE OF UNO <-> ESP32 UART

  Wiring:
    ESP32 TX / GPIO1 -> UNO A2 directly

    UNO A3
      -> CD4050 pin 11
      -> CD4050 pin 12
      -> ESP32 RX / GPIO3

    common GND required

  This sends UNO_PING every 2 seconds and prints anything
  received from the ESP32.

  Upload the matching ESP32 test sketch too.

  TEST INPUT / ACTION:
    Upload both 12A and 12B sketches, reconnect UART after ESP32 flashing,
    share ground, and open the UNO Serial Monitor at 9600 baud.

  EXPECTED OUTPUT:
    UNO prints "UNO -> ESP32: UNO_PING" about every 2 seconds and receives
    "ESP_PONG" from the ESP32 (an initial ESP_UART_READY may also appear).

  PASS CRITERIA / WHAT TO CHECK:
    Every repeated UNO_PING gets an ESP_PONG without corrupted characters.
    Missing replies indicate RX/TX, CD4050, ground, or baud-rate problems.*/

#include <SoftwareSerial.h>


const byte PIN_ESP_RX = A2;
const byte PIN_ESP_TX = A3;

SoftwareSerial espSerial(PIN_ESP_RX, PIN_ESP_TX);


void setup()
{
    Serial.begin(9600);

    espSerial.begin(9600);

    Serial.println("UNO UART test started.");
}


void loop()
{
    static unsigned long lastPingTime = 0;

    if (millis() - lastPingTime >= 2000)
    {
        lastPingTime = millis();

        espSerial.println("UNO_PING");

        Serial.println("UNO -> ESP32: UNO_PING");
    }

    while (espSerial.available() > 0)
    {
        char c = espSerial.read();

        Serial.write(c);
    }
}
