// Scripts attempts to read and write from one of the registers
// Mode register(0x09) is read/written 

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

    Serial.printf("Part ID = 0x%02X\n", readReg(0xFF));

    // Mode Configuration Register
    writeReg(0x09, 0x03);

    delay(10);

    uint8_t mode = readReg(0x09);

    Serial.printf("Mode Register = 0x%02X\n", mode);
}

void loop()
{
}