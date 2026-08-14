/*
  TEST 07 - EMERGENCY PUSH BUTTON

  Wiring:
    one electrical side -> UNO D2
    opposite side       -> GND

  D2 uses INPUT_PULLUP.

  If your push button has four pins, use a multimeter first to
  identify the two electrical sides. Two pins on the same side
  are often internally connected.

  Expected:
    released -> HIGH
    pressed  -> LOW

  TEST INPUT / ACTION:
    Open Serial Monitor at 9600 baud. Leave the emergency button
    released, press and hold it, then release it several times.

  EXPECTED OUTPUT:
    Released prints "Button released"; pressed prints
    "EMERGENCY BUTTON PRESSED".

  PASS CRITERIA / WHAT TO CHECK:
    Pressed always reads LOW and released always reads HIGH, with no
    stuck or intermittent state.*/


const byte PIN_EMERGENCY = 2;


void setup()
{
    Serial.begin(9600);

    pinMode(PIN_EMERGENCY, INPUT_PULLUP);

    Serial.println("Emergency button test.");
}


void loop()
{
    if (digitalRead(PIN_EMERGENCY) == LOW)
    {
        Serial.println("EMERGENCY BUTTON PRESSED");
    }
    else
    {
        Serial.println("Button released");
    }

    delay(200);
}
