#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ===== WIFI CREDENTIALS =====
const char* ssid = "";
const char* password = "";

// ===== SERVER URL =====
const char* serverName = "";

// ===== LoRa Pins =====
#define SS   5
#define RST  14
#define DIO0 27

// ===== OUTPUT DEVICES =====
#define GREEN_LED 2
#define RED_LED   4
#define BUZZER    26


// ===== THRESHOLDS =====
float TEMP_MAX = 38.5;
float BPM_MIN  = 50;
float BPM_MAX  = 120;
float SPO2_MIN = 92;

// ===== GPS GEOFENCE =====
float centerLat = -1.286389;
float centerLon = 36.817223;
float geoRadiusMeters = 50; // adjust boundary

unsigned long lastReceiveTime = 0;
const unsigned long timeout = 5000;

// ===== VARIABLES =====
float temperature, ax, ay, az, bpm, spo2, lat, lon;

unsigned long lastMotionTime = 0;
const unsigned long motionTimeout = 15000;


// ===== FUNCTION =====
float getValue(String data, String key) {
  int start = data.indexOf(key);
  if (start == -1) return 0;

  int from = start + key.length();
  int to = data.indexOf(",", from);
  if (to == -1) to = data.length();

  return data.substring(from, to).toFloat();
}


// ===== DISTANCE (Haversine) =====
float distanceMeters(float lat1, float lon1, float lat2, float lon2) {
  float R = 6371000;
  float dLat = (lat2 - lat1) * PI / 180;
  float dLon = (lon2 - lon1) * PI / 180;

  float a =
    sin(dLat / 2) * sin(dLat / 2) +
    cos(lat1 * PI / 180) * cos(lat2 * PI / 180) *
    sin(dLon / 2) * sin(dLon / 2);

  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}

// ===== BUZZ PATTERN =====
void alertBeep(int times, int dly) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(dly);
    digitalWrite(BUZZER, LOW);
    delay(dly);
  }
}

// ===== SEND TO SERVER =====
void sendToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData = "temperature=" + String(temperature) +
                      "&ax=" + String(ax) +
                      "&ay=" + String(ay) +
                      "&az=" + String(az) +
                      "&bpm=" + String(bpm) +
                      "&spo2=" + String(spo2) +
                      "&lat=" + String(lat, 6) +
                      "&lon=" + String(lon, 6);

    int httpResponseCode = http.POST(postData);

    Serial.print("HTTP Response: ");
    Serial.println(httpResponseCode);

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // 🔊 Startup beep
  digitalWrite(BUZZER, HIGH);
  delay(2000);
  digitalWrite(BUZZER, LOW);

  // ===== WIFI CONNECT =====
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");

  // ===== LORA =====
  SPI.begin(18, 19, 23);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();

  Serial.println("ESP32 LoRa Receiver Ready");
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    String received = "";

    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    Serial.println("Raw: " + received);

    // ===== PARSE =====
    temperature = getValue(received, "T:");
    ax = getValue(received, "AX:");
    ay = getValue(received, "AY:");
    az = getValue(received, "AZ:");
    bpm = getValue(received, "BPM:");
    spo2 = getValue(received, "SpO2:");
    lat = getValue(received, "LAT:");
    lon = getValue(received, "LON:");

    // ===== SEND TO SERVER =====
    sendToServer();

    lastReceiveTime = millis();

    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);
  }

  if (millis() - lastReceiveTime > timeout) {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  }
}

// ===== CHECK ALERTS =====
void checkAlerts() {

  bool alert = false;

  // TEMP ALERT
  if (temperature > TEMP_MAX) {
    Serial.println("TEMP ALERT");
    alertBeep(2, 150);
    alert = true;
  }

  // BPM ALERT
  if (bpm < BPM_MIN || bpm > BPM_MAX) {
    Serial.println("BPM ALERT");
    alertBeep(2, 150);
    alert = true;
  }

  // SPO2 ALERT
  if (spo2 < SPO2_MIN) {
    Serial.println("SPO2 ALERT");
    alertBeep(3, 100);
    alert = true;
  }

  // MOTION DETECTION
  float motionMag = sqrt(ax * ax + ay * ay + az * az);

  if (motionMag < 0.2) {
    if (millis() - lastMotionTime > motionTimeout) {
      Serial.println("NO MOTION ALERT");
      alertBeep(4, 200);
      alert = true;
    }
  } else {
    lastMotionTime = millis();
  }

  // GEOFENCE ALERT
  float dist = distanceMeters(lat, lon, centerLat, centerLon);

  if (dist > geoRadiusMeters) {
    Serial.println("GEOFENCE BREACH");
    alert = true;
    alertBeep(5, 100);
  }

  if (!alert) {
    digitalWrite(BUZZER, LOW);
  }
}
