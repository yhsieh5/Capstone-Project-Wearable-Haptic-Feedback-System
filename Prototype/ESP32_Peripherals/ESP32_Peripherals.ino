/* 
 * Copyright 2025 Yu-Chu Hsieh, Yuxuan Miao, Enjia Shi, Hongrui Wu
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <SparkFun_BNO080_Arduino_Library.h>
#include <Adafruit_DRV2605.h>
#include <Wire.h>


// Device Name
// #define DEVICE_NAME "HaptiDrum_Foot_L"
// #define DEVICE_NAME "HaptiDrum_Foot_R"
#define DEVICE_NAME "HaptiDrum_Hand_L"
// #define DEVICE_NAME "HaptiDrum_Hand_R"

// BLE
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// EMS
#define MCP4725_ADDR 0x60
#define VREF 3.3

const int pinEMS_IN1 = 1;  // AGD1636 IN1
const int pinEMS_IN2 = 2;  // AGD1636 IN2
const int pinEMS_EN = 7;   // AGD1636 ENABLE


BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

Adafruit_DRV2605 drv;
BNO080 myIMU;
unsigned long lastUpdate = 0;
bool imuInitialized = false;


// Trigger a simple vibration with given strength and duration
void vibrate(int strength, int durationMs) {
  drv.setMode(5);
  drv.setRealtimeValue(strength);
  delay(durationMs);
  drv.setRealtimeValue(0);
  drv.setMode(0);
}

// Handle BLE write events
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String rxValue = pCharacteristic->getValue().c_str();

    if (rxValue == "FIND") {
      vibrate(127, 3000);
    } else if (rxValue == "CONNECT") {
      for (int i = 0; i < 3; i++) {
        vibrate(127, 250);
        delay(250);
      }
    } else if (rxValue == "RESET" || rxValue.indexOf("\"type\":\"RESET\"") >= 0) {
      Serial.println("Received RESET command. Reinitializing IMU...");
      myIMU.enableRotationVector(20);
      myIMU.enableGyro(20);
    } else if (rxValue == "PLAY") {
      Serial.println("Received PLAY command. Vibrating 0.5s...");
      vibrate(127, 100);
      ems(100, 6, 3.3, 100);
    }
  }
};

// Handle BLE connection events
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
  }
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    BLEDevice::startAdvertising();
    Serial.println("Disconnected. Restart advertising...");
  }
};


// Initialize BNO085 IMU
bool initIMU() {
  Serial.println("Initializing BNO085...");
  if (!myIMU.begin(BNO080_DEFAULT_ADDRESS, Wire, 500)) {
    Serial.println("BNO085 not detected! Check wiring.");
    return false;
  }
  myIMU.softReset();
  delay(500);
  myIMU.enableRotationVector(20);
  myIMU.enableGyro(20);
  Serial.println("BNO085 ready.");
  return true;
}

// Initialize BLE service, characteristic and advertising
void initBLE() {
  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("BLE Server is ready and advertising.");
}

// Initialize EMS
void initEMS() {
  pinMode(pinEMS_IN1, OUTPUT);
  pinMode(pinEMS_IN2, OUTPUT);
  pinMode(pinEMS_EN, OUTPUT);
  digitalWrite(pinEMS_EN, HIGH);  // enable ems chip
}

// DAC
void setVoltage(uint16_t val) {
  Wire.beginTransmission(MCP4725_ADDR);
  Wire.write(0x40);
  Wire.write(val >> 4);
  Wire.write((val & 0xF) << 4);
  Wire.endTransmission();
}

// EMS
void ems(float frequency, float dutyCycle, float amplitude, int duration_ms) {
  amplitude = constrain(amplitude, 0.0, VREF);
  dutyCycle = constrain(dutyCycle, 0.1, 100.0);
  frequency = constrain(frequency, 1.0, 500.0);

  uint16_t dacValue = (uint16_t)((amplitude / VREF) * 4095);
  unsigned long period_us = (unsigned long)(1e6 / frequency);
  unsigned long ton_us = (unsigned long)(period_us * (dutyCycle / 100.0));
  int cycles = (duration_ms * 1000UL) / period_us;

  for (int i = 0; i < cycles; i++) {
    // high level first half
    setVoltage(dacValue);
    digitalWrite(pinEMS_IN1, HIGH);
    digitalWrite(pinEMS_IN2, LOW);
    delayMicroseconds(ton_us / 2);

    // high level last half
    digitalWrite(pinEMS_IN1, LOW);
    digitalWrite(pinEMS_IN2, HIGH);
    delayMicroseconds(ton_us / 2);

    // low level
    setVoltage(0);
    digitalWrite(pinEMS_IN1, LOW);
    digitalWrite(pinEMS_IN2, LOW);
    delayMicroseconds(period_us - ton_us);
  }
}

// Get IMU data as JSON string
String getIMUData() {
  if (myIMU.dataAvailable()) {
    lastUpdate = millis();
    imuInitialized = true;

    float yaw = myIMU.getYaw() * 180.0 / PI;
    float pitch = myIMU.getPitch() * 180.0 / PI;
    float roll = myIMU.getRoll() * 180.0 / PI;
    float gx = myIMU.getGyroX() * 180.0 / PI;
    float gy = myIMU.getGyroY() * 180.0 / PI;
    float gz = myIMU.getGyroZ() * 180.0 / PI;

    String jsonData = "{\"yaw\": " + String(yaw, 2) +
                      ", \"pitch\": " + String(pitch, 2) +
                      ", \"roll\": " + String(roll, 2) +
                      ", \"gx\": " + String(gx, 2) +
                      ", \"gy\": " + String(gy, 2) +
                      ", \"gz\": " + String(gz, 2) + "}";
    return jsonData;
  }

  if (millis() - lastUpdate > 1000 && imuInitialized) {
    myIMU.enableRotationVector(20);
    myIMU.enableGyro(20);
    lastUpdate = millis();
  }
  return "";
}

// Send message via BLE notify
void sendNotify(const String& msg) {
  if (deviceConnected && msg.length() > 0) {
    pCharacteristic->setValue(msg.c_str());
    pCharacteristic->notify();
    Serial.println("Sent: " + msg);
  }
}

unsigned long lastSend = 0;
unsigned long lastPrint = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize vibration motor driver
  if (!drv.begin()) {
    Serial.println("Failed to initialize DRV2605");
    while (1);
  }
  drv.setMode(5);
  drv.useLRA();

  // Builtin LED feedback
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); delay(1000);
  digitalWrite(LED_BUILTIN, LOW);

  // Start IMU and BLE and EMS
  initIMU();
  initBLE();
  initEMS();
}

void loop() {
  String imuData = getIMUData();

  // manual testing
  if (Serial.available()) {
    char input = Serial.read();
    if (input == 'p') {
      Serial.println("Manual input: PLAY command");
      vibrate(127, 100);
      ems(100, 6, 3.3, 500);
    }
  }

  if (deviceConnected && imuData.length() > 0 && millis() - lastSend > 50) {
    sendNotify(imuData);
    lastSend = millis();
  }

  if (millis() - lastPrint > 3000) {
    Serial.println("looping...");
    lastPrint = millis();
  }
}
