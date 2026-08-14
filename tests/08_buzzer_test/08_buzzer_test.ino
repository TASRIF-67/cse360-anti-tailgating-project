/*
  TEST 08 - ACTIVE BUZZER MODULE

  This test assumes a normal 3-pin active buzzer module:

    VCC -> UNO 5V logic rail
    GND -> GND
    SIG -> UNO D9

  If you have a bare 2-pin buzzer, do NOT use this wiring blindly.
*/


const byte PIN_BUZZER = 9;

// Set false if your module sounds when SIG is LOW.
const bool BUZZER_ACTIVE_HIGH = true;


void setup()
{
    pinMode(PIN_BUZZER, OUTPUT);

    turnBuzzerOff();
}


void loop()
{
    turnBuzzerOn();

    delay(500);

    turnBuzzerOff();

    delay(1000);
}


void turnBuzzerOn()
{
    if (BUZZER_ACTIVE_HIGH == true)
    {
        digitalWrite(PIN_BUZZER, HIGH);
    }
    else
    {
        digitalWrite(PIN_BUZZER, LOW);
    }
}


void turnBuzzerOff()
{
    if (BUZZER_ACTIVE_HIGH == true)
    {
        digitalWrite(PIN_BUZZER, LOW);
    }
    else
    {
        digitalWrite(PIN_BUZZER, HIGH);
    }
}
