
/* The working principles of ultrasound sensors
 *  They send ultrasound waves through the 'trig' pin
 *  They receive the waves through the 'echo' pin
 *  They return the time it took the pulses to travel to and back from the object in microseconds
 *  v_sound = 340 m/s
 *  v_sound = 0.0340 cm/micro_s
 *  distance = speed * (time/2)   // because we only need the time to OR from the object and not both 
 */

#include <HX711_ADC.h>
#include <EEPROM.h>

const int echoPin = 7;
const int trigPin = 8;
const int recordButtonPin = 2;
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 3;  //mcu > HX711 sck pin

HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 4;

char distanceUnitStr[] = "mm"; // C-type string
char forceUnitStr = 'N';       // character

const double soundSpeed = 0.034;
const int vibrationSampleTime = 20; //ms
const int deflectDelayTime = 150;   //ms

// prevent auto-scalling when plotting the vibration
const int minPlotVal = -25; // change on Monday
const int maxPlotVal = 25;

int experType = 0; // default is 0, user has not chosen an experiment

void setup()
{
    Serial.begin(57600);
    delay(10);
    Serial.println();
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    pinMode(recordButtonPin, INPUT);

    LoadCell.begin();
    float calibrationValue; // calibration value (see example file "Calibration.ino")
//  calibrationValue = 696.0; // uncomment this if you want to set the calibration value in the sketch
#if defined(ESP8266) || defined(ESP32)
    //EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
#endif
    EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

    long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
    boolean _tare = true;        //set this to false if you don't want tare to be performed in the next step
    LoadCell.start(stabilizingtime, _tare);
    if (LoadCell.getTareTimeoutFlag())
    {
        Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
        while (1)
            ;
    }
    else
    {
        LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
        Serial.println("Startup is complete");
    }
}

void loop()
{
    if (experType == 0)
    {
        printIntroProgram();
        while (Serial.available() == 0) {}

        delay(2);

        char experChoice = 0;
        experChoice = Serial.read();
        serialFlush();

        if ((experChoice >= '1') && (experChoice <= '3'))
        {
            experType = experChoice - '0';

            Serial.print("Number received: ");
            Serial.println(experType);
        }
        else
        {
            Serial.println("Input was not 1, 2, or 3.");
        }

        Serial.print("\n\n");
        delay(1000); // unnecessary delay
    }
    // user chose Deflection
    else if (experType == 1)
    {
        const int numDatapoints = 5;
        printIntroDeflect(numDatapoints);

        double distArr[numDatapoints] = {0};
        double forceArr[numDatapoints] = {0};
        int i = 0;
        double dist = 0.0, force = 0.0;

        while (i < numDatapoints)
        {
            int recordButtonState = digitalRead(recordButtonPin);
            delay(deflectDelayTime);
            if (recordButtonState == 1)
            {
                dist = getDistance();
                force = getForce();

                distArr[i] = dist;
                forceArr[i] = force;

                printDistForce(i + 1, numDatapoints, dist, force);
                ++i;
            }
        }

        delay(1000); // unnecessary delay
        printDistForceArrs(numDatapoints, distArr, forceArr);
        experType = 4;
    }
    // user chose Vibration Procedure
    else if (experType == 2)
    {
        printIntroVibration();
        while (Serial.available() == 0) {}

        serialFlush();
        delay(2);
        experType = 0;
    }
    // user chose Vibration Plotting
    else if (experType == 3)
    {
        plotVibration();
        experType = 4;
    }
    else if (experType == 4)
    {
        Serial.println("Experiment has finished");
        Serial.println("\n*** Type any key then press 'ENTER' to return to the main menue.\n\n");
        while (Serial.available() == 0) {}

        delay(2);
        serialFlush();
        Serial.print("\n\n\n*********************\n\n\n");
        experType = 0;
    }
}

