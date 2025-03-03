#ifndef __TESTEVENTARRAY_H__
#define __TESTEVENTARRAY_H__

#include <vector>

#include "../lib/EventManager/EventManager.h"

const uint32_t oneHour = 60*60;

struct TimeEventDataStruct testEvent1 = {1, 2 /*wakeup alarm*/, 24300 /*6:45*/, 0b00011111 /*mon-fri*/, oneHour, true};
struct TimeEventDataStruct testEvent2 = {2, 2 /*wakeup alarm*/, 36000 /*10am*/, 0b01100000 /*weekend*/, oneHour, true};
struct TimeEventDataStruct testEvent3 = {3, 3 /*nighttime mode*/, 75600 /*9pm*/, 0b01111111 /*everyday*/, oneHour, false /*background mode*/};
struct TimeEventDataStruct testEvent4 = {4, 4 /*sunset alarm/mode*/, 82800 /*11pm*/, 0b01111111 /*everyday*/, oneHour, false /*any alarms during this time should cancel into this mode*/};
struct TimeEventDataStruct testEvent5 = {5, 1 /*constant brightness*/, 32400 /*9am*/, 0b01111111 /*everyday*/, oneHour*8, false};

#endif