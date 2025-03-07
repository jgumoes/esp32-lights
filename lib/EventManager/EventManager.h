#ifndef __ALARM_MANAGER_H__
#define __ALARM_MANAGER_H__

#include <Arduino.h>
#include <map>
#include <cmath>

// NOTE: projectDefines in ConfigManager contains some defines used in this file
#include "ConfigManager.h"

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
  uint32_t eventWindow; // how long after the time should the event trigger? should equal "timeout" in the Mode Data Struct
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
  std::shared_ptr<ConfigManagerClass> _configs;

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

  /**
   * @brief returns the correct event window. if eventWindow == 0, returns the default
   * 
   * @param eventWindow event window to check
   * @return uint32_t 
   */
  uint32_t _checkEventWindow(uint32_t eventWindow){
    if(eventWindow == 0){
      return _configs->getEventManagerConfigs().defaultEventWindow;
    }
    return eventWindow;
  }

  // TODO: load default event window from configs
  // uint32_t _defaultEventWindow = 60*60; // system default event window

  /**
   * @brief calls the given event and finds the new trigger time for that event
   * 
   * @param timestampS 
   */
  void _callEvent(uint64_t timestampS, eventUUID eventID);

  /**
 * @brief adds a new event to the Event Manager, but will not store it.
 * performs some checks, but will not check that two events share a trigger time (simultaneous
 * events should still be triggered consecutively, but the order won't be guaranteed)
 * 
 * @param timestampS current timestamp in seconds
 * @param newEvent 
 * @return eventError_t 
 */
  eventError_t _addEvent(uint64_t timestampS, TimeEventDataStruct newEvent);

  /**
   * @brief loops through the stored modes to find the initial triggers.
   * i.e. if multiple active events overlap, it returns the latest active eventID and sets the trigger times for the missed events to the next trigger time
   * 
   * @param timestampS 
   * @return eventUUID 
   */
  eventUUID _findInitialTriggers(uint64_t timestampS);
public:
  EventManager(
    std::shared_ptr<ModalLightsInterface> modalLights,
    std::shared_ptr<ConfigManagerClass> configs,
    uint64_t timestampS,
    std::vector<TimeEventDataStruct> eventStructs);
  ~EventManager(){};

  /**
   * @brief Get the time of the next event
   * 
   * @return uint64_t timestamp in seconds
   */
  uint64_t getNextEventTime(){ return _nextEventTime;};

  eventUUID getNextEventID(){ return _nextEventID;};
  
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
   * @brief rechecks event times given the current timestamp. should be used after hardware changes
   * i.e. configs change, or massive adjustment to local time
   * 
   * @param timestampS 
   */
  void rebuildTriggerTimes(uint64_t timestampS);

  /**
   * @brief checks to see if nextEvent should be called, and finds the new nextEvent
   * 
   * @param timestampS current local time in seconds
   */
  void check(uint64_t timestampS);
};

#endif