# Rearview Parking Assistant 

Final Project – Winter 2025
By: Regina Youssif & Ferdaws Alriashi
---

## Overview

The Rearview Parking Assistant is an embedded system designed to enhance vehicle safety during reverse driving. Using a **TM4C123G microcontroller**, **HC-SR04 ultrasonic sensor**, and **HC-05 Bluetooth module**, the system detects obstacles, calculates their approach speed, and alerts users both visually and wirelessly.

---

## Features

- Real-time obstacle detection
- Speed calculation of approaching objects
- Visual alerts via onboard LEDs
- Wireless alerts via Bluetooth to Android devices
- Polling-based architecture (no RTOS)
- Precise timer-based signal capture

---

## Hardware Components

- TM4C123G LaunchPad (Tiva C Series)
- HC-SR04 Ultrasonic Sensor
- HC-05 Bluetooth Module
- Green, Yellow, and Red LEDs
- Android device with Bluetooth terminal app

---

## How It Works

1. **Distance Measurement**  
   The ultrasonic sensor is triggered every 200 ms. Echo pulse width is measured using **Timer0A** in input capture mode.

2. **Speed Estimation**  
   Compares current and previous distances to calculate speed (cm/s).

3. **Decision Logic**  
   - `SAFE` – Sufficient distance  
   - `CAUTION` – Approaching at moderate speed  
   - `STOP` – Very close or rapidly approaching object

4. **User Alerts**  
   - **Visual**: Green (Safe), Yellow (Caution), Red (Stop)  
   - **Wireless**: Alerts sent to Android via **UART5** using HC-05

---

## System Architecture

- **Polling-based control loop**:
  - Distance trigger and capture
  - Speed calculation
  - Alert condition evaluation
  - LED control
  - UART message formatting and transmission

- **Key Function**:  
  `Measure_distance()`  
  Triggers ultrasonic pulse and calculates object distance using Timer0A edge-time capture. Accounts for timer overflows and converts pulse width to centimeters using the speed of sound.

---

## Testing & Validation

- **Distance Accuracy**:  
  Verified using a ruler. System readings matched manual measurements.

- **Speed Calculation**:  
  Validated by simulating controlled object motion.

- **Bluetooth Communication**:  
  UART configuration issue resolved by explicitly selecting UART5 in Keil. Messages successfully received on Android.

- **LED Alerts**:  
  Tested under various speeds and distances. Added noise filtering to reject false readings in echo-prone environments.

---

## Tools & Libraries

- Keil uVision IDE  
- TivaWare Peripheral Libraries  
- PuTTY / Android Bluetooth Terminal  
- TM4C123G LaunchPad  
- HC-SR04 & HC-05 Datasheets  