void plotVibration()
{
    double displacement = 0;
    while (true)
    {
        if (Serial.available() > 0)
        {
            serialFlush();
            break;
        }
        displacement = getDistance();
        Serial.print(displacement);
        Serial.print(",");
        Serial.print(minPlotVal);
        Serial.print(",");
        Serial.print(maxPlotVal);
        Serial.println();
        delay(vibrationSampleTime);
    }
}

double getDistance()
{

    digitalWrite(trigPin, LOW); // make sure the trig is cleared
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    double distance;
    long int travelTime;

    travelTime = pulseIn(echoPin, HIGH);          // in micro seconds
    distance = (travelTime / 2.0) * soundSpeed;   // in cm
    double distance_mm = (distance * 10) - 225.0; // ruler at rest is 224.4-225.5mm distance

    return distance_mm;
}

void printDistForceArrs(const int numDatapoints, double *const distArray, double *const forceArray)
{
    Serial.println("The recorded datapoints are:");
    delay(500); // unnecessary delay
    for (int j = 0; j < numDatapoints; ++j)
    {
        Serial.print("(");
        Serial.print(distArray[j]);
        Serial.print(distanceUnitStr);
        Serial.print(", ");
        Serial.print(forceArray[j], 4);
        Serial.print(forceUnitStr);

        if (j < (numDatapoints - 1))
        {
            Serial.print("), ");
        }
        else
        {
            Serial.println(")\n\n");
        }
    }
}

void printDistForce(const int currentCount, const int numbDatapoints, double const dist, double const force)
{
    Serial.print("Datapoint ");
    Serial.print(currentCount);
    Serial.print("/");
    Serial.print(numbDatapoints);
    Serial.print(" - (D: ");
    Serial.print(dist);
    Serial.print(distanceUnitStr);
    Serial.print(", F: ");
    Serial.print(force, 4);
    Serial.print(forceUnitStr);
    Serial.println(")");
}

void printIntroProgram()
{
    Serial.println("Choose your experiment:\n");
    Serial.println("");
    Serial.println("\t 1 - Deflection (Procedure & Experiment).");
    Serial.println("\t 2 - Vibration (Procedure Only).");
    Serial.println("\t 3 - Vibration (Exeriment Only, USE 'SERIAL PLOTTER').");
    Serial.println("\n* To select an experiment, type the number in input field and press 'ENTER'.");
    Serial.println("* To Restart the experiment at any time, close the 'Serial Monitor' and re-open it.");
}

void printIntroDeflect(const int numDatapoints)
{
    Serial.println("Deflection Measurement.\n");
    Serial.println("\t - Units are millimeter & Newton.");
    Serial.println("\t - After moving the load cell press record button to see & save a datapoint.");
    Serial.print("\t - You can record ");
    Serial.print(numDatapoints);
    Serial.println(" datapoints.");
    Serial.println("\t - A datapoint is a pair of (Distance, Force).");
}

void printIntroVibration()
{
    Serial.println("Vibration Measurement. In the main menue (read the full instructions first)\n");
    Serial.println("\t - Close the 'Serial Monitor' (this window) and open the 'Serial Plotter' from the 'tools' tab.");
    Serial.println("\t - (if you want) you could adjust the window size. Axis are auto-scaled.");
    Serial.println("\t - In the input field, type '3'; then, press 'ENTER' (on your keyboard).");
    Serial.println("\t - Initiate the vibration by moving the Linear Actuator.");
    Serial.println("\t - Input any key into the input-field to stop the plotting.");
    Serial.println("\t - After that, you can take a screenshot of the plot.\n");
    Serial.println("\t - Close the 'Serial Monitor' after you finish.");
    Serial.println("\n*** Type any key then press 'ENTER' to return to the main menue.\n\n");
}

double getForce()
{
    double mass;

    for (int j = 0; j < 10; j++)
    {
        while (!LoadCell.update()) {} // wait until there is new data

        mass = LoadCell.getData();
    }

    return (mass / 1000) * 9.81;
}

void serialFlush()
{
    while (Serial.available() > 0)
    {
        char trash = Serial.read();
        delay(2);
    }
}
