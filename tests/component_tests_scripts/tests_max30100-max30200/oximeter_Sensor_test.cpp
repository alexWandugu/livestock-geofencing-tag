// After checking the part id, I found the new 'Oximeter' was actually a particle sensor
// This script attempts to communicate with the module and collect infrared and red data point
// The same infrared and red data points are used to calculate oximetry data

#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"

MAX30105 particleSensor;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("=== MAX3010x Live FIFO Test ===");

    Wire.begin(21, 22);
    Wire.setClock(100000);

    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD))
    {
        Serial.println("MAX3010x not found!");
        while (1)
        {
            delay(1000);
        }
    }

    Serial.println("Sensor found.");

    // Configuration:
    // LED brightness, sample average, LED mode,
    // sample rate, pulse width, ADC range

    byte ledBrightness = 0x1F;   // 0-255
    byte sampleAverage = 4;
    byte ledMode = 2;            // 2 = Red + IR
    int sampleRate = 100;        // Hz
    int pulseWidth = 411;        // max resolution
    int adcRange = 4096;

    particleSensor.setup(
        ledBrightness,
        sampleAverage,
        ledMode,
        sampleRate,
        pulseWidth,
        adcRange);

    particleSensor.enableDIETEMPRDY();

    Serial.println();
    Serial.println("IR\t\tRED");
}

void loop()
{
    uint32_t ir = particleSensor.getIR();
    uint32_t red = particleSensor.getRed();

    Serial.print("IR=");
    Serial.print(ir);

    Serial.print("  RED=");
    Serial.print(red);

    if(ir > 50000)
        Serial.print("  Finger detected");

    Serial.println();

    delay(100);
}