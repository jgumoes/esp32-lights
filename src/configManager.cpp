#include <Wire.h>
#include <Arduino.h>
#include "configManager.h"

#define EEPROM_ADDRESS 0x57

#define CONFIG_ADDRESS 501

// TODO: this causes errors
// const uint8_t* rtcConfigsAddress = reinterpret_cast<uint8_t*>(CONFIG_ADDRESS);
const uint8_t rtcConfigsAddress[2] = {0b00000001, 0b11110101};

ConfigManagerClass::ConfigManagerClass(/* args */)
{
}


RTCConfigsStruct ConfigManagerClass::getRTCConfigs(){
  RTCConfigsStruct configs;
  Serial.println("loading configs...");
  Serial.print("configs address 0: "); Serial.print(rtcConfigsAddress[0]);
  Serial.print("1: "); Serial.println(rtcConfigsAddress[1]);
  // uint8_t status = eep.read(CONFIG_ADDRESS, reinterpret_cast<uint8_t*>(&configs), sizeof(configs));
  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write(rtcConfigsAddress[0]);
  Wire.write(rtcConfigsAddress[1]);
  uint8_t status = Wire.endTransmission();
  Serial.print("\tread status: "); Serial.println(status);

  Wire.requestFrom(EEPROM_ADDRESS, sizeof(configs));
  Wire.readBytes(reinterpret_cast<uint8_t*>(&configs), sizeof(configs));

  Serial.print("\tread DST: "); Serial.println(configs.DST);
  Serial.print("\tread timezone: "); Serial.println(configs.timezone);
  return configs;
}
  
uint8_t ConfigManagerClass::setRTCConfigs(RTCConfigsStruct configs){
  // uint8_t status = eep.write(CONFIG_ADDRESS, reinterpret_cast<uint8_t*>(&configs), sizeof(configs));
  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write(rtcConfigsAddress[0]);
  Wire.write(rtcConfigsAddress[1]);
  // uint8_t* writeBuffer = reinterpret_cast<uint8_t*>(&configs);
  // for(int i = 0; i < sizeof(configs); i++){
  //   Wire.write(writeBuffer[i]);
  // }
  Wire.write(reinterpret_cast<uint8_t*>(&configs), sizeof(configs));
  // eep.write(CONFIG_ADDRESS, , sizeof(configs));
  uint8_t status = Wire.endTransmission();
  Serial.print("write status: "); Serial.println(status);
  return true;
}