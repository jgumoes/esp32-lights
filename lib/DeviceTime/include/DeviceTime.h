/**
 * usage: declare a DeviceTimeClass instance as a global variable in main.cpp
 * notes:
 *  - the local timestamp is stored locally and on the RTC chip, but the RTC_interface is built to accept UTC timestamps
 *  - the local timestamp is stored in microseconds because of timer peripheral. It won't overflow until we're long dead and the world is engulfed in a malestrom of hurricanes and drought
 */

#ifndef __DEVICETIME_H__
#define __DEVICETIME_H__

#ifndef native_env
  #include <Arduino.h>
  #include <Wire.h>
#else
  #include <ArduinoFake.h>
  // #include <iostream>
#endif

#include "onboardTimestamp.h"
#include "ConfigManager.h"

template<typename WireClassDependancy>
class DeviceTimeClass{
  private:
    RTCInterfaceClass<WireClassDependancy, ConfigManagerClass> _rtcChip;

    std::shared_ptr<ConfigManagerClass> _configManager;
    std::shared_ptr<WireClassDependancy> _wireClass;
    std::unique_ptr<OnboardTimestamp> _onboardTimestamp = std::make_unique<OnboardTimestamp>();

    uint64_t _timeofLastSync = 0; // time of the last sync with an external source (i.e. network or RTC chip)

    uint64_t _maxTimeBetweenSyncs;  // in uS

  public:
    DeviceTimeClass(
      std::shared_ptr<ConfigManagerClass> configManager,
      std::shared_ptr<WireClassDependancy> wireClass,
      uint64_t maxSecondsBetweenSyncs = 60*60*24
      ) : _configManager(configManager),
          _wireClass(wireClass),
          _maxTimeBetweenSyncs(maxSecondsBetweenSyncs * 1000000),
          _rtcChip(RTCInterfaceClass<WireClassDependancy, ConfigManagerClass>(wireClass, configManager))
    {
      _rtcChip.begin();
    };

    /**
     * @brief gets the UTC timestamp in seconds
     * 
     * @return uint64_t UTC timestamp in seconds
     */
    uint64_t getUTCTimestamp();

    /**
     * @brief get the local timestamp in seconds
     * 
     * @return uint64_t timestamp in seconds
     */
    uint64_t getTimestampSeconds();

    /**
     * @brief gets the local timestamp in microseconds
     * 
     * @return uint64_t timestamp in microseconds
     */
    uint64_t getTimestampMicros();

    /**
     * @brief gets the local timestamp in milliseconds
     * 
     * @return uint64_t timestamp in milliseconds
     */
    uint64_t getTimestampMillis();

    /**
     * @brief gets the current day of the week from 1-7 where 1 is Monday
     * 
     * @return uint8_t day of the week
     */
    uint8_t getDay();

    /**
     * @brief gets the current month from 1-12
     * 
     * @return uint8_t current month
     */
    uint8_t getMonth();

    /**
     * @brief gets the current years since 2000. i.e. 24 = 2024
     * 
     * @return uint8_t current year
     */
    uint8_t getYear();

    /**
     * @brief gets the current date starting at 1
     * 
     * @return uint8_t current date
     */
    uint8_t getDate();

    /**
     * @brief gets the local timestamp at 00:00 of the same day
     * 
     * @return uint64_t local timestamp in microseconds
     */
    uint64_t getStartOfDay();

    /**
     * @brief get the time since midnight of the same day
     * 
     * @return uint64_t day timestamp in uS
     */
    uint64_t getTimeInDay();

    /**
     * @brief sets the UTC timestamp from 2000 epoch. Timezone and DST are in seconds
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     */
    void setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST);

    /**
     * @brief sets the UTC timestamp from 1970 epoch. Timezone and DST follow the BLE-SIG specification for Device Time Service
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     */
    void setUTCTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST);

    /**
     * @brief sets the local timestamp from 2000 epoch. Timezone and DST follow the BLE-SIG specification for Device Time Service
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     */
    void setLocalTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST);

    /**
     * @brief sets the UTC timestamp from 1970 epoch. Timezone and DST follow the BLE-SIG specification for Device Time Service
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     */
    void setLocalTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST);
};

