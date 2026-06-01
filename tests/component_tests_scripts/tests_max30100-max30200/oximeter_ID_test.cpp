// Checks the idendity of the oximeter module
// Part identification register 0xFF

#include <Arduino.h>
#include <Wire.h>

#define MAX3010X_ADDR 0x57

uint8_t readRegister(uint8_t reg)
{
    Wire.beginTransmission(MAX3010X_ADDR);
    Wire.write(reg);

    if (Wire.endTransmission(false) != 0)
    {
        return 0xFF;
    }

    Wire.requestFrom(MAX3010X_ADDR, (uint8_t)1);

    if (Wire.available())
    {
        return Wire.read();
    }

    return 0xFF;
}

void scanI2C()
{
    Serial.println("\nScanning I2C bus...");

    bool found = false;

    for (uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);

        if (Wire.endTransmission() == 0)
        {
            Serial.print("Device found at 0x");

            if (addr < 16)
                Serial.print("0");

            Serial.println(addr, HEX);

            found = true;
        }
    }

    if (!found)
    {
        Serial.println("No I2C devices found.");
    }
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== MAX3010x Identification Test ===");

    Wire.begin(21, 22);     // SDA, SCL
    Wire.setClock(100000);

    scanI2C();

    uint8_t partID = readRegister(0xFF);
    uint8_t revID  = readRegister(0xFE);

    Serial.print("\nPart ID: 0x");
    Serial.println(partID, HEX);

    Serial.print("Revision ID: 0x");
    Serial.println(revID, HEX);

    Serial.println();

    switch(partID)
    {
        case 0x15:
            Serial.println("Detected MAX30102");
            break;

        case 0x11:
            Serial.println("Detected MAX30105");
            break;

        default:
            Serial.println("Unknown device or communication issue");
            break;
    }
}

void loop()
{
}