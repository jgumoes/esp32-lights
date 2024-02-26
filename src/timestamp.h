#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include <stdint.h>

#define TIMESTAMP_TIMER_GROUP TIMER_GROUP_0
#define TIMESTAMP_TIMER_NUM TIMER_0

void setTimestamp_uS(uint64_t startTime);
void setTimestamp(uint64_t timeNow);
void getTimestamp_uS(uint64_t& time);

#endif