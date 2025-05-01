#include "DeviceTime.h"

uint64_t DeviceTimeClass::getUTCTimestampMicros()
{
  uint64_t utcTime_uS = _onboardTimestamp->getTimestamp_uS();
  if(utcTime_uS <= BUILD_TIMESTAMP){
    _timeFault = true;
    utcTime_uS = BUILD_TIMESTAMP;
    _onboardTimestamp->setTimestamp_uS(BUILD_TIMESTAMP);
  }
  else if(utcTime_uS >= _timeOfNextSync){
    _timeFault = true;  // flags the need for an external sync
  }
  return utcTime_uS;
};

bool DeviceTimeClass::setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST)
{
  if(
    newTimesamp * 1000000 < BUILD_TIMESTAMP
    || newTimesamp >= ONBOARD_TIMESTAMP_OVERFLOW
    || !isTimezoneValid(timezone)
    || !isDSTValid(DST)
  ){
    return false;
  }
  _timeFault = false;
  _onboardTimestamp->setTimestamp_S(newTimesamp);

  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  if(
    configs.DST != DST
    || configs.timezone != timezone
  ){
    configs.DST = DST;
    configs.timezone = timezone;
    _configManager->setRTCConfigs(configs);
  }
  
  _timeofLastSync = newTimesamp*1000000;
  _timeOfNextSync = _timeofLastSync + _configManager->getRTCConfigs().maxSecondsBetweenSyncs*1000000;
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
  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  UsefulTimeStruct uts = makeUsefulTimeStruct(getLocalTimestampSeconds());
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
  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  UsefulTimeStruct uts = makeUsefulTimeStruct(getLocalTimestampSeconds());
  return uts.startOfDay * secondsToMicros;
}

uint64_t DeviceTimeClass::getTimeInDay()
{
  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  uint64_t time_uS = getLocalTimestampMicros();
  UsefulTimeStruct uts = makeUsefulTimeStruct(round(time_uS/secondsToMicros));
  uint64_t timeInDay = time_uS - (uts.startOfDay * secondsToMicros);
  return timeInDay;
}

bool DeviceTimeClass::setUTCTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST)
{
  return setUTCTimestamp2000(newTimesamp - SECONDS_BETWEEN_1970_2000, timezone, DST);
}

bool DeviceTimeClass::setLocalTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST)
{
  uint64_t utc = newTimesamp - timezone - DST;
  return setUTCTimestamp2000(utc, timezone, DST);
}

bool DeviceTimeClass::setLocalTimestamp1970(uint64_t newTimesamp, int32_t timezone, uint16_t DST)
{
  uint64_t localTimestamp = newTimesamp - SECONDS_BETWEEN_1970_2000;
  return setLocalTimestamp2000(newTimesamp - SECONDS_BETWEEN_1970_2000, timezone, DST);
}

bool DeviceTimeClass::hasTimeFault()
{
  return _timeFault;
}

uint64_t DeviceTimeClass::convertUTCToLocalMicros(uint64_t utcTimestamp_uS)
{
  // TODO: check DST start and end bounds
  RTCConfigsStruct configs = _configManager->getRTCConfigs();
  int64_t offset = (configs.DST + configs.timezone) * secondsToMicros;
  if(abs(offset) > utcTimestamp_uS){
    return 0;
  }
  return utcTimestamp_uS + offset;
}

uint64_t DeviceTimeClass::convertLocalToUTCMicros(uint64_t localTimestamp_uS)
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