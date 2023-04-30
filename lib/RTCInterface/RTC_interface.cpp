/*
 * A generic interface for reading from and writing to an RTC.
 * Currently only supports the DS3231, but I will add other chips
 * as I buy them ;-)
 * 
 * There is a known bug where a timestamp for 1/3/2024 (a leap year)
 * gets interpreted as 2/3/2024
 */

#include "RTC_interface.h"
#include "user_config.h"

// define which RTC chipset is being used. see @ref user_config.h
#ifdef USE_DS3231
  #define CLOCK_ADDRESS 0x68
#endif

// public

RTCInterfaceClass::RTCInterfaceClass(){
  setTo24hr();
}


uint8_t RTCInterfaceClass::bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
byte RTCInterfaceClass::decToBcd(byte val) { return ( ((val/10) << 4) + (val%10) ); }

/*
 * Fetchs the time from the RTC chipset
 */
void RTCInterfaceClass::fetchTime(){
  resetDatetime();
  transmitByte(0);

  Wire.requestFrom(CLOCK_ADDRESS, 7);
  datetime._seconds = bcd2bin(Wire.read() & 0x7F);
  datetime._minutes = bcd2bin(Wire.read());
  datetime._hours = bcd2bin(Wire.read());
  datetime._day = bcd2bin(Wire.read());
  datetime._date = bcd2bin(Wire.read());
  datetime._month = bcd2bin(Wire.read());
  datetime._years = bcd2bin(Wire.read()); // years since midnight 2000
}

/*
 * Returns the local timestamp in seconds since the 2000 epoch
 */
uint32_t RTCInterfaceClass::getLocalTimestamp(){
  fetchTime();
  datetime.readReady = true;
  // TODO: can this be improved?
  localTimestamp = (datetime._years * 365) + ((datetime._years + 3)/4);      // years to days

  uint16_t days = datetime._date - 1 + (!(datetime._years % 4) && (datetime._month > 2)); // remove a day to 0 index the days, but
                                                                                          // add a day if it's a leap year and after Feb
  for(int i = 0; i < datetime._month - 1; i++){ days += monthDays[i];}

  localTimestamp = (localTimestamp + days) * 24;      // days to hours
  localTimestamp = (localTimestamp + datetime._hours) * 60;     // hours to minutes
  localTimestamp = (localTimestamp + datetime._minutes) * 60;   // minutes to seconds
  localTimestamp += datetime._seconds;                          // add the seconds

  #ifdef DEBUG_PRINT_RTC_DATETIME
    Serial.println("RTC Interface: getLocalTimestamp converted time:");
    Serial.print("RTC seconds: "); Serial.println(datetime._seconds);
    Serial.print("RTC minutes: "); Serial.println(datetime._minutes);
    Serial.print("RTC hours: "); Serial.println(datetime._hours);
    Serial.print("RTC day: "); Serial.println(datetime._day);
    Serial.print("RTC date: "); Serial.println(datetime._date);
    Serial.print("RTC month: "); Serial.println(datetime._month);
    Serial.print("RTC years: "); Serial.println(datetime._years);
  #endif
  return localTimestamp;
}

/*
 * Returns the UTC timestamp, without timezone and DST ajustments
 */
uint32_t RTCInterfaceClass::getUTCTimestamp(){
  return getLocalTimestamp() - _timeZoneSecs - _DSTOffsetSecs;
}

/*
 * Sets the time with a UTC timestamp and offsets, and writes changes to RTC chip
 * @param time UTC timestamp in seconds
 * @param timezone offset in seconds
 * @param DST offset in seconds
 */
void RTCInterfaceClass::setUTCTimestamp(uint32_t time, int32_t timezone, uint16_t dst){
  setTimezoneOffset(timezone);
  setDSTOffset(dst);
  setUTCTimestamp(time);
  updateTime();
}

/*
 * Sets the time using a UTC timezone
 * @param time the UTC timestamp in seconds
 * @note the timezone and dst offsets must be set BEFORE calling this function
 */
void RTCInterfaceClass::setUTCTimestamp(uint32_t time){
  pendingUpdates.timestamp = time;
  pendingUpdates.timestampPending = true;
}

/*
 * @param seconds timezone offset in seconds
 */
void RTCInterfaceClass::setTimezoneOffset(int32_t seconds){
  pendingUpdates.timezone = seconds;
  pendingUpdates.timezonePending = true;
}

/*
 * @param seconds DST offset in seconds
 */
void RTCInterfaceClass::setDSTOffset(uint16_t seconds){
  pendingUpdates.DST = seconds;
  pendingUpdates.DSTPending = true;
}

/*
 * sends pending updates to the RTC and resets pending updates buffer.
 */
void RTCInterfaceClass::updateTime(){
  uint32_t timestamp = (pendingUpdates.timestampPending) ? pendingUpdates.timestamp : getUTCTimestamp();  // set a new time or update the current one
  if(pendingUpdates.DSTPending){ _DSTOffsetSecs = pendingUpdates.DST; }
  if(pendingUpdates.timezonePending){ _timeZoneSecs = pendingUpdates.timezonePending; }
  updateLocalTimestamp(timestamp + _timeZoneSecs + _DSTOffsetSecs);
  resetPendingUpdates();
}

