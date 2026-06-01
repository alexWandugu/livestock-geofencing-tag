// If the ADC is running, the write pointer should advance:
    // WR=3 RD=0
    // WR=7 RD=0
    // WR=12 RD=0
    // WR=16 RD=0
// Poor result:
    // WR=0 RD=0
    // WR=0 RD=0
    // WR=0 RD=0

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
    Wire.setClock(100000);

    Serial.println("\n=== Manual Configuration Test ===");

    Serial.printf("Part ID = 0x%02X\n", readReg(0xFF));

    // FIFO pointers
    writeReg(0x04, 0x00);
    writeReg(0x05, 0x00);
    writeReg(0x06, 0x00);

    // FIFO config
    writeReg(0x08, 0x0F);

    // SPO2 config
    writeReg(0x0A, 0x27);

    // LED currents
    writeReg(0x0C, 0x24);
    writeReg(0x0D, 0x24);

    // SPO2 mode (Red + IR)
    writeReg(0x09, 0x03);

    delay(100);

    Serial.printf("Mode Reg = 0x%02X\n", readReg(0x09));
    Serial.printf("SpO2 Reg = 0x%02X\n", readReg(0x0A));
}

void loop()
{
    static unsigned long last = 0;

    if (millis() - last > 1000)
    {
        last = millis();

        uint8_t wr = readReg(0x04);
        uint8_t rd = readReg(0x06);

        Serial.print("WR=");
        Serial.print(wr);

        Serial.print(" RD=");
        Serial.println(rd);
    }
}