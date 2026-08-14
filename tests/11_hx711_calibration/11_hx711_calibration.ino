/*
  TEST 11 - HX711 LOAD-CELL CALIBRATION

  Run TEST 10 first.

  Before uploading:
    Change KNOWN_WEIGHT_GRAMS to the real weight you will use.

  Procedure:
    1. Keep chamber platform EMPTY at startup.
    2. Sketch tares the empty platform.
    3. Place the known weight when instructed.
    4. The sketch calculates an approximate calibration factor.
    5. Copy that factor into the final UNO firmware.
    6. Verify the printed weight is close to the known weight.

  HX711 -> UNO:
    DOUT -> D3
    SCK  -> D4

  TEST INPUT / ACTION:
    Set KNOWN_WEIGHT_GRAMS accurately. Start with the platform empty;
    when instructed, place that weight on it and leave it still.

  EXPECTED OUTPUT:
    Serial reports tare, waits 8 seconds, prints the unscaled average and
    approximate calibration factor, then repeats "Weight = N.N g".

  PASS CRITERIA / WHAT TO CHECK:
    Printed weight is close to the known value and stable. Record the
    factor, then measure empty, normal-model, and excessive-load ranges.*/

#include <HX711.h>


const byte PIN_HX711_DOUT = 3;
const byte PIN_HX711_SCK = 4;

// CHANGE THIS.
const float KNOWN_WEIGHT_GRAMS = 200.0;

HX711 scale;


void setup()
{
    Serial.begin(9600);

    scale.begin(PIN_HX711_DOUT, PIN_HX711_SCK);

    Serial.println("HX711 calibration.");
    Serial.println("Keep platform EMPTY.");

    if (scale.wait_ready_timeout(2000) == false)
    {
        Serial.println("ERROR: HX711 not detected.");

        while (true)
        {
        }
    }

    scale.set_scale();

    scale.tare();

    Serial.println("Empty platform tared.");
    Serial.println();

    Serial.print("Place ");
    Serial.print(KNOWN_WEIGHT_GRAMS);
    Serial.println(" g on the platform now.");

    Serial.println("Waiting 8 seconds...");

    delay(8000);

    float unscaledUnits = scale.get_units(10);

    Serial.print("Unscaled averaged value = ");
    Serial.println(unscaledUnits, 3);

    float calibrationFactor = unscaledUnits / KNOWN_WEIGHT_GRAMS;

    Serial.print("Approximate calibration factor = ");
    Serial.println(calibrationFactor, 6);

    scale.set_scale(calibrationFactor);

    Serial.println();
    Serial.println("Now printing calibrated weight.");
}


void loop()
{
    if (scale.wait_ready_timeout(1000) == true)
    {
        float grams = scale.get_units(5);

        Serial.print("Weight = ");
        Serial.print(grams, 1);
        Serial.println(" g");
    }
    else
    {
        Serial.println("HX711 not ready.");
    }

    delay(500);
}
