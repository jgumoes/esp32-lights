/**
 * yes i'm using defines to switch in mocks, but the implementation is platform-specific so i'm not going to change it
 * 
 */

#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#ifndef native_env
  #include <Arduino.h>
#else
  #include <ArduinoFake.h>
#endif

#define TIMESTAMP_TIMER_GROUP TIMER_GROUP_0
#define TIMESTAMP_TIMER_NUM TIMER_0

class OnboardTimestamp{
  public:
#ifdef native_env
  static uint64_t _localTestingTimestamp;
#endif
  /**
   * @brief sets up the on board timers.
   * 
   */
  OnboardTimestamp();

  /**
   * @brief initiate a General Purpose Timer to act as an on-board RTC.
   * It measures in microseconds because it uses the 80MHz clock,
   * but won't overflow until 2570.
   * 
   * @param timeNow the current timestamp in microseconds
   */
  void setTimestamp_uS(uint64_t startTime);

  /**
   * @brief initiate a General Purpose Timer to act as an on-board RTC.
   * It measures in microseconds because it uses the 80MHz clock,
   * but won't overflow until 2570.
   * 
   * @param timeNow the current timestamp in microseconds
   */
  void setTimestamp_S(uint64_t timeNow);

  /**
   * @brief Get the timestamp. accepts the address of an empty value,
   * and fills it with the timestamp in microseconds
   * 
   * @param time address of the time value to fill
   */
  uint64_t getTimestamp_uS();
};

#endif