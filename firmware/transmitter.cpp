#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGPSPlus.h>
#include "MAX30100_PulseOximeter.h"

// ================= PIN DEFINITIONS =================
#define ONE_WIRE_BUS 4

#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 26

#define RXD2 16
#define TXD2 17

// ================= OBJECTS =================
Adafruit_MPU6050 mpu;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

PulseOximeter pox;

// ================= VARIABLES =================
float temperatureDS = 0;

float accelX = 0;
float accelY = 0;
float accelZ = 0;

float bpm = 0;
float spo2 = 0;

float latitude = 0;
float longitude = 0;

// ================= TIMERS =================
unsigned long lastSensorRead = 0;

// =====================================================
// MAX30100 CALLBACK
// =====================================================
void onBeatDetected()
{
  Serial.println("Beat!");
}

// =====================================================
// TASK: OXIMETER TASK
// Runs VERY FAST for stable readings
// =====================================================
void taskOximeter(void * parameter)
{
  while (1)
  {
    pox.update();

    bpm = pox.getHeartRate();
    spo2 = pox.getSpO2();

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// =====================================================
// TASK: GPS TASK
// =====================================================
void taskGPS(void * parameter)
{
  while (1)
  {
    while (gpsSerial.available())
    {
      gps.encode(gpsSerial.read());
    }

    if (gps.location.isValid() &&
        gps.location.lat() != 0 &&
        gps.location.lng() != 0)
    {
      latitude = gps.location.lat();
      longitude = gps.location.lng();
    }
    else
    {
      // Default coordinates
      latitude = -1.0970;   // South
      longitude = 37.0153;  // East
    }

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// =====================================================
// TASK: SENSOR TASK
// =====================================================
void taskSensors(void * parameter)
{
  while (1)
  {
    // ===== MPU6050 =====
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    accelX = a.acceleration.x;
    accelY = a.acceleration.y;
    accelZ = a.acceleration.z;

    // ===== DS18B20 =====
    ds18b20.requestTemperatures();
    temperatureDS = ds18b20.getTempCByIndex(0);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// =====================================================
// TASK: LORA TRANSMIT TASK
// =====================================================
void taskLoRa(void * parameter)
{
  while (1)
  {
    String data = "";

    data += "T:" + String(temperatureDS, 2);
    data += ",AX:" + String(accelX, 2);
    data += ",AY:" + String(accelY, 2);
    data += ",AZ:" + String(accelZ, 2);

    data += ",BPM:" + String(bpm, 2);
    data += ",SpO2:" + String(spo2, 2);

    data += ",LAT:" + String(latitude, 6);
    data += ",LON:" + String(longitude, 6);

    // ===== SEND LORA =====
    LoRa.beginPacket();
    LoRa.print(data);
    LoRa.endPacket();

    Serial.println("=================================");
    Serial.println("Sent:");
    Serial.println(data);

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

// =====================================================
// SETUP
// =====================================================
void setup()
{
  Serial.begin(115200);

  // ===== I2C =====
  Wire.begin(21, 22);

  // =================================================
  // MPU6050
  // =================================================
  if (!mpu.begin())
  {
    Serial.println("MPU6050 not found!");
    while (1);
  }

  Serial.println("MPU6050 OK");

  // =================================================
  // DS18B20
  // =================================================
  ds18b20.begin();

  // =================================================
  // GPS
  // =================================================
  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // =================================================
  // MAX30100
  // =================================================
  if (!pox.begin())
  {
    Serial.println("MAX30100 FAILED");
  }
  else
  {
    Serial.println("MAX30100 SUCCESS");

    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

    pox.setOnBeatDetectedCallback(onBeatDetected);
  }

  // =================================================
  // SPI + LORA
  // =================================================
  SPI.begin(18, 19, 23);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6))
  {
    Serial.println("LoRa init failed!");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  Serial.println("LoRa Ready");

  // =================================================
  // CREATE TASKS
  // =================================================

  xTaskCreatePinnedToCore(
    taskOximeter,
    "Oximeter Task",
    4096,
    NULL,
    3,
    NULL,
    1);

  xTaskCreatePinnedToCore(
    taskGPS,
    "GPS Task",
    4096,
    NULL,
    1,
    NULL,
    1);

  xTaskCreatePinnedToCore(
    taskSensors,
    "Sensor Task",
    4096,
    NULL,
    1,
    NULL,
    1);

  xTaskCreatePinnedToCore(
    taskLoRa,
    "LoRa Task",
    4096,
    NULL,
    1,
    NULL,
    0);

  Serial.println("System Ready");
}

// =====================================================
// LOOP
// =====================================================
void loop()
{
  // Empty because FreeRTOS tasks are running
}