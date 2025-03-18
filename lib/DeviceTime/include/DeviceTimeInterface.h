#ifndef __DEVICE_TIME_INTERFACE_H__
#define __DEVICE_TIME_INTERFACE_H__

#include <Arduino.h>

#include "ProjectDefines.h"
#include "onboardTimestamp.h"
#include "ConfigManager.h"
#include "timeHelpers.h"

class DeviceTimeInterface{
  public:
    DeviceTimeInterface(){};

    /**
     * @brief gets the UTC timestamp in seconds
     * 
     * @return uint64_t UTC timestamp in seconds
     */
    uint64_t virtual getUTCTimestampSeconds() = 0;

    /**
     * @brief get the local timestamp in seconds
     * 
     * @return uint64_t timestamp in seconds
     */
    uint64_t virtual getLocalTimestampSeconds() = 0;

    /**
     * @brief gets the local timestamp in microseconds
     * 
     * @return uint64_t timestamp in microseconds
     */
    uint64_t virtual getLocalTimestampMicros() = 0;

    /**
     * @brief gets the local timestamp in milliseconds
     * 
     * @return uint64_t timestamp in milliseconds
     */
    uint64_t virtual getLocalTimestampMillis() = 0;

    /**
     * @brief gets the current day of the week from 1-7 where 1 is Monday and 7 is Sunday
     * 
     * @return uint8_t day of the week
     */
    uint8_t virtual getDay() = 0;

    /**
     * @brief gets the current month from 1-12
     * 
     * @return uint8_t current month
     */
    uint8_t virtual getMonth() = 0;

    /**
     * @brief gets the current years since 2000. i.e. 24 = 2024
     * 
     * @return uint8_t current year
     */
    uint8_t virtual getYear() = 0;

    /**
     * @brief gets the current date starting at 1
     * 
     * @return uint8_t current date
     */
    uint8_t virtual getDate() = 0;

    /**
     * @brief gets the local timestamp at 00:00 of the same day
     * 
     * @return uint64_t local timestamp in microseconds
     */
    uint64_t virtual getStartOfDay() = 0;

    /**
     * @brief get the time since midnight of the same day
     * 
     * @return uint64_t day timestamp in uS
     */
    uint64_t virtual getTimeInDay() = 0;

    /**
     * @brief sets the UTC timestamp from 2000 epoch. Timezone and DST are in seconds
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     */
    void virtual setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST) = 0;

    /**
     * @brief sets the UTC timestamp from 1970 epoch.
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     */
    void virtual setUTCTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST) = 0;

    /**
     * @brief sets the local timestamp from 2000 epoch.
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     */
    void virtual setLocalTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST) = 0;

    /**
     * @brief sets the UTC timestamp from 1970 epoch.
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     */
    void virtual setLocalTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST) = 0;

    /**
     * @brief returns true if a time fault is detected and a network update is required
     * 
     * @return true 
     * @return false 
     */
    bool virtual hasTimeFault() = 0;
};



#endif