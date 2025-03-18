#ifndef __TIME_HELPERS__
#define __TIME_HELPERS__

#include <Arduino.h>
#include <cmath>

const uint32_t secondsInDay = 60*60*24;

uint32_t static timeToSeconds(uint8_t hours, uint8_t minutes, uint8_t seconds){
  return seconds + 60*(minutes + 60*hours);
}

struct UsefulTimeStruct {
  uint32_t timeInDay;   // seconds since midnight
  uint64_t startOfDay;  // timestamp of midnight
  uint8_t dayOfWeek;    // from 1 (Monday) to 7 (Sunday)
};

UsefulTimeStruct static makeUsefulTimeStruct(uint64_t timestampS, RTCConfigsStruct configs){
  // TODO: remove this function and integrate it with DeviceTime
  uint64_t startOfDay = (timestampS/(secondsInDay))*secondsInDay - configs.DST;
  uint32_t timeInDay = timestampS - startOfDay;
  uint8_t dayOfWeek = ((startOfDay/secondsInDay) + 6) % 7;
  UsefulTimeStruct out = {timeInDay, startOfDay, dayOfWeek};
  return out;
}

/**
 * @brief converts a UTC timestamp into a local timestamp
 * 
 * @param utcTimestamp_uS utc timestamp in microseconds
 * @param configVals 
 * @return uint64_t local timestamp in microseconds
 */
uint64_t static convertTimestampToLocal(uint64_t utcTimestamp_uS, RTCConfigsStruct configVals){
  // int64_t offset = (configVals.DST + configVals.timezone) * 1000000;
  int64_t offset = configVals.DST + configVals.timezone;
  offset = offset * 1000000;
  if(abs(offset) > utcTimestamp_uS){
    return 0;
  }
  return utcTimestamp_uS + offset;
}

/**
 * @brief converts a local timestamp into a UTC timestamp
 * 
 * @param utcTimestamp_uS local timestamp in microseconds
 * @param configVals 
 * @return uint64_t UTC timestamp in microseconds
 */
uint64_t static convertTimestampToUTC(uint64_t localTimestamp_uS, RTCConfigsStruct configVals){
  int64_t offset = configVals.DST + configVals.timezone;
  offset = offset * 1000000;
  if(abs(offset) > localTimestamp_uS){
    return 0;
  }
  return localTimestamp_uS - offset;
}


#endif