/*
  TEST 08 - ACTIVE BUZZER MODULE

  This test assumes a normal 3-pin active buzzer module:

    VCC -> UNO 5V logic rail
    GND -> GND
    SIG -> UNO D9

  If you have a bare 2-pin buzzer, do NOT use this wiring blindly.

  TEST INPUT / ACTION:
    Upload and power the 3-pin active buzzer module. No Serial Monitor
    or keyboard input is required.

  EXPECTED OUTPUT:
    The buzzer sounds for about 0.5 seconds, stays silent for about
    1.0 second, and repeats.

  PASS CRITERIA / WHAT TO CHECK:
    The pattern is clear and repeatable. If inverted, verify wiring and
    change BUZZER_ACTIVE_HIGH to match the module.*/


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
