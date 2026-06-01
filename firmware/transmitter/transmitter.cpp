#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGPSPlus.h>
#include <Arduino.h>
#include "spo2_algorithm.h"

// ================= PIN DEFINITIONS =================
#define ONE_WIRE_BUS  4     // DS18B20

#define LORA_SS       5
#define LORA_RST      14
#define LORA_DIO0     26
#define LED 2

#define RXD2          16    // GPS
#define TXD2          17

// ================= MAX30102 REGISTER ADDRESSES =================
#define MAX30100_ADDRESS 0x57

#define FIFO_WR_PTR   0x04
#define FIFO_OVF      0x05
#define FIFO_RD_PTR   0x06
#define FIFO_DATA     0x07

// ================= OBJECTS =================
Adafruit_MPU6050 mpu;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// ================= VARIABLES =================
float temperatureDS = 0.0;

float accelX = 0.0;
float accelY = 0.0;
float accelZ = 0.0;

float latitude  = -1.0970;   // Default fallback (Juja area)
float longitude = 37.0153;

// Oximetry variables - Shared between tasks
int32_t bpm = 0;
int8_t validBPM = 0;
int32_t spo2 = 0;
int8_t validSpO2 = 0;

uint32_t irBuffer[100];
uint32_t redBuffer[100];

// ================= MPU CALIBRATION OFFSETS =================
const float offsetX = 9.893;
const float offsetY = 0.459;
const float offsetZ = 2.025;

// =====================================================
// Oximeter Register Functions 
// =====================================================

void writeRegister(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(MAX30100_ADDRESS);
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
    Wire.beginTransmission(MAX30100_ADDRESS);
    Wire.write(reg);
    if(Wire.endTransmission(false) != 0) return 0;
    Wire.requestFrom(MAX30100_ADDRESS, 1);
    if(Wire.available()) return Wire.read();
    return 0;
}

bool readFIFOSample(uint32_t &red, uint32_t &ir)
{
    Wire.beginTransmission(MAX30100_ADDRESS);
    Wire.write(FIFO_DATA);
    if(Wire.endTransmission(false) != 0) return false;
    if(Wire.requestFrom(MAX30100_ADDRESS, 6) != 6) return false;

    red = 0; ir = 0;
    for(int i = 0; i < 3; i++) red = (red << 8) | Wire.read();
    for(int i = 0; i < 3; i++) ir  = (ir  << 8) | Wire.read();

    red &= 0x3FFFFF;
    ir  &= 0x3FFFFF;
    return true;
}

bool waitForNewSample(uint16_t timeoutMs = 100)
{
    uint32_t start = millis();
    uint8_t rd = readRegister(FIFO_RD_PTR);
    while(millis() - start < timeoutMs)
    {
        uint8_t wr = readRegister(FIFO_WR_PTR);
        if(wr != rd) return true;
        delay(1);
    }
    return false;
}

