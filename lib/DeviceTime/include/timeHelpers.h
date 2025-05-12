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

  /**
   * @brief Construct a new UsefulTimeStruct from a given timestamp
   * 
   * @param timestamp_S 
   */
  UsefulTimeStruct(uint64_t timestamp_S){
    uint16_t timestamp_days = timestamp_S/secondsInDay;
    startOfDay = timestamp_days * secondsInDay;
    timeInDay = timestamp_S - startOfDay;
    dayOfWeek = ((timestamp_days) + 5) % 7 + 1;
  };
};

#endif