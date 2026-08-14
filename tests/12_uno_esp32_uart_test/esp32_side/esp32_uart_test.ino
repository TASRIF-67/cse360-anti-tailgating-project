/*
  TEST 12B - ESP32-CAM SIDE OF UNO <-> ESP32 UART

  This sketch uses the ESP32-CAM normal UART0 pins:
    GPIO3 / U0R = RX from UNO through CD4050
    GPIO1 / U0T = TX to UNO A2

  IMPORTANT:
  Disconnect the UNO UART wires while flashing the ESP32 if your
  programmer also uses GPIO1/GPIO3.

  After normal boot, reconnect the UNO UART wires.

  The ESP32 replies:
    ESP_PONG
  whenever it receives:
    UNO_PING
*/


String line = "";


void setup()
{
    Serial.begin(9600);

    delay(800);

    Serial.println("ESP_UART_READY");
}


void loop()
{
    while (Serial.available() > 0)
    {
        char c = Serial.read();

        if (c == '\r')
        {
            continue;
        }

        if (c == '\n')
        {
            if (line == "UNO_PING")
            {
                Serial.println("ESP_PONG");
            }

            line = "";
        }
        else
        {
            line += c;
        }
    }
}
