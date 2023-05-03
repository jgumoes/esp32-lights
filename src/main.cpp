// third-party libraries
#include <Arduino.h>
#include <NimBLEDevice.h>

// my libraries
#include "DeviceTimeService.h"

// source files
#include "touch_switch.h"
#include "ac_lights_pwm.h"

static NimBLEServer* bleServer;

class ServerCallbacks: public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer){
    NimBLEDevice::stopAdvertising();
  }

  void onDisconnect(NimBLEServer* pServer) {
    Serial.println("Client disconnected - start advertising");
    NimBLEDevice::startAdvertising();
  }

  void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) {
    Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, desc->conn_handle);
  };
};

void setup() {
  Serial.begin(9600);
  Serial.println("setting up MCPWM");
  setup_PWM();

  setupTouch();

  // setup ble server
  NimBLEDevice::init("Smart-Lights");
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT ); // TODO: change
  NimBLEDevice::setSecurityAuth(true, false, false);
  bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new ServerCallbacks());

  // setup ble services
  setupDeviceTimeService(bleServer);

  // start advertising
  NimBLEAdvertising* bleAdvertising = NimBLEDevice::getAdvertising();
  bleAdvertising->setScanResponse(true);
  bleAdvertising->start();

  pinMode(15, INPUT);
}

void loop() {

#ifdef DEVKIT
  uint pot_val = analogRead(15);
  // uint duty = map(pot_val, 0, 4095, 0, 50);
  float duty = 100. * pot_val / 4095;
  set_PWM_Duty(duty);
  // Serial.print("duty cycle: "); Serial.print(duty); Serial.println("%");
#endif

// #ifdef PRINT_TOUCH
//   print_Touch();
// #endif
  update_Lights();

  // Serial.print("lights state: "); Serial.println(get_Lights_State());
  delay(200);
}