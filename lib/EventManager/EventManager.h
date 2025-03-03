#ifndef __ALARM_MANAGER_H__
#define __ALARM_MANAGER_H__

#include <Arduino.h>
#include <map>
#include <cmath>

#include "ModalLights.h"
#include "DeviceTime.h"

typedef uint8_t eventUUID;

struct TimeEventDataStruct {
  eventUUID eventID;
  modeUUID modeID;
  uint32_t timeOfDay;
  uint8_t daysOfWeek; // lsb = Monday, msb-1 = Sunday, msb is reserved, i.e. 0b01100000 = saturday and sunday
  uint32_t eventWindow; // TODO: delete me! how long after the time should the event trigger? should equal "timeout" in the Mode Data Struct
  bool isActive;
};

struct StoredEventStruct {
  uint64_t nextAlarmTime;
  modeUUID modeID;
  uint32_t timeOfDay;
  uint8_t daysOfWeek; // lsb = Monday, msb-1 = Sunday, msb is reserved, i.e. 0b01100000 = saturday and sunday
  uint32_t eventWindow; // TODO: delete me! how long after the time should the event trigger? should equal "timeout" in the Mode Data Struct
  bool isActive;
};

typedef uint8_t eventError_t;

namespace EventManagerErrors {
  constexpr eventError_t bad_time = -2;
  constexpr eventError_t bad_uuid = -1;
  constexpr eventError_t error = 0;
  constexpr eventError_t success = 1;
};

// class EventObject
// {
// private:
//   uint64_t _nextEventTime;  // time of the next event
//   TimeEventDataStruct _eventData;
// public:
// EventObject(TimeEventDataStruct eventData, uint64_t currentTime);
//   ~EventObject();

//   /**
//    * @brief finds the time of the next event, starting at the current time minus (eventWindow - 1 second)
//    * 
//    * @param currentTime 
//    * @return uint64_t 
//    */
//   uint64_t findNextEventTime(uint64_t currentTime);

//   /**
//    * @brief calls the event function. 
//    * 
//    */
//   void callEvent();
// };

class EventManager
{
private:
  std::map<eventUUID, StoredEventStruct> _events; // map of stored EventObjects. keys are UUIDs
  eventUUID _nextEvent = 0; // UUID of the next event to trigger
  uint64_t _nextEventTime = ~0; // time of next event, in seconds

  uint64_t _findNextTriggerTime(uint64_t timestamp, StoredEventStruct event);
public:
  EventManager(uint64_t timestamp, std::vector<TimeEventDataStruct> eventStructs);
  ~EventManager(){};

  /**
   * @brief Get the time of the next event
   * 
   * @return uint64_t timestamp in seconds
   */
  uint64_t getNextEventTime(){ return _nextEventTime;};
  
  eventError_t addEvent(uint64_t timestamp, TimeEventDataStruct newEvent);
};

#endif