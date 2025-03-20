#include "ConfigManager.h"

RTCConfigsStruct ConfigManagerClass::getRTCConfigs(){
  RTCConfigsStruct rtcConfigs;
  rtcConfigs.DST = _configs.DST;
  rtcConfigs.timezone = _configs.timezone;
  rtcConfigs.maxSecondsBetweenSyncs = _configs.maxSecondsBetweenSyncs;
  return rtcConfigs;
};

bool ConfigManagerClass::setRTCConfigs(RTCConfigsStruct rtcConfigs){
  _configs.DST = rtcConfigs.DST;
  _configs.timezone = rtcConfigs.timezone;
  _configs.maxSecondsBetweenSyncs = rtcConfigs.maxSecondsBetweenSyncs;
  return _configHAL->setConfigs(_configs);
}

EventManagerConfigsStruct ConfigManagerClass::getEventManagerConfigs()
{
  EventManagerConfigsStruct eventConfigs;
  eventConfigs.defaultEventWindow = _configs.defaultEventWindow;
  return eventConfigs;
}

bool ConfigManagerClass::setEventManagerConfigs(EventManagerConfigsStruct eventConfigs)
{
  _configs.defaultEventWindow = eventConfigs.defaultEventWindow;
  return _configHAL->setConfigs(_configs);
}
