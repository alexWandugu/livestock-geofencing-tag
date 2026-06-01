// Scans the I2C bus

#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Serial.println("I2C Scanner");

  Wire.begin(21, 22);           // SDA, SCL
  Wire.setClock(100000);        // Start slow

  Serial.println("Scanning...");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning I2C bus...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }

  if (nDevices == 0) {
    Serial.println("No I2C devices found!");
  } else {
    Serial.println("Scan done.");
  }

  delay(5000);  // Scan every 5 seconds
}