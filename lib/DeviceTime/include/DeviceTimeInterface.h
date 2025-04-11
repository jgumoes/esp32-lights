#ifndef __DEVICE_TIME_INTERFACE_H__
#define __DEVICE_TIME_INTERFACE_H__

#include <Arduino.h>

#include "ProjectDefines.h"
#include "onboardTimestamp.h"
// #include "ConfigManager.h"
#include "../ConfigManager/ConfigManager.h"
#include "timeHelpers.h"

#define secondsToMicros (uint64_t)1000000

uint64_t static roundMicrosToSeconds(uint64_t time){
  return (time / secondsToMicros) + (time % secondsToMicros >= (secondsToMicros/2));
}

uint64_t static roundMicrosToMillis(uint64_t time){
  return (time / 1000) + (time % 1000 >= 500);
}

/**
 * @brief interface for DeviceTime. getUTCTimestampMicros() and setUTCTimestamp2000() need to be overriden by concrete implementation, but everything else should be RTC-agnostic so is non-virtual
 * 
 */
class DeviceTimeInterface{
  protected:
    std::shared_ptr<ConfigManagerClass> _configManager;
    std::unique_ptr<OnboardTimestamp> _onboardTimestamp = std::make_unique<OnboardTimestamp>();

    bool _timeFault = true;

    uint64_t _timeofLastSync = 0; // UTC time of the last sync with an external source (i.e. network or RTC chip)
    uint64_t _timeOfNextSync = 0; // UTC time in micros of the next automatic sync

  public:
    DeviceTimeInterface(std::shared_ptr<ConfigManagerClass> configManager) : _configManager(configManager){};

    /**
     * @brief get the UTC timestamp in microseconds
     * 
     * @return uint64_t UTC timestamp in microseconds
     */
    uint64_t virtual getUTCTimestampMicros() = 0;

    /**
     * @brief sets the UTC timestamp from 2000 epoch. Timezone and DST are in seconds
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     * @return if operation was successful
     */
    bool virtual setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST) = 0;
    
    // nothing below needs to be overriden
    
    /**
     * @brief gets the local timestamp in microseconds
     * 
     * @return uint64_t timestamp in microseconds
     */
    uint64_t getLocalTimestampMicros();

    /**
     * @brief gets the UTC timestamp in seconds
     * 
     * @return uint64_t UTC timestamp in seconds
     */
    uint64_t getUTCTimestampSeconds();

    /**
     * @brief get the local timestamp in seconds
     * 
     * @return uint64_t timestamp in seconds
     */
    uint64_t getLocalTimestampSeconds();

    /**
     * @brief gets the local timestamp in milliseconds
     * 
     * @return uint64_t timestamp in milliseconds
     */
    uint64_t getLocalTimestampMillis();

    /**
     * @brief gets the current day of the week from 1-7 where 1 is Monday and 7 is Sunday
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
     * @brief sets the UTC timestamp from 1970 epoch.
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     * @return if operation was successful
     */
    bool setUTCTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST);

    /**
     * @brief sets the local timestamp from 2000 epoch.
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     * @return if operation was successful
     */
    bool setLocalTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST);

    /**
     * @brief sets the UTC timestamp from 1970 epoch.
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     * @return if operation was successful
     */
    bool setLocalTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST);

    /**
     * @brief returns true if a time fault is detected and a network update is required
     * 
     * @return true 
     * @return false 
     */
    bool hasTimeFault();

    /**
     * @brief Get the UTC time of last sync in microseconds
     * 
     * @return uint64_t UTC timestamp in microseconds
     */
    uint64_t getTimeOfLastSync(){return _timeofLastSync;}

    /**
     * @brief Get the utc time of the next sync in microseconds
     * 
     * @return uint64_t UTC timestamp in microseconds
     */
    uint64_t getTimeOfNextSync(){return _timeOfNextSync;}

