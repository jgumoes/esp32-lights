
#ifndef RTC_interface_h
#define RTC_interface_h

#include <Arduino.h>
#include <Wire.h>


struct DateTimeStruct{
  uint16_t _seconds;
  uint16_t _minutes;
  uint16_t _hours;
  uint16_t _day;      // day of the week
  uint16_t _date;
  uint16_t _month;
  uint16_t _years;
  bool readReady; // if true, struct is suitable for reading
};

struct PendingUpdatesStruct{
  bool timezonePending = false;
  int32_t timezone = 0;
  bool DSTPending = false;
  uint16_t DST = 0;
  bool timestampPending = false;
  uint32_t timestamp = 0; // incoming timestamps will always be UTC as per BLE DTS specification
};

/*
 * Handles interfacing with the RTC.
 * Gets and sets the current time, sets the alarm, and sets RTC settings.
 *
 * Changing the time:
 * the user must use setX(y) to change the intended values,
 * then call commitUpdates() to write the changes to the RTC chip
 */
class RTCInterfaceClass{
  public:
    RTCInterfaceClass();

    DateTimeStruct datetime;
    const uint8_t monthDays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

    uint32_t getLocalTimestamp();
    uint32_t getUTCTimestamp();

    void setTimezoneOffset(int32_t seconds);
    int32_t getTimezoneOffset();
    void setDSTOffset(uint16_t seconds);
    uint16_t getDSTOffset();
    bool setUTCTimestamp(uint32_t time, int32_t timezone, uint16_t dst);
    bool setUTCTimestamp(uint32_t time);

    bool commitUpdates();

    uint8_t bcd2bin (uint8_t val);
    byte decToBcd(byte val);
    void convertLocalTimestamp(uint32_t time);
    void resetDatetime();

  private:
    int32_t _timeZoneSecs = 0;    // timezone offset in seconds
    uint16_t _DSTOffsetSecs = 0;  // daylight savings offset in seconds

    void fetchTime();
    uint32_t localTimestamp;
    PendingUpdatesStruct pendingUpdates;
    void resetPendingUpdates();

    bool updateLocalTimestamp(uint32_t time);

    void setTo24hr();
    bool transmitByte(byte address);
    bool transmit2Bytes(byte address, byte value);
};

extern RTCInterfaceClass RTCInterface;
#endif
