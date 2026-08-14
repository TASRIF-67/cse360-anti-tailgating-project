/*
  TEST 02 - 16x2 I2C LCD

  First run TEST 01 and replace LCD_ADDRESS below.

  Wiring:
    LCD VCC -> UNO 5V
    LCD GND -> GND
    LCD SDA -> UNO A4
    LCD SCL -> UNO A5
*/

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
