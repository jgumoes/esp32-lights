#ifndef __DEVICE_TIME_WITH_RTC__
#define __DEVICE_TIME_WITH_RTC__

#include "DeviceTimeInterface.h"
#include "RTC_interface.h"

#ifdef native_env
// #include <iostream>
#endif

/**
 * @brief DeviceTimeClass with an I2C RTC chip
 * 
 * @tparam WireClassDependancy 
 */
template<typename WireClassDependancy>
class DeviceTimeWithRTCClass : public DeviceTimeInterface{
  private:
    RTCInterfaceClass<WireClassDependancy, ConfigManagerClass> _rtcChip;

  public:
    DeviceTimeWithRTCClass(
      std::shared_ptr<ConfigManagerClass> configManager,
      std::shared_ptr<WireClassDependancy> wireClass
      ) : DeviceTimeInterface(configManager),
          _rtcChip(RTCInterfaceClass<WireClassDependancy, ConfigManagerClass>(wireClass, configManager))
    {
      _rtcChip.begin();
      RTCConfigsStruct configs = _configManager->getRTCConfigs();
      uint64_t initialTime = _rtcChip.getUTCTimestamp() * secondsToMicros;
      if(initialTime < BUILD_TIMESTAMP){
        _timeFault = true;
        _rtcChip.setUTCTimestamp(BUILD_TIMESTAMP / secondsToMicros, configs.timezone, configs.DST);
        _onboardTimestamp->setTimestamp_uS(BUILD_TIMESTAMP);
      }
      else{
        _timeFault = false;
      }

    };

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

template <typename WireClassDependancy>
uint64_t DeviceTimeWithRTCClass<WireClassDependancy>::getUTCTimestampMicros()
{
  uint64_t utcTime_uS = _onboardTimestamp->getTimestamp_uS();
  // TODO: see if RTC has detected a time fault
  if(
    utcTime_uS >= _timeOfNextSync
    || utcTime_uS <= BUILD_TIMESTAMP
  ){
    // if onboard timestamp needs a sync or has a fault
    RTCConfigsStruct configs = _configManager->getRTCConfigs();
    uint64_t newUTCTime_uS = _rtcChip.getUTCTimestamp() * secondsToMicros;
    if(newUTCTime_uS > BUILD_TIMESTAMP){
      // if the RTC timestamp seems legit
      _onboardTimestamp->setTimestamp_uS(newUTCTime_uS);
      _timeofLastSync = newUTCTime_uS;
      _timeOfNextSync = newUTCTime_uS  + (configs.maxSecondsBetweenSyncs*secondsToMicros);
      // TODO: modalLights callback of time adjustment
      return newUTCTime_uS;
    }

    // if the RTC timestamp also has a fault
    _timeFault = true;
    if(utcTime_uS < BUILD_TIMESTAMP){
      // revert to BUILD_TIMESTAMP if onboard and RTC both have a fault
      utcTime_uS = BUILD_TIMESTAMP;
      _onboardTimestamp->setTimestamp_uS(BUILD_TIMESTAMP);
    }
    utcTime_uS = _onboardTimestamp->getTimestamp_uS();
    uint64_t time_S = (utcTime_uS / secondsToMicros) + (utcTime_uS % secondsToMicros >= (secondsToMicros/2));
    _rtcChip.setUTCTimestamp(time_S, configs.timezone, configs.DST);

    return utcTime_uS;
  }
  return utcTime_uS;
}

template<typename WireClassDependancy>
bool DeviceTimeWithRTCClass<WireClassDependancy>::setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST){
  // const uint64_t timeOverflow = (~(uint64_t)0) >> (64-54);
  if(
    newTimesamp * secondsToMicros < BUILD_TIMESTAMP
    || newTimesamp >= ONBOARD_TIMESTAMP_OVERFLOW
    || !isTimezoneValid(timezone)
    || !isDSTValid(DST)
  ){
    return false;
  }
  _timeFault = false;
  // new configs are currently set by _rtcChip. I know, I don't like it either
  _onboardTimestamp->setTimestamp_S(newTimesamp);
  _rtcChip.setUTCTimestamp(newTimesamp, timezone, DST);
  // uint64_t rtcTimestamp = _rtcChip.getUTCTimestamp();
  _timeofLastSync = newTimesamp*secondsToMicros;
  _timeOfNextSync = (newTimesamp + _configManager->getRTCConfigs().maxSecondsBetweenSyncs)*secondsToMicros;
  return true;
}

#endif