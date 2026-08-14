# Anti-Tailgating Security Interlock Gate

GitHub-ready code structure for the shoebox prototype.

The Arduino UNO is the safety authority; the ESP32-AI-CAM provides camera, Wi-Fi, and event-monitoring functions only. Start with [`docs/TESTING_ORDER.md`](docs/TESTING_ORDER.md), and use [`docs/PIN_MAP.md`](docs/PIN_MAP.md) while wiring the prototype.

## Why the code is split

There are **two microcontrollers**, so the final project should use **two final sketches**:

- `firmware/uno_main_controller/uno_main_controller.ino`
  - Main safety controller
  - RFID + PIN authentication
  - IR sensors
  - HX711 + load cell
  - Gate A / Gate B servos
  - Gate A CLOSED switch
  - Emergency button
  - Buzzer
  - Sends event messages to ESP32

- `firmware/esp32_ai_cam/esp32_ai_cam.ino`
  - Camera
  - Wi-Fi
  - Local status page
  - Receives UNO events
  - Takes alert photos
  - No gate-opening authority

The `tests/` folder contains small sketches that should be run **before** the final firmware.

For the real-hardware procedure and evaluator demo, follow [`docs/PRACTICAL_DEMO_GUIDE.md`](docs/PRACTICAL_DEMO_GUIDE.md).

## Recommended testing order

1. `01_i2c_scanner`
2. `02_lcd_test`
3. `03_pcf8574_keypad_test`
4. `04_rc522_uid_reader`
5. `05_ir_sensor_test`
6. `06_limit_switch_test`
7. `07_emergency_button_test`
8. `08_buzzer_test`
9. `09_servo_calibration`
10. `10_hx711_raw_test`
11. `11_hx711_calibration`
12. `12_uno_esp32_uart_test`
13. `13_esp32_camera_wifi_test`
14. Final UNO + ESP32 firmware

## Final UNO pin map

| UNO pin | Component |
|---|---|
| D2 | Emergency button |
| D3 | HX711 DOUT |
| D4 | HX711 SCK |
| D5 | Gate A SG90 signal |
| D6 | Gate B SG90 signal |
| D7 | IR-A OUT |
| D8 | IR-B OUT |
| D9 | Active buzzer SIG |
| D10 | RC522 SS/SDA through CD4050 |
| D11 | RC522 MOSI through CD4050 |
| D12 | RC522 MISO direct |
| D13 | RC522 SCK through CD4050 |
| A0 | RC522 RST through CD4050 |
| A1 | Gate A CLOSED switch |
| A2 | UNO RX from ESP32 TX |
| A3 | UNO TX through CD4050 to ESP32 RX |
| A4 | I2C SDA |
| A5 | I2C SCL |

## Important power rules

- Power bank -> Arduino UNO through USB.
- UNO 5V logic rail -> LCD, PCF8574, HX711, IR sensors and 3-pin buzzer module.
- UNO 3.3V -> RC522 and CD4050 only.
- Separate regulated 5V source -> two SG90 servos.
- Servo ground and logic ground must be common.
- Do **not** join the separate servo +5V rail to the UNO +5V rail.
- 2200 uF capacitor goes across servo +5V and servo GND.
- 0.1 uF capacitor goes across CD4050 VCC and GND.
- Missing fuse does not stop the prototype from working. Leave the empty holder unused.
- RTC and microSD are intentionally not used.

## Libraries

### UNO

Built in:
- `Wire`
- `SPI`
- `Servo`
- `SoftwareSerial`

Install with Arduino Library Manager:
- `MFRC522`
- `HX711` by bogde
- a `LiquidCrystal_I2C` library compatible with your LCD backpack

### ESP32

Install the Espressif `esp32` board platform. The ESP32 code uses:
- `WiFi`
- `WebServer`
- `esp_camera`

## Values you must discover before final firmware

The test sketches help you obtain:

- LCD I2C address
- PCF8574 I2C address
- RFID UID
- IR active level
- Gate A open/closed angle
- Gate B open/closed angle
- HX711 calibration factor
- normal one-model load range
- empty load threshold

Do not guess these values.
