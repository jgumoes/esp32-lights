#ifndef __TIME_HELPERS__
#define __TIME_HELPERS__

#include <Arduino.h>

const uint32_t secondsInDay = 60*60*24;

uint32_t static timeToSeconds(uint8_t hours, uint8_t minutes, uint8_t seconds){
  return seconds + 60*(minutes + 60*hours);
}

struct UsefulTimeStruct {
  uint32_t timeInDay = 0;   // seconds since midnight
  uint64_t startOfDay = 0;  // timestamp of midnight
  uint8_t dayOfWeek = 0;    // from 1 (Monday) to 7 (Sunday)
};

/**
 * @brief constructs a UsefulTimeStruct from a local timestamp and configs
 * 
 * @param timestampS local timestamp in seconds
 * @param configs 
 * @return UsefulTimeStruct 
 */
UsefulTimeStruct static makeUsefulTimeStruct(uint64_t timestampS){
  // TODO: remove this function and integrate it with DeviceTime
  uint64_t startOfDay = (timestampS/(secondsInDay))*secondsInDay;
  uint32_t timeInDay = timestampS - startOfDay;
  uint64_t timestampDays = timestampS / secondsInDay;
  uint8_t dayOfWeek = ((timestampDays) + 5) % 7 + 1;
  UsefulTimeStruct out = {timeInDay, startOfDay, dayOfWeek};
  return out;
}

#endif