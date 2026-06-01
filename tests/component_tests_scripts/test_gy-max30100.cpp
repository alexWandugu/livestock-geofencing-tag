#include <Arduino.h>
#include <Wire.h>
#include "spo2_algorithm.h"

#define MAX30102_ADDRESS 0x57

#define FIFO_WR_PTR   0x04
#define FIFO_OVF      0x05
#define FIFO_RD_PTR   0x06
#define FIFO_DATA     0x07

uint32_t irBuffer[100];
uint32_t redBuffer[100];

int32_t spo2 = 0;
int8_t validSPO2 = 0;
int32_t heartRate = 0;
int8_t validHeartRate = 0;

// =====================================================
// Register Functions
// =====================================================

void writeRegister(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(MAX30102_ADDRESS);
    Wire.write(reg);
    Wire.write(value);

    uint8_t result = Wire.endTransmission();

    if(result != 0)
    {
        Serial.print("I2C Write Error: ");
        Serial.println(result);
    }
}

uint8_t readRegister(uint8_t reg)
{
    Wire.beginTransmission(MAX30102_ADDRESS);
    Wire.write(reg);

    if(Wire.endTransmission(false) != 0)
    {
        return 0;
    }

    Wire.requestFrom(MAX30102_ADDRESS, 1);

    if(Wire.available())
    {
        return Wire.read();
    }

    return 0;
}

// =====================================================
// Read one FIFO sample
// =====================================================

bool readFIFOSample(uint32_t &red, uint32_t &ir)
{
    Wire.beginTransmission(MAX30102_ADDRESS);
    Wire.write(FIFO_DATA);

    if(Wire.endTransmission(false) != 0)
    {
        return false;
    }

    if(Wire.requestFrom(MAX30102_ADDRESS, 6) != 6)
    {
        return false;
    }

    red = 0;
    ir  = 0;

    for(int i = 0; i < 3; i++)
    {
        red = (red << 8) | Wire.read();
    }

    for(int i = 0; i < 3; i++)
    {
        ir = (ir << 8) | Wire.read();
    }

    // MAX30102 uses 18-bit ADC stored in 24-bit word
    red &= 0x3FFFFF;
    ir  &= 0x3FFFFF;

    return true;
}

// =====================================================
// Wait until a new sample exists
// =====================================================

bool waitForNewSample(uint16_t timeoutMs = 100)
{
    uint32_t start = millis();

    uint8_t rd = readRegister(FIFO_RD_PTR);

    while(millis() - start < timeoutMs)
    {
        uint8_t wr = readRegister(FIFO_WR_PTR);

        if(wr != rd)
        {
            return true;
        }

        delay(1);
    }

    return false;
}

void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("=== MAX30102 Improved FIFO Test ===");

    delay(2000);

    Wire.begin(21, 22);
    Wire.setClock(100000);

    // -------------------------------------------------
    // Sensor Configuration
    // -------------------------------------------------

    writeRegister(0x09, 0x40);      // Reset
    delay(1500);

    writeRegister(0x08, 0x1F);      // FIFO Config
    writeRegister(0x09, 0x03);      // SpO2 Mode

    // BEST CONFIG FROM TESTING
    writeRegister(0x0A, 0x67);

    writeRegister(0x0C, 0x5F);      // Red LED
    writeRegister(0x0D, 0x7F);      // IR LED

    // -------------------------------------------------
    // IMPORTANT:
    // Reset FIFO pointers
    // -------------------------------------------------

    writeRegister(FIFO_WR_PTR, 0x00);
    writeRegister(FIFO_OVF,    0x00);
    writeRegister(FIFO_RD_PTR, 0x00);

    Serial.println("Ready.");
}

void loop()
{
    // =================================================
    // Collect 100 ACTUAL FIFO samples
    // =================================================

    for(int i = 0; i < 100; i++)
    {
        if(!waitForNewSample())
        {
            Serial.println("FIFO timeout");
            return;
        }

        uint32_t red, ir;

        if(!readFIFOSample(red, ir))
        {
            Serial.println("FIFO read failed");
            return;
        }

        redBuffer[i] = red;
        irBuffer[i]  = ir;

        // ------------------------------------------------
        // DEBUG MODE:
        // Uncomment this section to view raw waveform
        // ------------------------------------------------

        
        // Serial.print(red);
        // Serial.print(",");
        // Serial.println(ir);
        
    }

    // =================================================
    // Run Maxim Algorithm
    // =================================================

    maxim_heart_rate_and_oxygen_saturation(
        irBuffer,
        100,
        redBuffer,
        &spo2,
        &validSPO2,
        &heartRate,
        &validHeartRate
    );

    // =================================================
    // Diagnostics
    // =================================================

    uint8_t wr = readRegister(FIFO_WR_PTR);
    uint8_t rd = readRegister(FIFO_RD_PTR);
    uint8_t ov = readRegister(FIFO_OVF);

    Serial.print("IR=");
    Serial.print(irBuffer[99]);

    Serial.print(" BPM=");
    Serial.print(validHeartRate ? heartRate : -1);

    Serial.print(" SpO2=");
    Serial.print(validSPO2 ? spo2 : -1);

    Serial.print("% WR=");
    Serial.print(wr);

    Serial.print(" RD=");
    Serial.print(rd);

    Serial.print(" OVF=");
    Serial.print(ov);

    Serial.println();
}