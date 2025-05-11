#ifndef __EVENT_SUPERVISOR_H__
#define __EVENT_SUPERVISOR_H__

#include <Arduino.h>
#include <map>
// #include <etl/flat_map.h>
#include "ProjectDefines.h"
#include <timeHelpers.h>

#define maxTimeOfDay 86399
#define daysOfWeekMask 0b01111111

// the struct that EventManager uses to map the events
struct EventMappingStruct {
  uint64_t nextTriggerTime = 0;
  modeUUID modeID = 0;
  uint32_t timeOfDay = 0;
  uint8_t daysOfWeek = 0; // lsb = Monday, msb-1 = Sunday, msb is reserved, i.e. 0b01100000 = saturday and sunday
  uint32_t eventWindow = 0; // how long after the time should the event trigger? should equal "timeout" in the Mode Data Struct
  bool isActive = 0;

  EventMappingStruct(const EventDataPacket& dataPacket) :
    modeID(dataPacket.modeID),
    timeOfDay(dataPacket.timeOfDay),
    daysOfWeek(dataPacket.daysOfWeek),
    eventWindow(dataPacket.eventWindow),
    isActive(dataPacket.isActive){};
};

typedef std::map<eventUUID, EventMappingStruct> EventMap_t;
// typedef etl::map<eventUUID, EventMappingStruct> EventMap_t;

typedef int8_t eventError_t;

namespace EventManagerErrors {
  constexpr eventError_t storage_full = -4;
  constexpr eventError_t event_not_found = -3;
  constexpr eventError_t bad_time = -2;
  constexpr eventError_t bad_uuid = -1;
  constexpr eventError_t error = 0;
  constexpr eventError_t success = 1;
};

struct NextEventStruct{
  eventUUID ID = 0;
  EventMappingStruct* data = nullptr;
  uint64_t getNextTriggerTime(){
    return data != nullptr ? data->nextTriggerTime : ~0;
  };
};

struct TriggeringModeStruct{
  eventUUID ID = 0;
  uint64_t triggerTime = 0;
};

struct EventTimeStruct{
  eventUUID ID = 0;
  uint64_t triggerTime = 0;
};

// for development only. should be tested to make sure this is removed
#define UseVirtualEventContainer true
#ifdef UseVirtualEventContainer
/*
Event Container Definitions
*/
class IEventSupervisor
{
public:
  IEventSupervisor(){};

  virtual TriggeringModeStruct check(uint64_t timestamp_S) = 0;

  virtual eventError_t addEvent(uint64_t timestamp_S, const EventDataPacket &newEventPacket) = 0;
  virtual void rebuildTriggerTimes(uint64_t timestamp_S) = 0;
  virtual eventError_t updateEvent(uint64_t timestamp_S, const EventDataPacket &event) = 0;
  virtual eventError_t removeEvent(uint64_t timestamp_S, eventUUID eventID) = 0;

  virtual EventTimeStruct getNextEvent() = 0;

  virtual size_t getNumberOfEvents() = 0;

  virtual bool doesEventExist(eventUUID eventID) = 0;

  virtual void updateTime(const uint64_t newTimestamp_S) = 0;
};
#endif

#ifndef UseVirtualEventContainer
class BackgroundEventSupervisor
#else
class BackgroundEventSupervisor : public IEventSupervisor
#endif
{
private:
  const EventManagerConfigsStruct& _configs;

  EventMap_t _events;

  NextEventStruct _nextEvent;

  EventTimeStruct _previousEvent;

  uint64_t _findPreviousTriggerTime(const EventMappingStruct& event, const UsefulTimeStruct &timeStruct);

  /**
   * @brief helper method for building/rebuilding trigger times. checks if an event should replace the current _nextEvent. if so, finds the next trigger time for the current nextEvent before reassigning. if not, finds the next trigger time for the checking event. assumes the nextTriggerTime is set to the previous trigger time
   * 
   * @param eventID 
   * @param eventData 
   * @param uts 
   */
  void _shouldEventBeNext(eventUUID eventID, EventMappingStruct* eventData, const UsefulTimeStruct& uts);

  /**
   * @brief rebuilds the trigger times without resetting previousTrigger
   * 
   * @param timestamp_S 
   */
  void _rebuildTriggerTimes(uint64_t timestamp_S);
public:
  BackgroundEventSupervisor(const EventManagerConfigsStruct& configs) : _configs(configs){};

  /**
   * @brief returns the mode that should be triggered now. if none should trigger, returns an empty TriggeringModeStruct.
   * 
   * @param timestamp_S 
   * @return TriggeringModeStruct modeID and trigger time of the mode
   */
  TriggeringModeStruct check(uint64_t timestamp_S);

  /**
   * @brief add an event to the map, and determine if it should trigger next. packet and size validations must have already happened, but it will check if the ID doesn't already exist
   * 
   * @param timestamp_S 
   * @param newEventPacket 
   * @return eventError_t 
   */
  eventError_t addEvent(uint64_t timestamp_S, const EventDataPacket &newEventPacket);

  /**
   * @brief rebuilds the trigger times and sets the next event. resets previousTriggerTime which can lead to re-triggering a mode, but how this gets handled is dependant on the mode, so it's really a ModalController problem
   * 
   * @param timestamp_S 
   */
  void rebuildTriggerTimes(uint64_t timestamp_S){
    _previousEvent.ID = 0;
    _previousEvent.triggerTime = 0;
    _rebuildTriggerTimes(timestamp_S);
  };
  
  /**
   * @brief updates a new event, and checks if it should trigger. this can lead to re-triggering a mode, but how this gets handled is dependant on the mode, so it's really a ModalController problem
   * 
   * @param timestamp_S 
   * @param event 
   * @return eventError_t 
   */
  eventError_t updateEvent(uint64_t timestamp_S, const EventDataPacket &event);

  /**
   * @brief removes the event, and rebuilds the trigger times.
   * 
   * @param timestamp_S 
   * @param eventID 
   * @return eventError_t 
   */
  eventError_t removeEvent(uint64_t timestamp_S, eventUUID eventID);

  EventTimeStruct getNextEvent(){
    if(_events.size() == 0){
      return EventTimeStruct{0, 0};
    }
    return EventTimeStruct{_nextEvent.ID,_nextEvent.getNextTriggerTime()};
  }

  size_t getNumberOfEvents(){return _events.size();};

  bool doesEventExist(eventUUID eventID){return _events.count(eventID) > 0;}

  /**
   * @brief performs the time update without performing any checks. rebuilds trigger times
   * 
   * @param timeUpdates 
   */
  void updateTime(const uint64_t newTimestamp_S){
    // _previousEvent.triggerTime += roundingDivide(timeUpdates.localTimeChange_uS, secondsToMicros);
    rebuildTriggerTimes(newTimestamp_S);
  };
};

