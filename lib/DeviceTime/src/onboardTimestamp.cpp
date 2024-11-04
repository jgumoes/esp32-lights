/**
 * stores the local timestamp in an on-board timer
 * 
 */

#include "onboardTimestamp.h"

void OnboardTimestamp::setTimestamp_S(uint64_t timeNow){
  setTimestamp_uS(timeNow * 1000000);
};

#ifdef ESP32S3
#include "driver/timer.h"

OnboardTimestamp::OnboardTimestamp(){
  // TODO: this is completely untested
  const timer_config_t config = {
    .alarm_en = TIMER_ALARM_DIS,
    .counter_dir = TIMER_COUNT_UP,
    .auto_reload = TIMER_AUTORELOAD_DIS,
    .divider = 80
  };
  timer_init(TIMESTAMP_TIMER_GROUP, TIMESTAMP_TIMER_NUM, &config);
  timer_set_counter_value(TIMESTAMP_TIMER_GROUP, TIMESTAMP_TIMER_NUM, 0);
  timer_start(TIMESTAMP_TIMER_GROUP, TIMESTAMP_TIMER_NUM);
}

void OnboardTimestamp::setTimestamp_uS(uint64_t timeNow){
  timer_set_counter_value(TIMESTAMP_TIMER_GROUP, TIMESTAMP_TIMER_NUM, timeNow);
}

uint64_t OnboardTimestamp::getTimestamp_uS(){
  uint64_t time = 0;
  timer_get_counter_value(TIMESTAMP_TIMER_GROUP, TIMESTAMP_TIMER_NUM, &time);
  return time;
}
#endif

// TODO: add more on-board timers as needed

#ifdef native_env

uint64_t OnboardTimestamp::_localTestingTimestamp = 0;

OnboardTimestamp::OnboardTimestamp(){
  OnboardTimestamp::_localTestingTimestamp = 0;
}

void OnboardTimestamp::setTimestamp_uS(uint64_t timeNow){
  OnboardTimestamp::_localTestingTimestamp = timeNow;
}

uint64_t OnboardTimestamp::getTimestamp_uS(){
  return OnboardTimestamp::_localTestingTimestamp;
}


#endif

