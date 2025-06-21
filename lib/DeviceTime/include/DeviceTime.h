/**
 * notes:
 *  - the local timestamp is stored locally and on the RTC chip, but the RTC_interface is built to accept UTC timestamps
 *  - the local timestamp is stored in microseconds because of timer peripheral,but it won't overflow until we're long dead and the world is engulfed in a malestrom of hurricanes and drought
 */

#ifndef __DEVICETIME_H__
#define __DEVICETIME_H__

#include <Arduino.h>
#include <etl/observer.h>

#include "ProjectDefines.h"
#include "onboardTimestamp.h"
#include "ConfigStorageClass.hpp"
#include "timeHelpers.h"

uint64_t static roundMicrosToSeconds(uint64_t time){
  return (time / secondsToMicros) + (time % secondsToMicros >= (secondsToMicros/2));
}

uint64_t static roundMicrosToMillis(uint64_t time){
  return (time / 1000) + (time % 1000 >= 500);
}

typedef etl::observer<const TimeUpdateStruct&> TimeObserver;

#ifndef MAX_TIME_OBSERVERS
#define MAX_TIME_OBSERVERS 2
#endif

/**
 * @brief interface for DeviceTime. getUTCTimestampMicros() and setUTCTimestamp2000() need to be overriden by concrete implementation, but everything else should be RTC-agnostic so is non-virtual
 * 
 */
class DeviceTimeClass :
  public etl::observable<TimeObserver, MAX_TIME_OBSERVERS>,
  public ConfigUser
{
  private:
    std::shared_ptr<ConfigStorageClass> _configStorage;
    std::unique_ptr<OnboardTimestamp> _onboardTimestamp = std::make_unique<OnboardTimestamp>();

    bool _timeFault = true;

    uint64_t _timeofLastSync_uS = 0; // UTC time of the last sync with an external source (i.e. network or RTC chip)
    uint64_t _timeOfNextSync_uS = 0; // UTC time in micros of the next automatic sync

    TimeConfigsStruct _configs;
    int64_t _offset = 0;  // local time = UTC + offset

    void _setOffset(){
      _offset = (_configs.DST + _configs.timezone) * secondsToMicros;
    }

  public:
    DeviceTimeClass(std::shared_ptr<ConfigStorageClass> configStorage) : _configStorage(configStorage), ConfigUser(ModuleID::deviceTime){
      _configStorage->registerUser(this, _configs);
    };

    /**
     * @brief get the UTC timestamp in microseconds
     * 
     * @return uint64_t UTC timestamp in microseconds
     */
    uint64_t getUTCTimestampMicros();

    /**
     * @brief sets the UTC timestamp from 2000 epoch. Timezone and DST are in seconds
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     * @return if operation was successful
     */
    bool setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST);
    
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
     * @brief Set the local timestamp without config changes. use to sync with RTC chip
     * 
     * @param newTimestamp 
     * @return true 
     * @return false 
     */
    bool setLocalTimestamp2000(uint64_t newTimestamp){
      return setLocalTimestamp2000(newTimestamp, _configs.timezone, _configs.DST);
    };

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
    uint64_t getTimeOfLastSync(){return _timeofLastSync_uS;}

    /**
     * @brief Get the utc time of the next sync in microseconds
     * 
     * @return uint64_t UTC timestamp in microseconds
     */
    uint64_t getTimeOfNextSync(){return _timeOfNextSync_uS;}

    bool setMaxTimeBetweenSyncs(uint32_t timeBetweenSyncs_S){
      if(timeBetweenSyncs_S == 0){return false;}
      _configs.maxSecondsBetweenSyncs = timeBetweenSyncs_S;
      _timeOfNextSync_uS = _timeofLastSync_uS + (timeBetweenSyncs_S*secondsToMicros);

      _timeFault = getUTCTimestampMicros() >= _timeOfNextSync_uS;
      return true;
    }

// some helper functions that can be used anywhere without fetching a new timestamp

    /**
     * @brief Get the current Device Time configs
     * 
     * @return RTCConfigsStruct 
     */
    TimeConfigsStruct getConfigs(){return _configs;}

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

// ConfigUser overrides

    void newConfigs(const byte newConfig[maxConfigSize]) override;

    packetSize_t getConfigs(byte config[maxConfigSize]) override {
      ConfigStructFncs::serialize(config, _configs);
      return getConfigPacketSize(ModuleID::deviceTime);
    }
};

#endif