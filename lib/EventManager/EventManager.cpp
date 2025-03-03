#include "EventManager.h"

#define maxTimeOfDay 86399
#define daysOfWeekMask 0b01111111

EventManager::EventManager(uint64_t timestampS, std::vector<TimeEventDataStruct> eventStructs)
{
  for(TimeEventDataStruct event : eventStructs){
    addEvent(timestampS, event);
  }
};

/**
 * @brief adds a new event to the Event Manager, but will not store it.
 * performs some checks, but will not check that two events share a trigger time (that
 * will have to be done by the interface app)
 * 
 * @param timestampS current timestamp in seconds
 * @param newEvent 
 * @return eventError_t 
 */
eventError_t EventManager::addEvent(uint64_t timestampS, TimeEventDataStruct newEvent)
{
  if(newEvent.eventID == 0 || newEvent.modeID == 0){return EventManagerErrors::bad_uuid;}
  if(newEvent.timeOfDay > maxTimeOfDay
    || newEvent.eventWindow > maxTimeOfDay
    || newEvent.daysOfWeek & daysOfWeekMask == 0
  ){
    return EventManagerErrors::bad_uuid;
  }

  StoredEventStruct event;
  event.daysOfWeek = newEvent.daysOfWeek;
  event.isActive = newEvent.isActive;
  event.modeID = newEvent.modeID;
  event.timeOfDay = newEvent.timeOfDay;
  event.eventWindow = newEvent.eventWindow;
  event.nextAlarmTime = _findNextTriggerTime(timestampS, event);
  _events[newEvent.eventID] = event;
  return EventManagerErrors::success;
}

/**
 * @brief finds the next trigger time for a given event. changes _nextEventTime if found time is earlier
 * 
 * @param timestampS current timestamp in microseconds
 * @param event 
 * @return uint64_t event time in seconds
 */
uint64_t EventManager::_findNextTriggerTime(uint64_t timestampS, StoredEventStruct event)
{
  DateTimeStruct dt;

  // get timeInDay and current day (minus the eventWindow)
  convertFromLocalTimestamp(timestampS - event.eventWindow, &dt);
  uint32_t timeInDay = dt.seconds + 60*(dt.minutes + 60*dt.hours);
  uint8_t today = dt.dayOfWeek - 1;

  // get the timestamp for the start of the day
  dt.hours = 0;
  dt.minutes = 0;
  dt.seconds = 0;
  uint64_t startOfDay = convertToLocalTimestamp(&dt);
  for(int x = 0; x < 7; x++){
    if((1 << ((today + x)%7)) & event.daysOfWeek
        && event.timeOfDay > timeInDay
      ){
        
        uint64_t triggerTime = startOfDay + event.timeOfDay + 60*60*24*x;
        if(triggerTime < _nextEventTime){_nextEventTime = triggerTime;}
        return triggerTime;
    };
    timeInDay = 0;
  }
  return ~0;
}