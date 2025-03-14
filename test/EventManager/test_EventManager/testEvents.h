#ifndef __TESTEVENTARRAY_H__
#define __TESTEVENTARRAY_H__

#include <vector>

#include "../lib/EventManager/EventManager.h"
#include "DeviceTime.h"

const uint32_t oneHour = 60*60;
const uint64_t mondayAtMidnight = 794275200;

const struct EventDataPacket testEvent1 = {1, 2 /*wakeup alarm*/, 24300 /*6:45*/, 0b00011111 /*mon-fri*/, oneHour, true};
const struct EventDataPacket testEvent2 = {2, 2 /*wakeup alarm*/, 36000 /*10am*/, 0b01100000 /*weekend*/, oneHour, true};
const struct EventDataPacket testEvent3 = {3, 4 /*nighttime mode*/, 75600 /*9pm*/, 0b01111111 /*everyday*/, oneHour, false /*background mode*/};
const struct EventDataPacket testEvent4 = {4, 3 /*sunset alarm/mode*/, 82800 /*11pm*/, 0b01111111 /*everyday*/, oneHour, false /*any alarms during this time should cancel into this mode*/};
const struct EventDataPacket testEvent5 = {5, 1 /*constant brightness*/, 32400 /*9am*/, 0b01111111 /*everyday*/, oneHour*8, false};

const struct EventDataPacket testEvent6 = {6, 5 /*wakeup alarm*/, timeToSeconds(7, 0, 0) /*7am*/, 0b00011111 /*mon-fri*/, oneHour, true};

const struct EventDataPacket testEvent7 = {7, 6 /*post-wakeup background mode*/, timeToSeconds(7, 0, 0) /*7am*/, 0b01111111 /*mon-fri*/, oneHour, false};

const struct EventDataPacket testEvent8 = {8, 7 /*wakeup alarm*/, 36000 /*10am*/, 0b01111111 /*weekend*/, oneHour, true};

const struct EventDataPacket testEvent9 = {9, 8 /*chirp*/, timeToSeconds(7, 0, 0) /*7am*/, 0b01111111 /*mon-fri*/, oneHour, true};

const struct EventDataPacket testEvent10 = {10, 8, timeToSeconds(22, 0, 0), 0b01111111, oneHour, true};

#endif