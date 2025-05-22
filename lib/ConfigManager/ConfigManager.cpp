#include "ConfigManager.h"

RTCConfigsStruct ConfigManagerClass::getRTCConfigs(){
  return _configs.rtcConfigs;
};

bool ConfigManagerClass::setRTCConfigs(RTCConfigsStruct rtcConfigs){
  // TODO: lower limit for maxSecondsBetweenSyncs (maybe change to hours between syncs?)
  _configs.rtcConfigs = rtcConfigs;
  return _configHAL->setConfigs(_configs);
}

EventManagerConfigsStruct ConfigManagerClass::getEventManagerConfigs()
{
  return _configs.eventConfigs;
}

bool ConfigManagerClass::setEventManagerConfigs(EventManagerConfigsStruct eventConfigs)
{
  if(eventConfigs.defaultEventWindow_S == 0){return false;}
  _configs.eventConfigs = eventConfigs;
  return _configHAL->setConfigs(_configs);
}

ModalConfigsStruct ConfigManagerClass::getModalConfigs()
{
  if(_configs.modalConfigs.minOnBrightness == 0){_configs.modalConfigs.minOnBrightness = 1;}
  
  return _configs.modalConfigs;
}

// rejects if defaultOnBrightness == 0, or if the windows are larger than a nibble
bool ConfigManagerClass::setModalConfigs(ModalConfigsStruct modalConfigs)
{
  if(
    modalConfigs.minOnBrightness == 0
    || modalConfigs.softChangeWindow >= (1 << 4)
  )
  {return false;}
  _configs.modalConfigs = modalConfigs;
  return _configHAL->setConfigs(_configs);
}
