#include "ConfigManager.h"

RTCConfigsStruct ConfigManagerClass::getRTCConfigs(){
  // RTCConfigsStruct rtcConfigs;
  // rtcConfigs.DST = _configs.DST;
  // rtcConfigs.timezone = _configs.timezone;
  // rtcConfigs.maxSecondsBetweenSyncs = _configs.maxSecondsBetweenSyncs;
  return _configs.rtcConfigs;
};

bool ConfigManagerClass::setRTCConfigs(RTCConfigsStruct rtcConfigs){
  // _configs.DST = rtcConfigs.DST;
  // _configs.timezone = rtcConfigs.timezone;
  // _configs.maxSecondsBetweenSyncs = rtcConfigs.maxSecondsBetweenSyncs;
  // TODO: lower limit for maxSecondsBetweenSyncs (maybe change to hours between syncs?)
  _configs.rtcConfigs = rtcConfigs;
  return _configHAL->setConfigs(_configs);
}

EventManagerConfigsStruct ConfigManagerClass::getEventManagerConfigs()
{
  // EventManagerConfigsStruct eventConfigs;
  // eventConfigs.defaultEventWindow_S = _configs.defaultEventWindow_S;
  return _configs.eventConfigs;
}

bool ConfigManagerClass::setEventManagerConfigs(EventManagerConfigsStruct eventConfigs)
{
  if(eventConfigs.defaultEventWindow_S == 0){return false;}
  // _configs.defaultEventWindow_S = eventConfigs.defaultEventWindow_S;
  _configs.eventConfigs = eventConfigs;
  return _configHAL->setConfigs(_configs);
}

ModalConfigsStruct ConfigManagerClass::getModalConfigs()
{
  // ModalConfigsStruct modalConfigs;
  // modalConfigs.changeoverWindow = _configs.changeoverWindow;
  // modalConfigs.defaultOnBrightness = _configs.defaultOnBrightness == 0 ? 1 : _configs.defaultOnBrightness;
  // modalConfigs.softChangeWindow = _configs.softChangeWindow;

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
  // _configs.changeoverWindow = modalConfigs.changeoverWindow;
  // _configs.defaultOnBrightness = modalConfigs.defaultOnBrightness;
  // _configs.softChangeWindow = modalConfigs.softChangeWindow;
  _configs.modalConfigs = modalConfigs;
  return _configHAL->setConfigs(_configs);
}
