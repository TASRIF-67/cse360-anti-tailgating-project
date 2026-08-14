/*
  TEST 09 - SG90 SERVO CALIBRATION

  Signals:
    Gate A -> D5
    Gate B -> D6

  Power:
    Servo red   -> separate regulated 5V servo rail
    Servo brown -> servo GND rail
    Servo signal -> UNO D5/D6

  IMPORTANT:
    common GND must connect servo GND to UNO GND.
    Do NOT power the two servos from UNO 5V.

  Serial commands:
    1 = select Gate A
    2 = select Gate B
    + = angle +5
    - = angle -5
    p = print angles

  Find four values:
    Gate A closed
    Gate A open
    Gate B closed
    Gate B open
*/

#include <Servo.h>


const byte PIN_SERVO_A = 5;
const byte PIN_SERVO_B = 6;

Servo gateA;
Servo gateB;

int angleA = 20;
int angleB = 20;

byte selectedGate = 1;


void setup()
{
    Serial.begin(9600);

    gateA.attach(PIN_SERVO_A);
    gateB.attach(PIN_SERVO_B);

    gateA.write(angleA);
    gateB.write(angleB);

    Serial.println("Servo calibration.");
    Serial.println("1=A, 2=B, +=increase, -=decrease, p=print.");
}


void loop()
{
    if (Serial.available() == 0)
    {
        return;
    }

    char command = Serial.read();

    if (command == '1')
    {
        selectedGate = 1;

        Serial.println("Selected Gate A.");
    }
    else if (command == '2')
    {
        selectedGate = 2;

        Serial.println("Selected Gate B.");
    }
    else if (command == '+')
    {
        increaseSelectedAngle();
    }
    else if (command == '-')
    {
        decreaseSelectedAngle();
    }
    else if (command == 'p')
    {
        printAngles();
    }
}


void increaseSelectedAngle()
{
    if (selectedGate == 1)
    {
        angleA = angleA + 5;

        if (angleA > 180)
        {
            angleA = 180;
        }

        gateA.write(angleA);
    }
    else
    {
        angleB = angleB + 5;

        if (angleB > 180)
        {
            angleB = 180;
        }

        gateB.write(angleB);
    }

    printAngles();
}


void decreaseSelectedAngle()
{
    if (selectedGate == 1)
    {
        angleA = angleA - 5;

        if (angleA < 0)
        {
            angleA = 0;
        }

        gateA.write(angleA);
    }
    else
    {
        angleB = angleB - 5;

        if (angleB < 0)
        {
            angleB = 0;
        }

        gateB.write(angleB);
    }

    printAngles();
}


void printAngles()
{
    Serial.print("Gate A angle = ");
    Serial.println(angleA);

    Serial.print("Gate B angle = ");
    Serial.println(angleB);

    Serial.println();
}
