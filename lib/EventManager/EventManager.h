#ifndef __ALARM_MANAGER_H__
#define __ALARM_MANAGER_H__

#include <Arduino.h>
#include <map>
#include <cmath>

#include "ProjectDefines.h"
#include "ConfigManager.h"

#include "ModalLights.h"
#include "DeviceTime.h"

// the struct that EventManager uses to map the events
struct EventMappingStruct {
  uint64_t nextTriggerTime = 0;
  modeUUID modeID = 0;
  uint32_t timeOfDay = 0;
  uint8_t daysOfWeek = 0; // lsb = Monday, msb-1 = Sunday, msb is reserved, i.e. 0b01100000 = saturday and sunday
  uint32_t eventWindow = 0; // how long after the time should the event trigger? should equal "timeout" in the Mode Data Struct
  bool isActive = 0;
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

  std::map<eventUUID, EventMappingStruct> _events; // map of stored EventObjects. keys are UUIDs
  uint64_t _nextEventID = 0; // UUID of the next event to trigger
  uint64_t _nextEventTime = ~0; // time of next event, in seconds

  uint64_t _previousBackgroundEventTime = 0;  // addEvent uses this to check if a new brackground event should be triggered

  /**
   * @brief checks the new event and returns the appropriate error code. skips the eventID check if `updating = true`
   * 
   * @param newEvent 
   * @param updating is newEvent actually an update of an existing event?
   * @return eventError_t 
   */
  eventError_t _isNewEventValid(EventDataPacket newEvent, bool updating = false);

  /**
 * @brief finds the next trigger time for a given event. Does not perform any side effects.
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

  /**
   * @brief calls the given event and finds the new trigger time for that event
   * 
   * @param timestampS 
   */
  void _callEvent(uint64_t timestampS, eventUUID eventID);

  /**
 * @brief adds a new event to the Event Manager. Uses time window for active events, and checks previous trigger time for background events.
 * Performs some checks, but will not check that two events share a trigger time (simultaneous
 * events should still be triggered consecutively, but the order won't be guaranteed)
 * 
 * @param timestampS current timestamp in seconds
 * @param newEvent 
 * @param updating is newEvent an update of an existing event?
 * @return eventError_t 
 */
  eventError_t _addEvent(uint64_t timestampS, EventDataPacket newEvent, bool updating = false);

  /**
   * @brief loops through the stored modes to find the background mode that should be triggered, and the next active mode to trigger. If overlapping active events should have triggered at the current time, it returns the latest active eventID and sets the trigger times for the missed events to the next trigger time
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
    std::vector<EventDataPacket> eventStructs);
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
  eventError_t addEvent(uint64_t timestampS, EventDataPacket newEvent);

  /**
   * @brief removes the given events and rebuilds the trigger times
   * 
   * @param timestampS current timestamp in seconds
   * @param eventIDs array of eventIDs to remove
   * @param number number of events to remove
   * @return eventError_t 
   */
  void removeEvents(uint64_t timestampS, eventUUID *eventIDs, nEvents_t number);

  /**
   * @brief removes a single given event and rebuilds the trigger times
   * 
   * @param timestampS current timestamp in seconds
   * @param eventID ID of the event to remove
   * @return eventError_t 
   */
  void removeEvent(uint64_t timestampS, eventUUID eventID);

  /**
   * @brief updates the given events and rebuilds the trigger times
   * 
   * @param timestampS current timestamp in seconds
   * @param events array of EventDataPackets
   * @param eventErrors array of eventError_t will get filled by this method
   * @param number number of events to update
   * @return eventError_t pass or fail. badEvents will hold the mode-specific errors
   */
  void updateEvents(uint64_t timestampS, EventDataPacket *events, eventError_t *eventErrors, nEvents_t number);
  
  /**
   * @brief updates a single event and rebuilds the trigger times
   * 
   * @param timestampS current timestamp in seconds
   * @param event data packet of the event to update
   * @return eventError_t 
   */
  eventError_t updateEvent(uint64_t timestampS, EventDataPacket event);
  
  /**
   * @brief rechecks event times given the current timestamp. should be used after hardware changes
   * i.e. configs change, or massive adjustment to local time
   * 
   * @param timestampS 
   */
  void rebuildTriggerTimes(uint64_t timestampS, bool checkMissedActive = true);

  /**
   * @brief checks to see if nextEvent should be called, and finds the new nextEvent
   * 
   * @param timestampS current local time in seconds
   */
  void check(uint64_t timestampS);
};

#endif