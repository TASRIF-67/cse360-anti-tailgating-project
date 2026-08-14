# Hardware Pin Map

## Arduino UNO

| Pin | Connection |
|---|---|
| D2 | Emergency push button |
| D3 | HX711 DOUT/DT |
| D4 | HX711 SCK/CLK |
| D5 | Gate A servo signal |
| D6 | Gate B servo signal |
| D7 | IR-A output |
| D8 | IR-B output |
| D9 | Active buzzer signal |
| D10 | RC522 SS/SDA through CD4050 |
| D11 | RC522 MOSI through CD4050 |
| D12 | RC522 MISO directly |
| D13 | RC522 SCK through CD4050 |
| A0 | RC522 RST through CD4050 |
| A1 | Gate A CLOSED limit switch |
| A2 | UNO RX from ESP32 GPIO1/TX |
| A3 | UNO TX through CD4050 to ESP32 GPIO3/RX |
| A4 | I2C SDA: LCD and PCF8574 |
| A5 | I2C SCL: LCD and PCF8574 |

## ESP32-AI-CAM UART

| ESP32 pin | Connection |
|---|---|
| GPIO1/TX | UNO A2 directly |
| GPIO3/RX | UNO A3 through CD4050 |
| GND | Common logic and servo ground |

Disconnect the UNO UART wires while flashing the ESP32-CAM when the programmer
also uses GPIO1 and GPIO3.

## Power and level shifting

- Power the UNO from USB and the two servos from a separate regulated 5 V rail.
- Join servo ground and logic ground, but do not join the separate servo +5 V
  rail to the UNO +5 V rail.
- Power the RC522 and CD4050 from 3.3 V.
- Level-shift UNO D10, D11, D13, A0, and A3 through the CD4050.
- Connect RC522 MISO to UNO D12 directly.

