
// positives are connected to the arduino digital pins
const int pulPin = 2;
const int dirPin = 3;
const int pulPerRev = 400;
const int microStep = 1;
const int upButtonPin = 5;   // blue
const int downButtonPin = 6; // orange
const int relayPin = 4;      // orange

void setup()
{
    pinMode(pulPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(relayPin, OUTPUT);

    pinMode(upButtonPin, INPUT);
    pinMode(downButtonPin, INPUT);

    Serial.begin(9600);
}

void loop()
{
    int upButtonState = digitalRead(upButtonPin);
    int downButtonState = digitalRead(downButtonPin);
    if (upButtonState == 1)
    {
        Serial.println("MOVING UP");
        // pull back solenoid
        digitalWrite(relayPin, HIGH);

        // move linear actuator up
        // direction 1 is towards the NON-motor-end.
        moveLinearActuator(1);
    }
    else if (downButtonState == 1)
    {
        Serial.println("MOVING DOWN");

        // move linear actuator down
        // direction 0 is towards the motor-end.
        moveLinearActuator(0);
    }
    else
    {
        digitalWrite(relayPin, LOW);
    }
    delayMicroseconds(300);
}

void moveLinearActuator(int direction)
{
    // set the direction of movement
    if (direction == 1)
    {
        digitalWrite(dirPin, HIGH); // towards the NON-motor end.
    }
    else if (direction == 0)
    {
        digitalWrite(dirPin, LOW); // towards the motor end.
    }

    for (int x = 0; x < pulPerRev; ++x)
    {
        digitalWrite(pulPin, HIGH);
        delayMicroseconds(500);
        digitalWrite(pulPin, LOW);
        delayMicroseconds(500);
    }
}
