#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

void setup() {
  Serial.begin(115200);
  ds18b20.begin();

  Serial.println("=== DS18B20 Test Starting ===");

  if (ds18b20.getDeviceCount() == 0) {
    Serial.println("❌ No DS18B20 found!");
  } else {
    Serial.printf("✅ Found %d DS18B20 sensor(s)\n", ds18b20.getDeviceCount());
  }
}

void loop() {
  ds18b20.requestTemperatures();
  float tempC = ds18b20.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("❌ Sensor disconnected");
  } else {
    Serial.printf("🌡️  Temperature: %.2f °C\n", tempC);
  }

  delay(1000);
}