#ifndef UseVirtualEventContainer
class ActiveEventSupervisor
#else
class ActiveEventSupervisor : public IEventSupervisor
#endif
{
private:
  const EventManagerConfigsStruct& _configs;

  EventMap_t _events;

  NextEventStruct _nextEvent;

  EventTimeStruct _previousEvent;

  /**
   * @brief helper method for building/rebuilding trigger times. checks if an event should replace the current _nextEvent. if so, finds the next trigger time for the current nextEvent before reassigning. if not, finds the next trigger time for the checking event. assumes the nextTriggerTime is set using eventWindow
   * 
   * @param eventID 
   * @param eventData 
   * @param timestamp_s 
   */
  void _shouldEventBeNext(eventUUID eventID, EventMappingStruct* eventData, const UsefulTimeStruct &uts);

  /**
   * @brief returns the correct event window. if eventWindow == 0, returns the default
   * 
   * @param eventWindow event window to check
   * @return uint32_t 
   */
  uint32_t _checkEventWindow(uint32_t eventWindow){
    if(eventWindow == 0){
      return _configs.defaultEventWindow_S;
    }
    return eventWindow;
  }

  /**
   * @brief finds the correct event window, then calls findNextTriggerTime() starting from the window's start time
   * 
   * @param timestamp_S 
   * @param event 
   * @return uint64_t 
   */
  uint64_t _findNextTriggerWithWindow(const uint64_t timestamp_S, const EventMappingStruct* event);

public:
  ActiveEventSupervisor(const EventManagerConfigsStruct& configs) : _configs(configs){};
  
  /**
   * @brief returns the mode that should be triggered now. if none should trigger, returns an empty TriggeringModeStruct.
   * 
   * @param timestamp_S 
   * @return TriggeringModeStruct modeID and trigger time of the mode
   */
  TriggeringModeStruct check(uint64_t timestamp_S);

  /**
   * @brief add an event to the map, and determine if it should trigger next. packet and size validations must have already happened, but it will check if the ID doesn't already exist
   * 
   * @param timestamp_S 
   * @param newEventPacket 
   * @return eventError_t 
   */
  eventError_t addEvent(uint64_t timestamp_S, const EventDataPacket &newEventPacket);

  /**
   * @brief rebuilds trigger times, checking for missed events and checking against previousEventTime.
   * 
   * @param timestamp_S 
   */
  void rebuildTriggerTimes(uint64_t timestamp_S);

  /**
   * @brief updates an event and checks if it should be next. if the event was the most recent and it's still in the (new) event window, it won't be re-triggered.
   * 
   * @param timestamp_S 
   * @param event 
   * @return eventError_t 
   */
  eventError_t updateEvent(uint64_t timestamp_S, const EventDataPacket &event);

  /**
   * @brief remove an event and rebuild the trigger times using event windows. previousEvent is unchanged, even if it's the removed event
   * 
   * @param timestamp_S 
   * @param eventID 
   * @return eventError_t 
   */
  eventError_t removeEvent(uint64_t timestamp_S, eventUUID eventID);

  EventTimeStruct getNextEvent(){
    return EventTimeStruct{_nextEvent.ID,_nextEvent.getNextTriggerTime()};
  }

  size_t getNumberOfEvents(){return _events.size();};

  bool doesEventExist(eventUUID eventID){return _events.count(eventID) > 0;}

  /**
   * @brief performs the time update without performing any checks. rebuild trigger times.
   * 
   * @param timeUpdates 
   */
  void updateTime(const uint64_t newTimestamp_S){
    // _previousEvent.triggerTime += roundingDivide(timeUpdates.localTimeChange_uS, secondsToMicros);
    _previousEvent.triggerTime = 0;
    _previousEvent.ID = 0;
    rebuildTriggerTimes(newTimestamp_S);
  };

};

#endif