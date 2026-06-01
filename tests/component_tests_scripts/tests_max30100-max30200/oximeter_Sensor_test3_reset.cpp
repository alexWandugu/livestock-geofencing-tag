// To check whether we can write the reset bit self-clears
// Typical Output:
    // Mode = 0x03
    // Mode after reset = 0x40
    // Mode final = 0x03
// This means the chip is still alive and writable, but the reset bit implementation is broken
// Note if Mode after reset = 0x00, the reset implementation works just takes a while longer

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

uint8_t readReg(uint8_t reg)
{
    Wire.beginTransmission(MAX_ADDR);
    Wire.write(reg);

    if (Wire.endTransmission(false) != 0)
        return 0xFF;

    Wire.requestFrom((uint8_t)MAX_ADDR, (uint8_t)1);

    if (Wire.available())
        return Wire.read();

    return 0xFF;
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Wire.begin(21,22);

    Serial.printf("Part ID: 0x%02X\n", readReg(0xFF));

    Serial.println("\nWriting 0x03...");
    writeReg(0x09, 0x03);

    delay(10);

    Serial.printf("Mode = 0x%02X\n", readReg(0x09));

    Serial.println("\nWriting reset...");
    writeReg(0x09, 0x40);

    delay(100);

    Serial.printf("Mode after reset = 0x%02X\n", readReg(0x09));

    Serial.println("\nTrying to write 0x03 again...");
    writeReg(0x09, 0x03);

    delay(10);

    Serial.printf("Mode final = 0x%02X\n", readReg(0x09));
}

void loop()
{
}