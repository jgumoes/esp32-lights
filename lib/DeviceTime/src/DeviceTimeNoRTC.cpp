#include "DeviceTimeNoRTC.h"

uint64_t DeviceTimeNoRTCClass::getUTCTimestampMicros()
{
  uint64_t utcTime_uS = _onboardTimestamp->getTimestamp_uS();
  if(utcTime_uS <= BUILD_TIMESTAMP){
    _timeFault = true;
    utcTime_uS = BUILD_TIMESTAMP;
    _onboardTimestamp->setTimestamp_uS(BUILD_TIMESTAMP);
  }
  else if(utcTime_uS >= _timeOfNextSync){
    _timeFault = true;  // flags the need for an external sync
  }
  return utcTime_uS;
}

bool DeviceTimeNoRTCClass::setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST)
{
  if(
    newTimesamp * 1000000 < BUILD_TIMESTAMP
    || newTimesamp >= ONBOARD_TIMESTAMP_OVERFLOW
    || !isTimezoneValid(timezone)
    || !isDSTValid(DST)
  ){
    return false;
  }
  _timeFault = false;
  _onboardTimestamp->setTimestamp_S(newTimesamp);

  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  if(
    configs.DST != DST
    || configs.timezone != timezone
  ){
    configs.DST = DST;
    configs.timezone = timezone;
    _configManager->setRTCConfigs(configs);
  }
  
  _timeofLastSync = newTimesamp*1000000;
  _timeOfNextSync = _timeofLastSync + _configManager->getRTCConfigs().maxSecondsBetweenSyncs*1000000;
  return true;
}