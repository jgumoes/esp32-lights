#include "driver/timer.h"

#include "timestamp.h"

/**
 * @brief initiate a General Purpose Timer to act as an on-board RTC.
 * It measures in microseconds because it uses the 80MHz clock,
 * but won't overflow until 2570.
 * 
 * @param timeNow the current timestamp in microseconds
 */
void setTimestamp_uS(uint64_t timeNow){
  const timer_config_t config = {
    .alarm_en = TIMER_ALARM_DIS,
    .counter_dir = TIMER_COUNT_UP,
    .auto_reload = TIMER_AUTORELOAD_DIS,
    .divider = 80
  };
  timer_init(TIMESTAMP_TIMER_GROUP, TIMESTAMP_TIMER_NUM, &config);
  // uint64_t timeNow = RTC.getLocalTimestamp() * 1000000;
  timer_set_counter_value(TIMESTAMP_TIMER_GROUP, TIMESTAMP_TIMER_NUM, timeNow);
  timer_start(TIMESTAMP_TIMER_GROUP, TIMESTAMP_TIMER_NUM);
}

/**
 * @brief initiate a General Purpose Timer to act as an on-board RTC.
 * It measures in microseconds because it uses the 80MHz clock,
 * but won't overflow until 2570.
 * 
 * @param timeNow the current timestamp in microseconds
 */
void setTimestamp(uint64_t timeNow){
  setTimestamp_uS(timeNow * 1000000);
};

/**
 * @brief Get the timestamp. accepts the address of an empty value,
 * and fills it with the timestamp in microseconds
 * 
 * @param time address of the time value to fill
 */
void getTimestamp_uS(uint64_t& time){
  timer_get_counter_value(TIMESTAMP_TIMER_GROUP, TIMESTAMP_TIMER_NUM, &time);
}

// double time2;
// timer_get_counter_time_sec(TIMESTAMP_TIMER_GROUP, TIMESTAMP_TIMER_NUM, &time2);

