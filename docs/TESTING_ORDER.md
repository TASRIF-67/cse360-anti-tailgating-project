# Hardware Testing Order

Run one isolated test at a time before uploading the final firmware.

1. `01_i2c_scanner`: record the LCD and PCF8574 addresses.
2. `02_lcd_test`: verify both LCD rows and the backlight.
3. `03_pcf8574_keypad_test`: verify every key and row/column order.
4. `04_rc522_uid_reader`: record each authorized card UID.
5. `05_ir_sensor_test`: determine whether each sensor is active LOW or HIGH.
6. `06_limit_switch_test`: verify the Gate A CLOSED switch polarity.
7. `07_emergency_button_test`: verify released and pressed states.
8. `08_buzzer_test`: confirm the buzzer active level.
9. `09_servo_calibration`: record open and closed angles for both gates.
10. `10_hx711_raw_test`: confirm stable readings that respond to load.
11. `11_hx711_calibration`: calculate the calibration factor and measure empty,
    expected, and excessive-load ranges.
12. `12_uno_esp32_uart_test`: upload both side-specific sketches and verify the
    `UNO_PING`/`ESP_PONG` exchange.
13. `13_esp32_camera_wifi_test`: verify camera capture and the local web page.
14. Upload both final sketches from `firmware/` and test the full interlock.

Before final operation, update I2C addresses, authorized UIDs and PINs, servo
angles, sensor polarity, Wi-Fi credentials, and load-cell calibration values.
Keep `LOAD_CELL_CONFIGURED` false until calibration is complete.

