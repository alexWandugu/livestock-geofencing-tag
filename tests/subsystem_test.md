# Subsystem Test Results

## 1. LoRa Communication
- Frequency: 433MHz, SF7, BW 125kHz
- Packet format: `T:xx.xx,AX:xx,AY:xx,AZ:xx,BPM:xx,SpO2:xx,LAT:xx.xxxxxx,LON:xx.xxxxxx`
- Status: ✅ Stable transmission

## 2. GPS (GY-GPS6MV2)
- Cold start fix time: __ seconds
- Accuracy: __ meters
- Default fallback: Juja, Kenya area

## 3. Sensors
- **MPU6050**: Good motion detection
- **DS18B20**: Accurate body/environment temp
- **GY-MAX30100**: Good SpO2 and eart rate readings
- **MAX30100/MAX30102**: Heart rate & SpO2 not working (See the "MAX30100-MAX30102_Failure_Analysis_Report.pdf")

## 4. Power & Range Tests
- 10m in an Urban Area
- 27m-30m in an open field