/*
 * breaks a 2000 epoch timestamp it into datetime values
 * @param time local timestamp in seconds
 */
void RTCInterfaceClass::convertLocalTimestamp(uint32_t time){
  #ifdef DEBUG_PRINT_RTC_ENABLE
    Serial.print("RTC Interface: Converting Timestamp: "); Serial.println(time);
  #endif
  datetime.readReady = false;
  resetDatetime();
  datetime._seconds = time % 60;   // i.e. divide by number of minutes and take remainder
  time /= 60;             // time is now in minutes
  datetime._minutes = time % 60;   // i.e. divide by number of hours and take remainder
  time /= 60;             // time is now in hours
  datetime._hours = time % 24;     // i.e. divide by number of days and take remainder
  time /= 24;             // time is now in days since 1/1/2000
  datetime._day = time % 7;
  datetime._day += 7 * !datetime._day;  // if day is 0, set it to 7
  datetime._years = ((4 * time) / 1461);
  time -= (datetime._years * 365.25) - 1;   // time is now in day of the year
                                            // +1 because day of the year isn't zero-indexed

  int leapDay = (!(datetime._years % 4) && (time > 59)); // knock of a day if its a leap year and after Feb 29
  time -= leapDay;
  int i = 0;
  while (time > monthDays[i]){
      time -= monthDays[i];
      i++;
  }
  datetime._date = time + leapDay;
  datetime._month = i + 1;

  #ifdef DEBUG_PRINT_RTC_DATETIME
    Serial.println("RTC Interface converted time:");
    Serial.print("RTC seconds: "); Serial.println(datetime._seconds);
    Serial.print("RTC minutes: "); Serial.println(datetime._minutes);
    Serial.print("RTC hours: "); Serial.println(datetime._hours);
    Serial.print("RTC day: "); Serial.println(datetime._day);
    Serial.print("RTC date: "); Serial.println(datetime._date);
    Serial.print("RTC month: "); Serial.println(datetime._month);
    Serial.print("RTC years: "); Serial.println(datetime._years);
  #endif
}

/*
 * resets all stored values before performing read/write operations
 */
void RTCInterfaceClass::resetDatetime(){
  datetime._date = 0;
  datetime._day = 0;
  datetime._hours = 0;
  datetime._minutes = 0;
  datetime._month = 0;
  datetime._seconds = 0;
  datetime._years = 0;
}

// Private

/*
 * resets pending updates after updates have been made
 */
void RTCInterfaceClass::resetPendingUpdates(){
  pendingUpdates.timezonePending = 0;
  pendingUpdates.timezone = 0;
  pendingUpdates.DSTPending = 0;
  pendingUpdates.DST = 0;
  pendingUpdates.timestampPending = 0;
  pendingUpdates.timestamp = 0;
}

/*
 * Writes a local timestamp to the RTC chip.
 * @param time local timestamp in seconds
 */
void RTCInterfaceClass::updateLocalTimestamp(uint32_t time){
  convertLocalTimestamp(time);
  transmit2Bytes(0x00, decToBcd(datetime._seconds));
  transmit2Bytes(0x01, decToBcd(datetime._minutes));
  transmit2Bytes(0x02, decToBcd(datetime._hours));
  transmit2Bytes(0x03, datetime._day);
  transmit2Bytes(0x04, decToBcd(datetime._date));
  transmit2Bytes(0x05, decToBcd(datetime._month));
  transmit2Bytes(0x06, decToBcd(datetime._years));

  #ifdef DEBUG_PRINT_RTC_DATETIME
    Serial.println("RTC Interface written time:");
    Serial.print("RTC seconds: "); Serial.println(datetime._seconds);
    Serial.print("RTC minutes: "); Serial.println(datetime._minutes);
    Serial.print("RTC hours: "); Serial.println(datetime._hours);
    Serial.print("RTC day: "); Serial.println(datetime._day);
    Serial.print("RTC date: "); Serial.println(datetime._date);
    Serial.print("RTC month: "); Serial.println(datetime._month);
    Serial.print("RTC years: "); Serial.println(datetime._years);
  #endif
}

// sets the rtc to use 24 hour time
// NOTE: as it sets the rtc to the same time it just read,
// there's a non-zero chance the hour could change and set
// the time back by up to a minute
void RTCInterfaceClass::setTo24hr(){
  byte temp_buffer;
  transmitByte(0x02);
  Wire.requestFrom(CLOCK_ADDRESS, 1);
  temp_buffer = Wire.read();

  if(temp_buffer & 0b01000000){
    // if set to 12 hour time
    temp_buffer = temp_buffer & 0b10111111;
    transmit2Bytes(0x02, temp_buffer);
  }

}

// transmits a single byte, AKA the address to be read from
// note: device address is set by CLOCK_ADDRESS
void RTCInterfaceClass::transmitByte(byte address){
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();
}

// transmits 2 byte packets, AKA the address and th value
// note: device address is set by CLOCK_ADDRESS
void RTCInterfaceClass::transmit2Bytes(byte address, byte value){
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(address);
  Wire.write(value);
  Wire.endTransmission();
}

RTCInterfaceClass RTCInterface = RTCInterfaceClass();