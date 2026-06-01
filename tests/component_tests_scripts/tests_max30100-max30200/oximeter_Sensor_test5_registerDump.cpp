// After initialization, fifo should increase

#include <Arduino.h>
#include <Wire.h>

#define MAX_ADDR 0x57

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

    Serial.println("Register dump:");

    for(uint8_t reg = 0x00; reg <= 0x10; reg++)
    {
        Serial.print("0x");

        if(reg < 16)
            Serial.print("0");

        Serial.print(reg, HEX);

        Serial.print(" = 0x");

        uint8_t value = readReg(reg);

        if(value < 16)
            Serial.print("0");

        Serial.println(value, HEX);

        delay(10);
    }

    Serial.print("Part ID = 0x");
    Serial.println(readReg(0xFF), HEX);
}

void loop()
{
}