#ifndef __DEVICE_TIME_NO_RTC__
#define __DEVICE_TIME_NO_RTC__

#include "DeviceTimeInterface.h"

class DeviceTimeNoRTCClass : public DeviceTimeInterface{
  public:
    DeviceTimeNoRTCClass(std::shared_ptr<ConfigManagerClass> configManager)
      : DeviceTimeInterface(configManager)
    {
      _onboardTimestamp->setTimestamp_uS(BUILD_TIMESTAMP);
      _timeFault = true;
    }

    uint64_t getUTCTimestampMicros() override;

     /**
     * @brief sets the UTC timestamp from 2000 epoch. Timezone and DST are in seconds
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     * @return if operation was successful
     */
    bool setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST) override;
};

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

#endif