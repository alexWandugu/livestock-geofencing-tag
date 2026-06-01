#include <SPI.h>
#include <LoRa.h>

#define SS   5
#define RST  14
#define DIO0 27

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== LoRa Receiver Test ===");

  SPI.begin(18, 19, 23);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();

  Serial.println("LoRa Receiver READY - Waiting...");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }
    Serial.println("\n=== PACKET RECEIVED ===");
    Serial.println("Data: " + received);
    Serial.print("RSSI: "); Serial.println(LoRa.packetRssi());
    Serial.print("SNR:  "); Serial.println(LoRa.packetSnr());
  }
}