# HaptiDrum Firmware (ESP32 + nRF52)

Firmware for the **HaptiDrum prototype**, a wearable haptic system that simulates air drumming using **IMU motion sensing**, **Bluetooth Low Energy (BLE) communication**, and **dual tactile feedback** via vibration motors (LRA) and electrical muscle stimulation (EMS).  

This repository includes parallel implementations for **ESP32-S3** and **nRF52** platforms, with a custom PCB integrating the **DRV2605L haptic driver** and **MCP4725 DAC**.

---

## Hardware Architecture
```text
              Motion Data
          (Hits / Gestures)
                   |
                   v
+------------------+------------------+
|                 IMU                 |
|      BNO080 (ESP32, External) or    |
|     LSM6DS3 (nRF52 Sense, On-Board) |
+-------------------------------------+ 
                   |
                   v I²C
      +------------+--------------+
      |    ESP32-S3 / nRF52 MCU   |
      |  • BLE Communication      |
      |  • IMU Data Processing    |
      |  • Control Logic          |
      +------------+--------------+
                   |
                   v I²C
          +------------------+
          |                  |
          v                  v
   +--------------+    +-------------+
   |   DRV2605L   |    |  MCP4725 DAC|
   | Haptic Driver|    | EMS Voltage |
   |              |    |   Control   |
   +------+-------+    +------+------+
          |                   |
          v                   v
   +-------------+     +-------------+
   |  LRA Motor  |     | EMS Module  |
   | (External)  |     | (External)  |
   +-------------+     +-------------+
```
---

## Libraries Required

### ESP32
- [ESP32 BLE Arduino](https://github.com/nkolban/ESP32_BLE_Arduino)
- [SparkFun BNO080 Arduino Library](https://github.com/sparkfun/SparkFun_BNO080_Arduino_Library)
- [Adafruit DRV2605](https://github.com/adafruit/Adafruit_DRV2605_Library)

### nRF52
- [Adafruit Bluefruit nRF52 Library](https://github.com/adafruit/Adafruit_nRF52_Arduino)
- [Arduino LSM6DS3 Library](https://github.com/arduino-libraries/Arduino_LSM6DS3)
- [Adafruit DRV2605](https://github.com/adafruit/Adafruit_DRV2605_Library)

---

## Haptic Feedback

The DRV2605L haptic driver on the PCB controls an external LRA (Linear Resonant Actuator) motor.  
- Provides tactile cues such as long vibrations, short buzzes, and rhythm patterns triggered by BLE commands.  
- Optimized for precise 200 Hz resonance LRAs (e.g., Vybronics VL120628H).  
- Used to simulate drum hits and improve the realism of air drumming.

---

## EMS Output

The integrated MCP4725 DAC enables precise voltage control for an external EMS module.  
- Converts digital control signals from the MCU into safe, smooth analog voltages.  
- Allows adjustable EMS intensity for tactile feedback beyond vibration.  
- EMS current limits are kept within [IEC 60601‑1](https://www.iso.org/standard/72428.html) and [U.S. FDA](https://www.fda.gov/medical-devices/standards-and-conformity-assessment-program) safety guidelines (< 0.1 mA normal, < 0.5 mA fault).  
- Provides an additional layer of realism and immersion for wearable haptics.

---

## Example BLE Commands

- **FIND** → Long vibration (locate device)  
- **CONNECT** → Triple short buzz (confirm connection)  
- **RESET** → IMU reset routine  
- **PLAY** → Hit detected, Short vibration + EMS pulse  

---

## Purpose & Outcome

This firmware validates:
- BLE connectivity and command handling on ESP32 and nRF52
- Real-time IMU orientation and gesture tracking
- Integrated DRV2605L vibration motor control
- Safe, precise EMS output control via MCP4725 DAC

By supporting **two hardware platforms** with a shared PCB design, the HaptiDrum system demonstrates flexibility, robustness, and professional-level embedded integration.

---

## Media (Website)

For full project documentation, photos, and demo videos:

[HeptiDrum Website Link](https://sites.google.com/uw.edu/team5-haptidrum/home?authuser=0)

---

## Safety Notice

Electrical Muscle Stimulation (EMS) usage requires caution:

- Limit currents to **≤ 0.1 mA** under normal operation and **≤ 0.5 mA** under single fault conditions, in line with [IEC 60601‑1](https://www.iso.org/standard/72428.html) medical electrical equipment safety standards, which are also referenced by the [U.S. FDA](https://www.fda.gov/medical-devices/standards-and-conformity-assessment-program).
- Always verify EMS output with instrumentation before human trials.
- Use only safe electrodes and medically approved insulation.


---

## Author

**Yu-Chu Hsieh, Yuxuan Miao, Enjia Shi, Hongrui Wu**
Electrical and Computer Engineering
University of Washington
