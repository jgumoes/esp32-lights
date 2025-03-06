#ifndef __TIME_HELPERS__
#define __TIME_HELPERS__

#include <Arduino.h>

const uint32_t secondsInDay = 60*60*24;

uint32_t static timeToSeconds(uint8_t hours, uint8_t minutes, uint8_t seconds){
  return seconds + 60*(minutes + 60*hours);
}

struct UsefulTimeStruct {
  uint32_t timeInDay;   // seconds since midnight
  uint64_t startOfDay;  // timestamp of midnight
  uint8_t dayOfWeek;    // from 1 (Monday) to 7 (Sunday)
};

UsefulTimeStruct static getUsefulTimeStruct(uint64_t timestampS){
  UsefulTimeStruct out;
  DateTimeStruct dt;
  // get timeInDay and current day
  convertFromLocalTimestamp(timestampS, &dt);
  out.timeInDay = dt.seconds + 60*(dt.minutes + 60*dt.hours);
  out.dayOfWeek = dt.dayOfWeek;

  // get the timestamp for the start of the day
  dt.hours = 0;
  dt.minutes = 0;
  dt.seconds = 0;
  out.startOfDay = convertToLocalTimestamp(&dt);
  return out;
}

#endif