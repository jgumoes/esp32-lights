#include "DeviceTime.h"

uint64_t DeviceTimeClass::getUTCTimestampMicros()
{
  uint64_t utcTime_uS = _onboardTimestamp->getTimestamp_uS();
  if(utcTime_uS <= BUILD_TIMESTAMP){
    _timeFault = true;
    utcTime_uS = BUILD_TIMESTAMP;
    _onboardTimestamp->setTimestamp_uS(BUILD_TIMESTAMP);
  }
  else if(utcTime_uS >= _timeOfNextSync_uS){
    _timeFault = true;  // flags the need for an external sync
  }
  return utcTime_uS;
};

bool DeviceTimeClass::setUTCTimestamp2000(uint64_t newTimestamp, int32_t timezone, uint16_t DST)
{
  const uint64_t newUTCTimestamp_uS = newTimestamp*secondsToMicros;
  if(
    newTimestamp * secondsToMicros < BUILD_TIMESTAMP
    || newTimestamp >= ONBOARD_TIMESTAMP_OVERFLOW
    || newUTCTimestamp_uS >= ONBOARD_TIMESTAMP_OVERFLOW
    || !isTimezoneValid(timezone)
    || !isDSTValid(DST)
  ){
    return false;
  }
  _timeFault = false;
  const uint64_t oldUTCTimestamp_uS = _onboardTimestamp->getTimestamp_uS();
  _onboardTimestamp->setTimestamp_S(newTimestamp);

  int64_t oldOffset = _offset;
  if(
    _configs.DST != DST
    || _configs.timezone != timezone
  ){
    _configs.DST = DST;
    _configs.timezone = timezone;
    _setOffset();
    _configManager->setRTCConfigs(_configs);
  }
  
  _timeofLastSync_uS = newUTCTimestamp_uS;
  _timeOfNextSync_uS = newUTCTimestamp_uS + _configs.maxSecondsBetweenSyncs*secondsToMicros;

  const int64_t utcTimeChange_uS = newUTCTimestamp_uS - oldUTCTimestamp_uS;
  const TimeUpdateStruct timeUpdates{
    .utcTimeChange_uS = utcTimeChange_uS,
    .localTimeChange_uS = utcTimeChange_uS + _offset - oldOffset,
    .currentLocalTime_uS = newUTCTimestamp_uS + _offset
  };
  notify_observers(timeUpdates);
  return true;
}

uint64_t DeviceTimeClass::getLocalTimestampSeconds()
{
  return roundMicrosToSeconds(getLocalTimestampMicros());
}

uint64_t DeviceTimeClass::getUTCTimestampSeconds()
{
  return roundMicrosToSeconds(getUTCTimestampMicros());
}

uint64_t DeviceTimeClass::getLocalTimestampMillis()
{
  return roundMicrosToMillis(getLocalTimestampMicros());
}

uint64_t DeviceTimeClass::getLocalTimestampMicros()
{
  return convertUTCToLocalMicros(getUTCTimestampMicros());
}

uint8_t DeviceTimeClass::getDay()
{
  UsefulTimeStruct uts(getLocalTimestampSeconds());
  return uts.dayOfWeek;
}

uint8_t DeviceTimeClass::getMonth()
{
  DateTimeStruct dt;
  convertFromLocalTimestamp(getLocalTimestampSeconds(), &dt);
  return dt.month;
}

uint8_t DeviceTimeClass::getYear()
{
  uint64_t time = getLocalTimestampSeconds();
  return ((4* (time/secondsInDay)) / (4*365.25));
}

uint8_t DeviceTimeClass::getDate()
{
  DateTimeStruct dt;
  convertFromLocalTimestamp(getLocalTimestampSeconds(), &dt);
  return dt.date;
}

uint64_t DeviceTimeClass::getStartOfDay()
{
  UsefulTimeStruct uts(getLocalTimestampSeconds());
  return uts.startOfDay * secondsToMicros;
}

uint64_t DeviceTimeClass::getTimeInDay()
{
  uint64_t time_uS = getLocalTimestampMicros();
  UsefulTimeStruct uts(round(time_uS/secondsToMicros));
  uint64_t timeInDay = time_uS - (uts.startOfDay * secondsToMicros);
  return timeInDay;
}

bool DeviceTimeClass::setUTCTimestamp1970(uint64_t newTimestamp, int32_t timezone, uint16_t DST)
{
  return setUTCTimestamp2000(newTimestamp - SECONDS_BETWEEN_1970_2000, timezone, DST);
}

bool DeviceTimeClass::setLocalTimestamp2000(uint64_t newTimestamp, int32_t timezone, uint16_t DST)
{
  uint64_t utc = newTimestamp - timezone - DST;
  return setUTCTimestamp2000(utc, timezone, DST);
}

bool DeviceTimeClass::setLocalTimestamp1970(uint64_t newTimestamp, int32_t timezone, uint16_t DST)
{
  uint64_t localTimestamp = newTimestamp - SECONDS_BETWEEN_1970_2000;
  return setLocalTimestamp2000(newTimestamp - SECONDS_BETWEEN_1970_2000, timezone, DST);
}

bool DeviceTimeClass::hasTimeFault()
{
  return _timeFault;
}

uint64_t DeviceTimeClass::convertUTCToLocalMicros(uint64_t utcTimestamp_uS)
{
  // TODO: check DST start and end bounds
  if(abs(_offset) > utcTimestamp_uS){
    return 0;
  }
  return utcTimestamp_uS + _offset;
}

uint64_t DeviceTimeClass::convertLocalToUTCMicros(uint64_t localTimestamp_uS)
{
  // TODO: check DST start and end bounds
  if(abs(_offset) > localTimestamp_uS){
    return 0;
  }
  return localTimestamp_uS - _offset;
}