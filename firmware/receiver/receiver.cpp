#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ===== WIFI CREDENTIALS =====
const char* ssid = "";
const char* password = "";

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
float SPO2_MIN = 40;

// ===== GPS GEOFENCE =====
float centerLat = -1.09;
float centerLon = 37.01;
float geoRadiusMeters = 1500.0;

// ===== AVERAGING FOR BPM & SPO2 =====
const int NUM_READINGS = 10;          // Average over last 10 readings (~20 seconds)
float bpmReadings[NUM_READINGS];
float spo2Readings[NUM_READINGS];
int readIndex = 0;
float bpmSum = 0.0;
float spo2Sum = 0.0;
bool bufferFilled = false;

// ===== TIMERS =====
unsigned long lastReceiveTime = 0;
const unsigned long timeout = 5000;

unsigned long lastMotionTime = 0;
const unsigned long motionTimeout = 15000;

// ===== VARIABLES =====
float temperature = 0.0;
float ax = 0.0, ay = 0.0, az = 0.0;
float bpm = 0.0, spo2 = 0.0;           // Raw values
float avgBPM = 0.0, avgSpO2 = 0.0;     // Averaged values
float lat = 0.0, lon = 0.0;

// ===== HELPER FUNCTIONS =====
float getValue(String data, String key) {
  int start = data.indexOf(key);
  if (start == -1) return 0.0;
  int from = start + key.length();
  int to = data.indexOf(",", from);
  if (to == -1) to = data.length();
  return data.substring(from, to).toFloat();
}

float distanceMeters(float lat1, float lon1, float lat2, float lon2) {
  float R = 6371000;
  float dLat = (lat2 - lat1) * PI / 180;
  float dLon = (lon2 - lon1) * PI / 180;
  float a = sin(dLat / 2) * sin(dLat / 2) +
            cos(lat1 * PI / 180) * cos(lat2 * PI / 180) *
            sin(dLon / 2) * sin(dLon / 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}

void alertBeep(int times, int dly) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(dly);
    digitalWrite(BUZZER, LOW);
    delay(dly);
  }
}

void updateAverages(float newBPM, float newSpO2) {
  // Subtract the oldest reading
  bpmSum -= bpmReadings[readIndex];
  spo2Sum -= spo2Readings[readIndex];

  // Store new readings
  bpmReadings[readIndex] = newBPM;
  spo2Readings[readIndex] = newSpO2;

  // Add the new readings
  bpmSum += newBPM;
  spo2Sum += newSpO2;

  readIndex = (readIndex + 1) % NUM_READINGS;

  if (readIndex == 0) bufferFilled = true;

  // Calculate averages
  if (bufferFilled) {
    avgBPM = bpmSum / NUM_READINGS;
    avgSpO2 = spo2Sum / NUM_READINGS;
  } else {
    // Use partial average until buffer is full
    avgBPM = bpmSum / (readIndex + 1);
    avgSpO2 = spo2Sum / (readIndex + 1);
  }
}

// ===== FIREBASE CONFIG =====
const char* firebaseHost = "https://example-default-rtdb.firebasedatabase.app/";
const char* firebasePath = "/livestock_readings.json";

// ===== SEND TO FIREBASE =====
void sendToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = String(firebaseHost) + firebasePath;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Build JSON payload
    String jsonData = "{";
    jsonData += "\"temperature\":" + String(temperature, 2) + ",";
    jsonData += "\"ax\":" + String(ax, 2) + ",";
    jsonData += "\"ay\":" + String(ay, 2) + ",";
    jsonData += "\"az\":" + String(az, 2) + ",";
    jsonData += "\"bpm\":" + String(bpm, 2) + ",";
    jsonData += "\"spo2\":" + String(spo2, 2) + ",";
    jsonData += "\"lat\":" + String(lat, 6) + ",";
    jsonData += "\"lon\":" + String(lon, 6) + ",";
    jsonData += "\"clat\":" + String(centerLat, 6) + ",";
    jsonData += "\"clon\":" + String(centerLon, 6) + ",";
    jsonData += "\"radius\":" + String(geoRadiusMeters, 2) + ",";
    jsonData += "\"timestamp\":" + String(millis()) + ""; 
    jsonData += "}";

    int httpResponseCode = http.POST(jsonData);

    Serial.print("Firebase Response: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void checkAlerts() {
  bool alert = false;

  // Temperature alert
  if (temperature > TEMP_MAX) {
    Serial.println("HIGH TEMPERATURE ALERT");
    alertBeep(3, 200);
    alert = true;
  }

  // BPM ALERT - using AVERAGE
  if (avgBPM < BPM_MIN || avgBPM > BPM_MAX) {
    Serial.println("BPM ALERT (Avg: " + String(avgBPM, 1) + ")");
    alertBeep(2, 150);
    alert = true;
  }

  // SPO2 ALERT - using AVERAGE
  if (avgSpO2 < SPO2_MIN) {
    Serial.println("SPO2 ALERT (Avg: " + String(avgSpO2, 1) + ")");
    alertBeep(3, 100);
    alert = true;
  }

  // Motion / Inactivity detection
  float motionMag = sqrt(ax * ax + ay * ay + az * az);
  if (motionMag < 0.3) {
    if (millis() - lastMotionTime > motionTimeout) {
      Serial.println("NO MOTION / POSSIBLE ILLNESS ALERT");
      alertBeep(4, 150);
      alert = true;
    }
  } else {
    lastMotionTime = millis();
  }

  // Geofence breach
  float dist = distanceMeters(lat, lon, centerLat, centerLon);
  if (dist > geoRadiusMeters) {
    Serial.println("GEOFENCE BREACH ALERT - Distance: " + String(dist) + "m");
    alertBeep(5, 100);
    alert = true;
  }

  if (!alert) {
    digitalWrite(BUZZER, LOW);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Initialize averaging arrays
  for (int i = 0; i < NUM_READINGS; i++) {
    bpmReadings[i] = 0;
    spo2Readings[i] = 0;
  }

  // Startup beep
  digitalWrite(BUZZER, HIGH);
  delay(1000);
  digitalWrite(BUZZER, LOW);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  // LoRa
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

    // Parse raw values
    temperature = getValue(received, "T:");
    ax = getValue(received, "AX:");
    ay = getValue(received, "AY:");
    az = getValue(received, "AZ:");
    bpm = getValue(received, "BPM:");
    spo2 = getValue(received, "SpO2:");
    lat = getValue(received, "LAT:");
    lon = getValue(received, "LON:");

    // Update moving averages
    updateAverages(bpm, spo2);

    // Send averaged values to dashboard
    sendToServer();

    lastReceiveTime = millis();

    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);

    // Check alerts using averages
    checkAlerts();
  }

  // Connection timeout indicator
  if (millis() - lastReceiveTime > timeout) {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  }
}