#include <SPI.h>
#include <LoRa.h>

// ===== LoRa Pins (Transmitter) =====
#define SS   5
#define RST  14
#define DIO0 26   // Important: matches your Ra-01 wiring

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== LoRa Transmitter Test ===");

  SPI.begin(18, 19, 23);           // SCK, MISO, MOSI
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {        // 433 MHz - must match receiver
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();

  Serial.println("LoRa Transmitter READY");
}

void loop() {
  static int counter = 0;

  String packet = "TEST_PACKET #" + String(counter) + " | RSSI Check | Hello from Tag!";

  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();

  Serial.println("Sent: " + packet);
  
  counter++;
  delay(2000);   // Send every 2 seconds
}