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
  uint64_t nextTriggerTime;
  modeUUID modeID;
  uint32_t timeOfDay;
  uint8_t daysOfWeek; // lsb = Monday, msb-1 = Sunday, msb is reserved, i.e. 0b01100000 = saturday and sunday
  uint32_t eventWindow; // TODO: delete me! how long after the time should the event trigger? should equal "timeout" in the Mode Data Struct
  bool isActive;
};

typedef int8_t eventError_t;

namespace EventManagerErrors {
  constexpr eventError_t bad_time = -2;
  constexpr eventError_t bad_uuid = -1;
  constexpr eventError_t error = 0;
  constexpr eventError_t success = 1;
};

class EventManager
{
private:
  std::shared_ptr<ModalLightsInterface> _modalLights;

  std::map<eventUUID, StoredEventStruct> _events; // map of stored EventObjects. keys are UUIDs
  uint64_t _nextEventID = 0; // UUID of the next event to trigger
  uint64_t _nextEventTime = ~0; // time of next event, in seconds

  uint64_t _previousBackgroundEventTime = 0;  // addEvent uses this to check if a new brackground event should be triggered

  /**
 * @brief finds the next trigger time for a given event.
 * 
 * @param timestampS current timestamp in microseconds
 * @param event 
 * @return uint64_t event time in seconds
 */
  uint64_t _findNextTriggerTime(uint64_t timestampS, eventUUID eventID);

  void _findNextEvent();

  // TODO: load default event window from configs
  uint32_t _defaultEventWindow = 60*60; // system default event window

  /**
   * @brief calls the given event and finds the new trigger time for that event
   * 
   * @param timestampS 
   */
  void _callEvent(uint64_t timestampS, eventUUID eventID);

  eventError_t _addEvent(uint64_t timestampS, TimeEventDataStruct newEvent);
public:
  EventManager(
    std::shared_ptr<ModalLightsInterface> modalLights,
    uint64_t timestampS,
    std::vector<TimeEventDataStruct> eventStructs);
  ~EventManager(){};

  /**
   * @brief Get the time of the next event
   * 
   * @return uint64_t timestamp in seconds
   */
  uint64_t getNextEventTime(){ return _nextEventTime;};
  
  /**
   * @brief adds a new event, and checks if it should trigger. the mode should already
   * be registered with ModalController
   * 
   * @param timestampS 
   * @param newEvent 
   * @return eventError_t 
   */
  eventError_t addEvent(uint64_t timestampS, TimeEventDataStruct newEvent);

  /**
   * @brief checks to see if nextEvent should be called, and finds the new nextEvent
   * 
   * @param timestampS current local time in seconds
   */
  void check(uint64_t timestampS);
};

#endif