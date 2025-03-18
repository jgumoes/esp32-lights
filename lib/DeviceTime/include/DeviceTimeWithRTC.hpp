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

    std::shared_ptr<ConfigManagerClass> _configManager;
    std::shared_ptr<WireClassDependancy> _wireClass;
    std::unique_ptr<OnboardTimestamp> _onboardTimestamp = std::make_unique<OnboardTimestamp>();

    uint64_t _timeofLastSync = 0; // UTC time of the last sync with an external source (i.e. network or RTC chip)

    uint64_t _maxTimeBetweenSyncs;  // in uS

    bool _timeFault = false;

  public:
    DeviceTimeWithRTCClass(
      std::shared_ptr<ConfigManagerClass> configManager,
      std::shared_ptr<WireClassDependancy> wireClass,
      uint64_t maxSecondsBetweenSyncs = 60*60*24
      ) : _configManager(configManager),
          _wireClass(wireClass),
          _maxTimeBetweenSyncs(maxSecondsBetweenSyncs * 1000000),
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

    };

    /**
     * @brief gets the UTC timestamp in seconds
     * 
     * @return uint64_t UTC timestamp in seconds
     */
    uint64_t getUTCTimestampSeconds() override;

    /**
     * @brief get the local timestamp in seconds
     * 
     * @return uint64_t timestamp in seconds
     */
    uint64_t getLocalTimestampSeconds() override;

    /**
     * @brief gets the local timestamp in microseconds
     * 
     * @return uint64_t timestamp in microseconds
     */
    uint64_t getLocalTimestampMicros() override;

    /**
     * @brief gets the local timestamp in milliseconds
     * 
     * @return uint64_t timestamp in milliseconds
     */
    uint64_t getLocalTimestampMillis() override;

    /**
     * @brief gets the current day of the week from 1-7 where 1 is Monday
     * 
     * @return uint8_t day of the week
     */
    uint8_t getDay() override;

    /**
     * @brief gets the current month from 1-12
     * 
     * @return uint8_t current month
     */
    uint8_t getMonth() override;

    /**
     * @brief gets the current years since 2000. i.e. 24 = 2024
     * 
     * @return uint8_t current year
     */
    uint8_t getYear() override;

    /**
     * @brief gets the current date starting at 1
     * 
     * @return uint8_t current date
     */
    uint8_t getDate() override;

    /**
     * @brief gets the local timestamp at 00:00 of the same day
     * 
     * @return uint64_t local timestamp in microseconds
     */
    uint64_t getStartOfDay() override;

    /**
     * @brief get the time since midnight of the same day
     * 
     * @return uint64_t day timestamp in uS
     */
    uint64_t getTimeInDay() override;

    /**
     * @brief sets the UTC timestamp from 2000 epoch. Timezone and DST are in seconds
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     */
    void setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST) override;

    /**
     * @brief sets the UTC timestamp from 1970 epoch. Timezone and DST follow the BLE-SIG specification for Device Time Service
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     */
    void setUTCTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST) override;

    /**
     * @brief sets the local timestamp from 2000 epoch. Timezone and DST follow the BLE-SIG specification for Device Time Service
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     */
    void setLocalTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST) override;

    /**
     * @brief sets the UTC timestamp from 1970 epoch. Timezone and DST follow the BLE-SIG specification for Device Time Service
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     */
    void setLocalTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST) override;

    bool hasTimeFault() override {return _timeFault;}
};

template<typename WireClassDependancy>
uint64_t DeviceTimeWithRTCClass<WireClassDependancy>::getUTCTimestampSeconds(){
  RTCConfigsStruct configVals = _configManager->getRTCConfigs();
  return getLocalTimestampSeconds() - configVals.DST - configVals.timezone;
}

template<typename WireClassDependancy>
uint64_t DeviceTimeWithRTCClass<WireClassDependancy>::getLocalTimestampSeconds(){
  uint64_t time = getLocalTimestampMicros();
  return (time / 1000000) + (time % 1000000 >= 500000);
}

