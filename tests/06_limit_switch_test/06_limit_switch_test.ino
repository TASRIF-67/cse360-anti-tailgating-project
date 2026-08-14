/*
  TEST 06 - GATE A CLOSED LIMIT SWITCH

  Final wiring:
    Switch COM -> GND
    Switch NO  -> UNO A1
    Switch NC  -> not used

  A1 uses INPUT_PULLUP.

  Expected:
    switch released -> HIGH -> Gate A NOT confirmed closed
    switch pressed  -> LOW  -> Gate A CLOSED confirmed

  TEST INPUT / ACTION:
    Open Serial Monitor at 9600 baud. Release the Gate A CLOSED switch,
    then press and release it several times by hand with the servo off.

  EXPECTED OUTPUT:
    Released prints "RELEASED -> Gate A NOT CLOSED"; pressed prints
    "PRESSED -> Gate A CLOSED".

  PASS CRITERIA / WHAT TO CHECK:
    The message changes immediately and consistently with switch state.
    Gate A must physically press this switch at its closed servo angle.*/


const byte PIN_GATE_A_CLOSED = A1;


void setup()
{
    Serial.begin(9600);

    pinMode(PIN_GATE_A_CLOSED, INPUT_PULLUP);

    Serial.println("Gate A CLOSED switch test.");
}


void loop()
{
    int value = digitalRead(PIN_GATE_A_CLOSED);

    if (value == LOW)
    {
        Serial.println("PRESSED -> Gate A CLOSED");
    }
    else
    {
        Serial.println("RELEASED -> Gate A NOT CLOSED");
    }

    delay(250);
}
