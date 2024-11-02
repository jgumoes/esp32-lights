#include "ConfigManager.h"

RTCConfigsStruct ConfigManagerClass::getRTCConfigs(){
  RTCConfigsStruct rtcConfigs;
  rtcConfigs.DST = _configs.DST;
  rtcConfigs.timezone = _configs.timezone;
  return rtcConfigs;
};

bool ConfigManagerClass::setRTCConfigs(RTCConfigsStruct rtcConfigs){
  _configs.DST = rtcConfigs.DST;
  _configs.timezone = rtcConfigs.timezone;
  _configHAL->setRTCConfigs(rtcConfigs);
  return true;
}