template<typename WireClassDependancy>
uint64_t DeviceTimeWithRTCClass<WireClassDependancy>::getLocalTimestampMicros(){

  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  uint64_t utcTime_uS = _onboardTimestamp->getTimestamp_uS();

  if(utcTime_uS > _timeofLastSync + _maxTimeBetweenSyncs || utcTime_uS <= BUILD_TIMESTAMP){
    // if onboard timestamp needs a sync or has a fault
    uint64_t newUTCTime = _rtcChip.getUTCTimestamp() * 1000000;
    if(newUTCTime > BUILD_TIMESTAMP){
      // if the RTC timestamp seems legit
      _onboardTimestamp->setTimestamp_uS(newUTCTime);
      _timeofLastSync = newUTCTime;

      uint64_t newLocalTime = convertTimestampToLocal(newUTCTime, configs);
      return newLocalTime;
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

    return convertTimestampToLocal(utcTime_uS, configs);
  }
  return convertTimestampToLocal(utcTime_uS, configs);
}

template<typename WireClassDependancy>
uint64_t DeviceTimeWithRTCClass<WireClassDependancy>::getLocalTimestampMillis(){
  uint64_t time = getLocalTimestampMicros();
  return (time / 1000) + (time % 1000 >= 500);
}

template<typename WireClassDependancy>
uint8_t DeviceTimeWithRTCClass<WireClassDependancy>::getDay(){
  DateTimeStruct dt;
  convertFromLocalTimestamp(getLocalTimestampSeconds(), &dt);
  return dt.dayOfWeek;
}

template<typename WireClassDependancy>
uint8_t DeviceTimeWithRTCClass<WireClassDependancy>::getMonth(){
  DateTimeStruct dt;
  convertFromLocalTimestamp(getLocalTimestampSeconds(), &dt);
  return dt.month;
}

template<typename WireClassDependancy>
uint8_t DeviceTimeWithRTCClass<WireClassDependancy>::getYear(){
  DateTimeStruct dt;
  convertFromLocalTimestamp(getLocalTimestampSeconds(), &dt);
  return dt.years;
}

template<typename WireClassDependancy>
uint8_t DeviceTimeWithRTCClass<WireClassDependancy>::getDate(){
  DateTimeStruct dt;
  convertFromLocalTimestamp(getLocalTimestampSeconds(), &dt);
  return dt.date;
}

template<typename WireClassDependancy>
uint64_t DeviceTimeWithRTCClass<WireClassDependancy>::getStartOfDay(){
  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  UsefulTimeStruct uts = makeUsefulTimeStruct(getLocalTimestampSeconds(), configs);
  return uts.startOfDay * 1000000;
}

template<typename WireClassDependancy>
uint64_t DeviceTimeWithRTCClass<WireClassDependancy>::getTimeInDay(){
  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  uint64_t time_uS = getLocalTimestampMicros();
  UsefulTimeStruct uts = makeUsefulTimeStruct(round(time_uS/1000000), configs);
  uint64_t timeInDay = time_uS - (uts.startOfDay * 1000000);
  return timeInDay;
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

template<typename WireClassDependancy>
void DeviceTimeWithRTCClass<WireClassDependancy>::setUTCTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST){
  setUTCTimestamp2000(newTimesamp - SECONDS_BETWEEN_1970_2000, timezone, DST);
}

template<typename WireClassDependancy>
void DeviceTimeWithRTCClass<WireClassDependancy>::setLocalTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST){
  uint64_t utc = newTimesamp - timezone - DST;
  setUTCTimestamp2000(utc, timezone, DST);
}

template<typename WireClassDependancy>
void DeviceTimeWithRTCClass<WireClassDependancy>::setLocalTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST){
  uint64_t localTimestamp = newTimesamp - SECONDS_BETWEEN_1970_2000;
  setLocalTimestamp2000(newTimesamp - SECONDS_BETWEEN_1970_2000, timezone, DST);
}

#endif