// =====================================================
// TASK: GPS TASK
// =====================================================
void taskGPS(void * parameter) {
  while (1) {
    while (gpsSerial.available()) {
      gps.encode(gpsSerial.read());
    }

    if (gps.location.isValid() && gps.location.lat() != 0 && gps.location.lng() != 0) {
      latitude  = gps.location.lat();
      longitude = gps.location.lng();
    }
    // Else keep last known / fallback coordinates

    if(gps.satellites.value()>0){
      digitalWrite(LED,HIGH);
    }else{
      digitalWrite(LED,LOW);   // Fixed: was always HIGH
    }
    
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

// =====================================================
// TASK: SENSOR TASK (Temperature + MPU6050)
// =====================================================
void taskSensors(void * parameter) {
  while (1) {
    // MPU6050
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    accelX = a.acceleration.x - offsetX;
    accelY = a.acceleration.y - offsetY;
    accelZ = a.acceleration.z - offsetZ;

    // DS18B20 Temperature
    ds18b20.requestTemperatures();
    temperatureDS = ds18b20.getTempCByIndex(0);

    vTaskDelay(800 / portTICK_PERIOD_MS);
  }
}

// =====================================================
// TASK: OXIMETER TASK (Low-level FIFO + Maxim Algorithm)
// =====================================================
void taskOximeter(void * parameter) {
  while (1) {
    for(int i = 0; i < 100; i++) {
      if(!waitForNewSample(120)) {
        vTaskDelay(50 / portTICK_PERIOD_MS);
        continue;
      }

      uint32_t red, ir;
      if(!readFIFOSample(red, ir)) {
        continue;
      }

      redBuffer[i] = red;
      irBuffer[i]  = ir;
    }

    // Run Maxim Algorithm
    maxim_heart_rate_and_oxygen_saturation(
        irBuffer, 100, redBuffer,
        &spo2, &validSpO2,
        &bpm, &validBPM
    );

    // Debug output - This is what you want to see
    Serial.printf("Raw BPM=%d (%s), SpO2=%d (%s)\n", 
                  bpm, validBPM ? "VALID" : "INVALID",
                  spo2, validSpO2 ? "VALID" : "INVALID");

    vTaskDelay(2000 / portTICK_PERIOD_MS);   // 2 second update
  }
}

// =====================================================
// TASK: LORA TRANSMIT TASK
// =====================================================
void taskLoRa(void * parameter) {
  while (1) {
    String data = "";

    data += "T:" + String(temperatureDS, 2);
    data += ",AX:" + String(accelX, 2);
    data += ",AY:" + String(accelY, 2);
    data += ",AZ:" + String(accelZ, 2);

    // === Use latest valid oximeter readings ===
    int32_t txBPM = (validBPM && bpm > 30 && bpm < 220) ? bpm : 0;
    int32_t txSpO2 = (validSpO2 && spo2 > 70 && spo2 <= 100) ? spo2 : 0;

    data += ",BPM:" + String(txBPM);
    data += ",SpO2:" + String(txSpO2);

    data += ",LAT:" + String(latitude, 6);
    data += ",LON:" + String(longitude, 6);

    // Send via LoRa
    LoRa.beginPacket();
    LoRa.print(data);
    LoRa.endPacket();

    Serial.printf("Satellites in view: %d\n", gps.satellites.value());
    Serial.println("=== TRANSMITTED ===");
    Serial.println(data);
    Serial.println(" ");

    vTaskDelay(2500 / portTICK_PERIOD_MS);
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  // I2C
  Wire.begin(21, 22);
  Wire.setClock(100000);

  // ================= MPU6050 =================
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1);
  }
  Serial.println("MPU6050 OK");

  // ================= DS18B20 =================
  ds18b20.begin();
  Serial.println("DS18B20 OK");

  // ================= GPS =================
  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("GPS Serial Started");

  // ================= MAX30100 Oximeter Init =================
  Serial.println("=== MAX30100 Initializing ===");
  writeRegister(0x09, 0x40);      // Reset
  delay(1500);
  writeRegister(0x08, 0x1F);      // FIFO Config
  writeRegister(0x09, 0x03);      // SpO2 Mode
  writeRegister(0x0A, 0x67);
  writeRegister(0x0C, 0x5F);      // Red LED
  writeRegister(0x0D, 0x7F);      // IR LED

  writeRegister(FIFO_WR_PTR, 0x00);
  writeRegister(FIFO_OVF,    0x00);
  writeRegister(FIFO_RD_PTR, 0x00);
  Serial.println("MAX30100 Ready");

  // ================= LoRa =================
  SPI.begin(18, 19, 23);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();

  Serial.println("LoRa Ready");

  // ================= CREATE TASKS =================
  xTaskCreatePinnedToCore(taskGPS,     "GPS Task",     4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskSensors, "Sensors Task", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskOximeter,"Oximeter Task", 8192, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskLoRa,    "LoRa Task",    4096, NULL, 1, NULL, 0);

  Serial.println("=== Livestock Tag Transmitter Ready ===");
}

void loop() {
  // All work done in tasks
}