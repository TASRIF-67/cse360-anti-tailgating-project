/*
  TEST 05 - FC-51 IR SENSORS

  Wiring:
    IR-A VCC -> 5V
    IR-A GND -> GND
    IR-A OUT -> D7

    IR-B VCC -> 5V
    IR-B GND -> GND
    IR-B OUT -> D8

  Purpose:
  Determine whether your modules output LOW or HIGH when blocked.

  Adjust the FC-51 potentiometer so the opposite shoebox wall
  does not cause permanent detection.
*/


const byte PIN_IR_A = 7;
const byte PIN_IR_B = 8;


void setup()
{
    Serial.begin(9600);

    pinMode(PIN_IR_A, INPUT);
    pinMode(PIN_IR_B, INPUT);

    Serial.println("IR sensor test.");
}


void loop()
{
    int valueA = digitalRead(PIN_IR_A);
    int valueB = digitalRead(PIN_IR_B);

    Serial.print("IR-A = ");

    if (valueA == HIGH)
    {
        Serial.print("HIGH");
    }
    else
    {
        Serial.print("LOW");
    }

    Serial.print("    IR-B = ");

    if (valueB == HIGH)
    {
        Serial.println("HIGH");
    }
    else
    {
        Serial.println("LOW");
    }

    delay(300);
}
