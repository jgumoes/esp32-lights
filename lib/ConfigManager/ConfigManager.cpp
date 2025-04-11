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
  if(eventConfigs.defaultEventWindow == 0){return false;}
  _configs.defaultEventWindow = eventConfigs.defaultEventWindow;
  return _configHAL->setConfigs(_configs);
}

ModalConfigsStruct ConfigManagerClass::getModalConfigs()
{
  ModalConfigsStruct modalConfigs;
  modalConfigs.changeoverWindow = _configs.changeoverWindow;
  modalConfigs.minOnBrightness = _configs.minOnBrightness == 0 ? 1 : _configs.minOnBrightness;
  modalConfigs.softChangeWindow = _configs.softChangeWindow;
  return modalConfigs;
}

// rejects if minOnBrightness == 0, or if the windows are larger than a nibble
bool ConfigManagerClass::setModalConfigs(ModalConfigsStruct modalConfigs)
{
  if(
    modalConfigs.minOnBrightness == 0
    || modalConfigs.changeoverWindow >= (1 << 4)
    || modalConfigs.softChangeWindow >= (1 << 4)
  )
  {return false;}
  _configs.changeoverWindow = modalConfigs.changeoverWindow;
  _configs.minOnBrightness = modalConfigs.minOnBrightness;
  _configs.softChangeWindow = modalConfigs.softChangeWindow;
  return _configHAL->setConfigs(_configs);
}
