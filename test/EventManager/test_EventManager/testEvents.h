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

const struct EventDataPacket testEvent8 = {8, 7 /*wakeup alarm*/, 36000 /*10am*/, 0b01111111 /*everyday*/, oneHour, true};

const struct EventDataPacket testEvent9 = {9, 8 /*chirp*/, timeToSeconds(7, 0, 0) /*7am*/, 0b01111111 /*mon-fri*/, oneHour, true};

const struct EventDataPacket testEvent10 = {10, 8, timeToSeconds(22, 0, 0), 0b01111111, oneHour, true};

const struct EventDataPacket testEvent11 = {11, 10, timeToSeconds(8, 30, 0), 0b00011111 /*weekday*/, oneHour, true};

const struct EventDataPacket testEvent12 = {8, 7 /*wakeup alarm*/, 36000 /*10am*/, 0b01111111 /*everyday*/, oneHour, false};

std::vector<EventDataPacket> getAllTestEvents() {
  std::vector<EventDataPacket> allEvents;
  EventDataPacket event1 = testEvent1;
  allEvents.push_back(event1);
  EventDataPacket event2 = testEvent2;
  allEvents.push_back(event2);
  EventDataPacket event3 = testEvent3;
  allEvents.push_back(event3);
  EventDataPacket event4 = testEvent4;
  allEvents.push_back(event4);
  EventDataPacket event5 = testEvent5;
  allEvents.push_back(event5);
  EventDataPacket event6 = testEvent6;
  allEvents.push_back(event6);
  EventDataPacket event7 = testEvent7;
  allEvents.push_back(event7);
  EventDataPacket event8 = testEvent8;
  allEvents.push_back(event8);
  EventDataPacket event9 = testEvent9;
  allEvents.push_back(event9);
  EventDataPacket event10 = testEvent10;
  allEvents.push_back(event10);
  EventDataPacket event11 = event11;
  allEvents.push_back(event11);
  return allEvents;
};

#endif