/*
  TEST 10 - HX711 RAW READING

  HX711 -> UNO:
    VCC  -> 5V logic rail
    GND  -> GND
    DOUT -> D3
    SCK  -> D4

  Planned load-cell wiring:
    Red   -> E+
    Black -> E-
    White -> A+
    Green -> A-

  Verify the actual sensor before wiring because wire colors can vary.

  Purpose:
  Only verify that:
    - HX711 responds
    - raw number is reasonably stable when empty
    - number changes when weight is added
    - number returns toward the original region when removed

  TEST INPUT / ACTION:
    Open Serial Monitor at 9600 baud. Observe the empty platform, add a
    known object, remove it, and repeat without touching other supports.

  EXPECTED OUTPUT:
    "Raw average = N" repeats; values change significantly with load and
    return near the original empty region after removal.

  PASS CRITERIA / WHAT TO CHECK:
    Readings are reasonably stable, respond in a consistent direction,
    and recover after unloading. "HX711 NOT READY" is a failure.*/

#include <HX711.h>


const byte PIN_HX711_DOUT = 3;
const byte PIN_HX711_SCK = 4;

HX711 scale;


void setup()
{
    Serial.begin(9600);

    scale.begin(PIN_HX711_DOUT, PIN_HX711_SCK);

    Serial.println("HX711 raw test.");
}


void loop()
{
    if (scale.wait_ready_timeout(1000) == false)
    {
        Serial.println("HX711 NOT READY");

        delay(500);

        return;
    }

    long rawValue = scale.read_average(10);

    Serial.print("Raw average = ");
    Serial.println(rawValue);

    delay(400);
}
