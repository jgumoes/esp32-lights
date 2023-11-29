/*
 * If multiple alarms are used, they should be initiated and checked in batches.
 * DateTimeStruct is used instead of timestamps to save unnecessarily repeating
 * computations.
 */
#ifndef _SOFTWARE_ALARM_H_
#define _SOFTWARE_ALARM_H_
// #ifndef test_desktop
//   #include <Arduino.h>
// #endif
#include "RTC_interface.h"

struct AlarmTimeStruct{
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
};

/**
 * @brief Contains and controls a single Day Of The Week alarm.
 * Given the current timestamp, it'll calculate the local timestamp
 * of the next alarm. checkAlarm(currentTimestamp) will test if the
 * current time is less than SoftwareAlarm::_triggerWindow seconds over the
 * current timestamp. If so, alarmCallback will trigger, and the
 * nextTimestamp will be found
 * 
 */
class SoftwareAlarm
{
private:
  bool _isActive = true;     // allows alarm to be paused
  uint64_t _nextTimestamp = 0;  // timestamp of the next alarm in seconds since 1/1/2000
  AlarmTimeStruct _alarmTime; // time of alarm
  uint32_t _alarmSecondsFromMidnight = 0;

  uint8_t _daysFlag;   // bit flag for days of the week. LSB is Monday, MSB is Sunday

  uint16_t _triggerWindow = 60 * 60 * 5;  // i.e. how long after _nextTimestamp should the alarm still trigger

  void findNextTimestamp(DateTimeStruct datetime, uint32_t timestamp); // should be called in constructor and any setter
public:
  SoftwareAlarm(AlarmTimeStruct timeOfAlarm, uint8_t daysFlag, DateTimeStruct localTimeStruct, uint32_t localTimestamp);
  // SoftwareAlarm(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t daysFlag, DateTimeStruct now, void alarmCallback);
  SoftwareAlarm(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t daysFlag, DateTimeStruct now, uint32_t localTimestamp);
  // ~SoftwareAlarm();

  // uint32_t getAlarmTime();
  // uint8_t getDaysFlag();
  uint64_t getNextTimestamp();

  // void setAlarmTime(uint32_t _alarmTime);
  // void setDaysFlag(uint8_t daysFlag);

  // void checkAlarm(uint64_t currentTimestamp);  // check if the alarm should trigger
};

#endif