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
      std::shared_ptr<WireClassDependancy> wireClass,
      uint64_t maxSecondsBetweenSyncs = 60*60*24
      ) : DeviceTimeInterface(configManager, maxSecondsBetweenSyncs),
          _rtcChip(RTCInterfaceClass<WireClassDependancy, ConfigManagerClass>(wireClass, configManager))
    {
      _rtcChip.begin();
      RTCConfigsStruct configs = _configManager->getRTCConfigs();
      uint64_t initialTime = _rtcChip.getUTCTimestamp() * 1000000;
      if(initialTime < BUILD_TIMESTAMP){
        _timeFault = true;
        _rtcChip.setUTCTimestamp(BUILD_TIMESTAMP / 1000000, configs.timezone, configs.DST);
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
     */
    void setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST) override;

};

template <typename WireClassDependancy>
uint64_t DeviceTimeWithRTCClass<WireClassDependancy>::getUTCTimestampMicros()
{
  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  uint64_t utcTime_uS = _onboardTimestamp->getTimestamp_uS();

  if(utcTime_uS > _timeofLastSync + _maxTimeBetweenSyncs || utcTime_uS <= BUILD_TIMESTAMP){
    // if onboard timestamp needs a sync or has a fault
    uint64_t newUTCTime = _rtcChip.getUTCTimestamp() * 1000000;
    if(newUTCTime > BUILD_TIMESTAMP){
      // if the RTC timestamp seems legit
      _onboardTimestamp->setTimestamp_uS(newUTCTime);
      _timeofLastSync = newUTCTime;
      return newUTCTime;
    }

    // if the RTC timestamp also has a fault
    _timeFault = true;
    if(utcTime_uS < BUILD_TIMESTAMP){
      // revert to BUILD_TIMESTAMP if onboard and RTC both have a fault
      utcTime_uS = BUILD_TIMESTAMP;
      _onboardTimestamp->setTimestamp_uS(BUILD_TIMESTAMP);
    }
    utcTime_uS = _onboardTimestamp->getTimestamp_uS();
    uint64_t time_S = (utcTime_uS / 1000000) + (utcTime_uS % 1000000 >= 500000);
    _rtcChip.setUTCTimestamp(time_S, configs.timezone, configs.DST);

    return utcTime_uS;
  }
  return utcTime_uS;
}

template<typename WireClassDependancy>
void DeviceTimeWithRTCClass<WireClassDependancy>::setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST){
  uint64_t timeOverflow = pow(2, 54) - 1;
  if(newTimesamp * 1000000 > BUILD_TIMESTAMP && newTimesamp < timeOverflow){
    _timeFault = false;
    _rtcChip.setUTCTimestamp(newTimesamp, timezone, DST);
    _onboardTimestamp->setTimestamp_S(_rtcChip.getUTCTimestamp());
    uint64_t rtcTimestamp = _rtcChip.getUTCTimestamp();
    _onboardTimestamp->setTimestamp_S(rtcTimestamp);
  }
}

#endif