/*
  TEST 02 - 16x2 I2C LCD

  First run TEST 01 and replace LCD_ADDRESS below.

  Wiring:
    LCD VCC -> UNO 5V
    LCD GND -> GND
    LCD SDA -> UNO A4
    LCD SCL -> UNO A5

  TEST INPUT / ACTION:
    Set LCD_ADDRESS to the scanner result, upload, and power the LCD.
    No Serial Monitor or keyboard input is required.

  EXPECTED OUTPUT:
    Backlight turns on. Row 1 shows "LCD TEST OK" and row 2 shows
    "Anti-Tailgate".

  PASS CRITERIA / WHAT TO CHECK:
    Both lines are readable, complete, stable, and correctly positioned.
    Blank blocks usually mean contrast/address/wiring needs correction.*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>


// CHANGE THIS after the I2C scanner.
const byte LCD_ADDRESS = 0x27;

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);


void setup()
{
    Wire.begin();

    lcd.init();
    lcd.backlight();

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("LCD TEST OK");

    lcd.setCursor(0, 1);
    lcd.print("Anti-Tailgate");
}


void loop()
{
}
