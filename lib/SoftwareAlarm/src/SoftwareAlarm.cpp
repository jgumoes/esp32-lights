#include "SoftwareAlarm.h"

SoftwareAlarm::SoftwareAlarm(AlarmTimeStruct timeOfAlarm, uint8_t daysFlag, DateTimeStruct localTimeStruct, uint32_t localTimestamp){
  _alarmTime = timeOfAlarm;
  _daysFlag = daysFlag;
  _alarmSecondsFromMidnight = _alarmTime.seconds + 60 * (_alarmTime.minutes + 60 * _alarmTime.hours);
  findNextTimestamp(localTimeStruct, localTimestamp);
}


SoftwareAlarm::SoftwareAlarm(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t daysFlag, DateTimeStruct localTimeStruct, uint32_t localTimestamp)
{
  _alarmTime.seconds = seconds;
  _alarmTime.minutes = minutes;
  _alarmTime.hours = hours;
  _daysFlag = daysFlag;
  _alarmSecondsFromMidnight = _alarmTime.seconds + 60 * (_alarmTime.minutes + 60 * _alarmTime.hours);
  findNextTimestamp(localTimeStruct, localTimestamp);
}

/**
 * @brief finds the timestamp of the next alarm. use convertLocalTimestamp
 * from RTC_interface to fill datetime.
 * 
 * @param datetime 
 */
void SoftwareAlarm::findNextTimestamp(DateTimeStruct datetime, uint32_t timestamp)
{
  // note: datetime should not be modified in this method

  // if(_nextTimestamp > 0){
  //   // _nextTimestamp is 0 when class initiates
  //   datetime.dayOfWeek = (datetime.dayOfWeek % 7) + 1;
  // }

  const uint32_t secondsFromMidnight = datetime.seconds + 60 * (datetime.minutes + 60 * datetime.hours);

  // these settings will always need to happen regardless
  datetime.hours = _alarmTime.hours;
  datetime.minutes = _alarmTime.minutes;
  datetime.seconds = _alarmTime.seconds;
  
  if((_daysFlag & (1 << (datetime.dayOfWeek - 1))) != 0){
    // if the day is an alarm day
    if(secondsFromMidnight < _alarmSecondsFromMidnight){
      // if the time is before the alarm time
      _nextTimestamp = convertToLocalTimestamp(&datetime);
      return;
    }
    else {
      // otherwise, start checking from the next day
      datetime.dayOfWeek = (datetime.dayOfWeek % 7) + 1;
      datetime.date += 1;   // this will be corrected if needed
    }
  }

  // search for the date of the next alarm
  while((_daysFlag & (1 << (datetime.dayOfWeek - 1))) == 0){
    datetime.dayOfWeek = (datetime.dayOfWeek % 7) + 1;
    datetime.date += 1;     // this will be corrected if needed
  }

  if(datetime.date > monthDays[datetime.month - 1]){
    // correct date if it's more than possible
    datetime.date -= monthDays[datetime.month - 1];
    datetime.month += 1;
    if(datetime.month > 12){
      datetime.month -= 1;
      datetime.years += 1;
    }
  }

  // datetime.hours = _alarmTime.hours;
  // datetime.minutes = _alarmTime.minutes;
  // datetime.seconds = _alarmTime.seconds;

  _nextTimestamp = convertToLocalTimestamp(&datetime);
}

uint64_t SoftwareAlarm::getNextTimestamp()
{
  return _nextTimestamp;
}