// some helper functions that can be used anywhere without fetching a new timestamp

    /**
     * @brief checks the validity of the timezone, according to BLE-SIG specifications
     * 
     * @param timezone offset in seconds
     * @return true if valid
     */
    bool isTimezoneValid(int32_t timezone){
      return (timezone >= -48*15*60) && (timezone <= 56*15*60) && (timezone % (15*60) == 0);
    }

    /**
     * @brief checks the validity of the DST, according to BLE-SIG specifications
     * 
     * @param DST offset in seconds
     * @return true if valid
     */
    bool isDSTValid(uint16_t DST){
      return (DST == 0) || (DST == 60*60) || (DST == 30*60) || (DST == 2*60*60);
    }
    
    /**
     * @brief converts a UTC timestamp into a local timestamp, using stored configs.
     * TODO: integrate DST time window
     * 
     * @param utcTimestamp_uS utc timestamp in microseconds
     * @return uint64_t local timestamp in microseconds
     */
    uint64_t convertUTCToLocalMicros(uint64_t utcTimestamp_uS);

    /**
     * @brief converts a local timestamp into a UTC timestamp, using stored configs.
     * TODO: integrate DST time window
     * 
     * @param localTimestamp_uS local timestamp in microseconds
     * @return uint64_t utc timestamp in microseconds
     */
    uint64_t convertLocalToUTCMicros(uint64_t localTimestamp_uS);
};

inline uint64_t DeviceTimeInterface::getLocalTimestampSeconds()
{
  return roundMicrosToSeconds(getLocalTimestampMicros());
}

inline uint64_t DeviceTimeInterface::getUTCTimestampSeconds()
{
  return roundMicrosToSeconds(getUTCTimestampMicros());
}

inline uint64_t DeviceTimeInterface::getLocalTimestampMillis()
{
  return roundMicrosToMillis(getLocalTimestampMicros());
}

inline uint64_t DeviceTimeInterface::getLocalTimestampMicros()
{
  return convertUTCToLocalMicros(getUTCTimestampMicros());
}

inline uint8_t DeviceTimeInterface::getDay()
{
  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  UsefulTimeStruct uts = makeUsefulTimeStruct(getLocalTimestampSeconds());
  return uts.dayOfWeek;
}

inline uint8_t DeviceTimeInterface::getMonth()
{
  DateTimeStruct dt;
  convertFromLocalTimestamp(getLocalTimestampSeconds(), &dt);
  return dt.month;
}

inline uint8_t DeviceTimeInterface::getYear()
{
  uint64_t time = getLocalTimestampSeconds();
  return ((4* (time/secondsInDay)) / (4*365.25));
}

inline uint8_t DeviceTimeInterface::getDate()
{
  DateTimeStruct dt;
  convertFromLocalTimestamp(getLocalTimestampSeconds(), &dt);
  return dt.date;
}

inline uint64_t DeviceTimeInterface::getStartOfDay()
{
  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  UsefulTimeStruct uts = makeUsefulTimeStruct(getLocalTimestampSeconds());
  return uts.startOfDay * secondsToMicros;
}

inline uint64_t DeviceTimeInterface::getTimeInDay()
{
  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  uint64_t time_uS = getLocalTimestampMicros();
  UsefulTimeStruct uts = makeUsefulTimeStruct(round(time_uS/secondsToMicros));
  uint64_t timeInDay = time_uS - (uts.startOfDay * secondsToMicros);
  return timeInDay;
}

inline bool DeviceTimeInterface::setUTCTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST)
{
  return setUTCTimestamp2000(newTimesamp - SECONDS_BETWEEN_1970_2000, timezone, DST);
}

inline bool DeviceTimeInterface::setLocalTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST)
{
  uint64_t utc = newTimesamp - timezone - DST;
  return setUTCTimestamp2000(utc, timezone, DST);
}

inline bool DeviceTimeInterface::setLocalTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST)
{
  uint64_t localTimestamp = newTimesamp - SECONDS_BETWEEN_1970_2000;
  return setLocalTimestamp2000(newTimesamp - SECONDS_BETWEEN_1970_2000, timezone, DST);
}

inline bool DeviceTimeInterface::hasTimeFault()
{
  return _timeFault;
}

inline uint64_t DeviceTimeInterface::convertUTCToLocalMicros(uint64_t utcTimestamp_uS)
{
  // TODO: check DST start and end bounds
  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  int64_t offset = (configs.DST + configs.timezone) * secondsToMicros;
  if(abs(offset) > utcTimestamp_uS){
    return 0;
  }
  return utcTimestamp_uS + offset;
}

inline uint64_t DeviceTimeInterface::convertLocalToUTCMicros(uint64_t localTimestamp_uS)
{
  // TODO: check DST start and end bounds
  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  int64_t offset = configs.DST + configs.timezone;
  offset = offset * secondsToMicros;
  if(abs(offset) > localTimestamp_uS){
    return 0;
  }
  return localTimestamp_uS - offset;
}

#endif