// Test for the LED drivers of the new 'Oximeter'

#include <Arduino.h>
#include <Wire.h>

#define MAX_ADDR 0x57

void writeReg(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(MAX_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Wire.begin(21,22);

    // Red LED current = max
    writeReg(0x0C, 0xFF);

    // IR LED current = max
    writeReg(0x0D, 0xFF);

    // SPO2 mode
    writeReg(0x09, 0x03);

    Serial.println("Sensor configured.");
    Serial.println("Look through a phone camera at the sensor.");
}

void loop()
{
}