template<typename WireClassDependancy>
uint64_t DeviceTimeClass<WireClassDependancy>::getUTCTimestamp(){
  RTCConfigsStruct configVals = _configManager->getRTCConfigs();
  return getTimestampSeconds() - configVals.DST - configVals.timezone;
}

template<typename WireClassDependancy>
uint64_t DeviceTimeClass<WireClassDependancy>::getTimestampSeconds(){
  uint64_t time = getTimestampMicros();
  return (time / 1000000) + (time % 1000000 >= 500000);
}

template<typename WireClassDependancy>
uint64_t DeviceTimeClass<WireClassDependancy>::getTimestampMicros(){
  uint64_t time_uS = _onboardTimestamp->getTimestamp_uS();
  if(time_uS > _timeofLastSync + _maxTimeBetweenSyncs || time_uS == 0){
    time_uS = _rtcChip.getLocalTimestamp() * 1000000;
    _onboardTimestamp->setTimestamp_uS(time_uS);
    _timeofLastSync = time_uS;
  }
  return time_uS;
}

template<typename WireClassDependancy>
uint64_t DeviceTimeClass<WireClassDependancy>::getTimestampMillis(){
  uint64_t time = getTimestampMicros();
  return (time / 1000) + (time % 1000 >= 500);
}

template<typename WireClassDependancy>
uint8_t DeviceTimeClass<WireClassDependancy>::getDay(){
  DateTimeStruct dt;
  convertFromLocalTimestamp(getTimestampSeconds(), &dt);
  return dt.dayOfWeek;
}

template<typename WireClassDependancy>
uint8_t DeviceTimeClass<WireClassDependancy>::getMonth(){
  DateTimeStruct dt;
  convertFromLocalTimestamp(getTimestampSeconds(), &dt);
  return dt.month;
}

template<typename WireClassDependancy>
uint8_t DeviceTimeClass<WireClassDependancy>::getYear(){
  DateTimeStruct dt;
  convertFromLocalTimestamp(getTimestampSeconds(), &dt);
  return dt.years;
}

template<typename WireClassDependancy>
uint8_t DeviceTimeClass<WireClassDependancy>::getDate(){
  DateTimeStruct dt;
  convertFromLocalTimestamp(getTimestampSeconds(), &dt);
  return dt.date;
}

template<typename WireClassDependancy>
uint64_t DeviceTimeClass<WireClassDependancy>::getStartOfDay(){
  DateTimeStruct dt;
  convertFromLocalTimestamp(getTimestampSeconds(), &dt);
  dt.hours = 0;
  dt.minutes = 0;
  dt.seconds = 0;
  return convertToLocalTimestamp(&dt) * 1000000;
}

template<typename WireClassDependancy>
uint64_t DeviceTimeClass<WireClassDependancy>::getTimeInDay(){
  uint64_t startOfDay = getStartOfDay();
  uint64_t timeInDay = getTimestampMicros() - startOfDay;
  uint64_t uSecondsInDay = (uint64_t)1000000*24*60*60 -1;
  if(timeInDay > uSecondsInDay){
    // if midnight occurs during function call, call it again
    return getTimeInDay();
  }
  return timeInDay;
}

template<typename WireClassDependancy>
void DeviceTimeClass<WireClassDependancy>::setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST){
  // TODO: delay to synchronise RTC with onboard timer
  _rtcChip.setUTCTimestamp(newTimesamp, timezone, DST);
  _onboardTimestamp->setTimestamp_S(_rtcChip.getLocalTimestamp());
}

template<typename WireClassDependancy>
void DeviceTimeClass<WireClassDependancy>::setUTCTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST){
  setUTCTimestamp2000(newTimesamp - SECONDS_BETWEEN_1970_2000, timezone, DST);
}

template<typename WireClassDependancy>
void DeviceTimeClass<WireClassDependancy>::setLocalTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST){
  uint64_t utc = newTimesamp - timezone - DST;
  setUTCTimestamp2000(utc, timezone, DST);
}

template<typename WireClassDependancy>
void DeviceTimeClass<WireClassDependancy>::setLocalTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST){
  setLocalTimestamp2000(newTimesamp - SECONDS_BETWEEN_1970_2000, timezone, DST);
}

#endif