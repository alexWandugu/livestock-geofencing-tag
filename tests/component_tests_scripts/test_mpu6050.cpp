#include <arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;
float offsetX = 0, offsetY = 0, offsetZ = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA=21, SCL=22 (as in your main code)

  Serial.println("=== MPU6050 Test Starting ===");

  if (!mpu.begin()) {
    Serial.println("❌ MPU6050 not found! Check wiring.");
    while(1);
  }

  Serial.println("✅ MPU6050 Initialized");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("Keep the sensor **perfectly still** on a flat surface...");
  delay(2000);
  
  // Calibrate (average 200 readings)
  sensors_event_t a, g, temp;
  for(int i = 0; i < 200; i++) {
    mpu.getEvent(&a, &g, &temp);
    offsetX += a.acceleration.x;
    offsetY += a.acceleration.y;
    offsetZ += a.acceleration.z;
    delay(10);
  }
  
  offsetX /= 200;
  offsetY /= 200;
  offsetZ /= 200;

  Serial.printf("Calibration done!\nOffsets -> X: %.3f  Y: %.3f  Z: %.3f\n\n", offsetX, offsetY, offsetZ);
  Serial.println("Now showing calibrated readings:\n");
  // record your result and use it for the final program
  // ================= SAMPLE CALIBRATION OFFSETS  =================
  // offsetX = 9.893;
  // offsetY = 0.459;
  // offsetZ = 2.025;
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float calX = a.acceleration.x - offsetX;
  float calY = a.acceleration.y - offsetY;
  float calZ = a.acceleration.z - offsetZ;

  Serial.printf("Calibrated Accel → X: %.2f  Y: %.2f  Z: %.2f | Temp: %.2f°C\n", 
                calX, calY, calZ, temp.temperature);

  delay(2000);

}