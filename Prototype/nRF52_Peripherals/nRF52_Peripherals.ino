#include <bluefruit.h>
#include <Wire.h>
#include "LSM6DS3.h"
#include <Adafruit_DRV2605.h>


// Device Name
// #define DEVICE_NAME "HaptiDrum_Foot_L"
#define DEVICE_NAME "HaptiDrum_Foot_R"
// #define DEVICE_NAME "HaptiDrum_Hand_L"
// #define DEVICE_NAME "HaptiDrum_Hand_R"


// ===========================
// ==== IMU + DRV2605 ====
// ===========================
LSM6DS3 myIMU(I2C_MODE, 0x6A);         // IMU at address 0x6A
Adafruit_DRV2605 drv;                 // Vibration motor controller

// ===========================
// ==== BLE Definitions  ====
// ===========================
BLEService customService = BLEService("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
BLECharacteristic customChar = BLECharacteristic("beb5483e-36e1-4688-b7f5-ea07361b26a8");

bool deviceConnected = false;

// ===========================
// ==== Setup ====
// ===========================
void setup() {
  Serial.begin(115200);
  // while (!Serial);

  Wire.begin();

  // ==== Initialize IMU ====
  if (myIMU.begin() != 0) {
    Serial.println("IMU init failed");
  } else {
    Serial.println("IMU ready");
  }

  // ==== Initialize DRV2605 ====
  if (!drv.begin()) {
    Serial.println("DRV2605 init failed");
  } else {
    drv.setMode(DRV2605_MODE_REALTIME);
    drv.useLRA();
    drv.setRealtimeValue(0);
    drv.setMode(0);
  }

  // ==== Initialize BLE ====
  Bluefruit.begin();
  Bluefruit.setName(DEVICE_NAME);
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // BLE Service and Characteristic（with Notify and Write）
  customService.begin();
  customChar.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_WRITE);
  customChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  customChar.setFixedLen(100);
  customChar.setWriteCallback(write_callback);
  customChar.begin();

  // advertising
  Bluefruit.Advertising.addService(customService);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.start(0);

  Serial.println("BLE advertising started");
}

// ===========================
// ==== Loop ====
// ===========================
unsigned long lastSend = 0;

void loop() {
  float ax = myIMU.readFloatAccelX();
  float ay = myIMU.readFloatAccelY();
  float az = myIMU.readFloatAccelZ();

  float pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
  float roll  = atan2(ay, az) * 180.0 / PI;

  float gx = myIMU.readFloatGyroX();
  float gy = myIMU.readFloatGyroY();

  // send pitch/roll every 50ms
  // if (deviceConnected && millis() - lastSend > 50) {
  //   char buf[100];
  //   snprintf(buf, sizeof(buf),
  //           "{\"pitch\": %.2f, \"roll\": %.2f}", pitch, roll);
  //   customChar.notify((uint8_t*)buf, strlen(buf));
  //   Serial.println(buf);  // for print imu data to serial monitor
  //   lastSend = millis();
  // }

  if (deviceConnected && customChar.notifyEnabled() && millis() - lastSend > 50) {
  char buf[100];
  snprintf(buf, sizeof(buf),
          "{\"pitch\": %.2f, \"roll\": %.2f, \"gx\": %.2f, \"gy\": %.2f}",
          pitch, roll, gx, gy);
  customChar.notify((uint8_t*)buf, strlen(buf));
  Serial.println("✅ Sent: " + String(buf));
  lastSend = millis();

} else if (deviceConnected && !customChar.notifyEnabled()) {
  static unsigned long lastWarn = 0;
  if (millis() - lastWarn > 3000) {
    Serial.println("iOS has NOT subscribed to notify. Skipping send.");
    lastWarn = millis();
  }
}


  delay(10);
}

// ===========================
// ==== BLE Callbacks ====
// ===========================
void connect_callback(uint16_t conn_handle) {
  Serial.println("Connected");
  deviceConnected = true;
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("Disconnected");
  deviceConnected = false;
}

// ===========================
// ==== BLE Write Callback ====
// ===========================
void write_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  String received = String((char*)data).substring(0, len);
  Serial.print("Received: ");
  Serial.println(received);

  if (received == "FIND") {
    Serial.println("FIND command received -> Vibrate!");

    drv.setMode(5);
    drv.setRealtimeValue(100);
    delay(1000);
    drv.setRealtimeValue(0);
    drv.setMode(0);

  } else if (received == "CONNECT") {
    Serial.println("CONNECT command received -> Vibrate x2!");
    drv.setMode(5);
      for (int i = 0; i < 3; i++) {
        drv.setRealtimeValue(100);
        delay(300);
        drv.setRealtimeValue(0);
        delay(300);
      }
    drv.setMode(0);

  } else if (received == "PLAY") {

    drv.setMode(5);
    drv.setRealtimeValue(100);
    delay(300);
    drv.setRealtimeValue(0);
    drv.setMode(0);

  } else if (received == "RESET") {
    Serial.println("RESET command received -> Resetting IMU");

    // stop i2c
    Wire.end(); delay(100);
    Wire.begin(); delay(100);

    // reinitialize IMU
    if (myIMU.begin() != 0) {
      Serial.println("❌ IMU re-init failed");
    } else {
      Serial.println("✅ IMU re-initialized");
    }

    // reset send time
    lastSend = millis();
  }
}
