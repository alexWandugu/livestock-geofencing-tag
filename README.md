# Low-Powered Electronic Livestock Tag for Geofencing & Activity Monitoring

**Date**: May 2025

## Project Overview

This project develops a smart, low-power electronic tag for livestock in free-grazing systems. It provides real-time GPS tracking, geofencing alerts, activity monitoring via accelerometer, and vital signs (temperature, heart rate, SpO2) using IoT/LoRa communication.

**Key Features**
- Real-time location tracking with u-blox NEO-6M GPS
- Geofencing with custom boundaries
- Activity monitoring using MPU6050 (accelerometer)
- Environmental & vital monitoring: DS18B20 temperature + MAX30100 Pulse Oximeter
- Long-range LoRa communication (Ra-01 433MHz)
- Data forwarding via ESP32 WiFi to web dashboard

## Hardware Components

### Transmitter (Tag)
- **MCU**: ESP32 DevKit
- **LoRa**: AI-Thinker Ra-01 (433MHz)
- **GPS**: GY-GPS6MV2 (u-blox NEO-6M)
- **IMU**: MPU6050 (3-axis accelerometer)
- **Temp**: DS18B20
- **Pulse Ox**: MAX30100

![Transmitter Board](hardware/images/ANIMAL_TAG.jpg)

### Receiver (Base Station)
ESP32 + LoRa receiving data and pushing to web server.
![Transmitter Board](hardware/images/BASE_STATION.jpg)

## Firmware

- **Transmitter**: FreeRTOS multi-tasking for stable sensor readings (see `firmware/`)
- **Receiver**: Parses packets, checks alerts, sends to PHP dashboard

## Setup Instructions

1. Clone repo
2. Flash Transmitter & Receiver sketches
3. Update WiFi credentials & server URL in Receiver
4. Adjust geofence center coordinates

## Future Work (per Proposal)

- Full deployment & field testing

## Contributors

- Ochieng Acheng Tabby
- Alex Thiong'o Wandugu

---

**Status**: Prototype complete | In testing phase
