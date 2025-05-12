#include "EventSupervisor.h"

/*############################################################
common functions for Background and Active containers
############################################################*/

/**
 * @brief returns true if eventTime is not in future and after nextEventTime, or nextEventTime is future and after event time. should return false if eventTime == nextEventTime
 * 
 * @param eventTime 
 * @param nextEventTime 
 * @param now 
 * @return true 
 * @return false 
 */
inline bool isEventCloserThanNext(const uint64_t& eventTime, const uint64_t& nextEventTime, const uint64_t& now){
  return (
    (eventTime <= now && eventTime > nextEventTime)
    || (nextEventTime > now && eventTime < nextEventTime)
  );
};

uint64_t findNextTriggerTime(const UsefulTimeStruct& uts, const EventMappingStruct* event){
  uint32_t timeInDay = uts.timeInDay;
  uint8_t today = uts.dayOfWeek - 1;
  uint64_t startOfDay = uts.startOfDay;

  for(int x = 0; x < 7; x++){
    bool doesEventTriggerOnDay = (1 << ((today + x)%7)) & event->daysOfWeek;
    bool doesEventTriggerAfterTimeInDay = event->timeOfDay > timeInDay;
    if(doesEventTriggerOnDay
        && doesEventTriggerAfterTimeInDay
      ){
        
        uint64_t triggerTime = startOfDay + event->timeOfDay + 60*60*24*x;
        return triggerTime;
    };
    timeInDay = 0;
  }
  // TODO: alert system of error because this should be innaccessible
  return ~0;
}

void findNextEvent(NextEventStruct& _nextEvent, EventMap_t& _events){
  _nextEvent.ID = 0;
  _nextEvent.data = nullptr;
  for(EventMap_t::iterator mapIt = _events.begin(); mapIt != _events.end(); mapIt ++){
    eventUUID eventID = mapIt->first;
    EventMappingStruct* event = &mapIt->second;
    if(event->nextTriggerTime < _nextEvent.getNextTriggerTime()){
      _nextEvent.ID = eventID;
      _nextEvent.data = &mapIt->second;
    }
  }
}

/*############################################################
Background Event Container
############################################################*/

uint64_t BackgroundEventSupervisor::_findPreviousTriggerTime(const EventMappingStruct& event, const UsefulTimeStruct &timeStruct){
  uint8_t today = timeStruct.dayOfWeek - 1;
  uint32_t timeInDay = timeStruct.timeInDay;
  for(int x = 0; x < 7; x++){
    uint8_t searchDay = (1 << ((7 + today - x)%7));
    if(searchDay & event.daysOfWeek
        && event.timeOfDay <= timeInDay
      ){
        
        return timeStruct.startOfDay + event.timeOfDay - 60*60*24*x;
    };
    timeInDay = maxTimeOfDay;
  }
  // TODO: alert an error because the function should not reach here
  return 0;
};

void BackgroundEventSupervisor::_shouldEventBeNext(eventUUID eventID, EventMappingStruct *eventData, const UsefulTimeStruct &uts){
  if(_nextEvent.data == nullptr){
    _nextEvent.ID = eventID;
    _nextEvent.data = eventData;
    return;
  }

 const bool eventTriggersAfterPrevious = eventData->nextTriggerTime > _previousEvent.triggerTime;

  if(eventData->nextTriggerTime < _previousEvent.triggerTime){
    eventData->nextTriggerTime = findNextTriggerTime(uts, eventData);
  }

  if(
    isEventCloserThanNext(eventData->nextTriggerTime, _nextEvent.getNextTriggerTime(), uts.startOfDay + uts.timeInDay)
  ){
    _nextEvent.data->nextTriggerTime = findNextTriggerTime(uts, _nextEvent.data);
    _nextEvent.ID = eventID;
    _nextEvent.data = eventData;
    return;
  }

  // if event isn't closer than nextEvent but is past, its trigger is in the future
  if(eventData->nextTriggerTime < (uts.startOfDay + uts.timeInDay)){
    eventData->nextTriggerTime = findNextTriggerTime(uts, eventData);
  }
}

TriggeringModeStruct BackgroundEventSupervisor::check(uint64_t timestamp_S){
  TriggeringModeStruct triggeringMode;
  if(_events.size() == 0){
    // no background events defaults to constant brightness mode
    triggeringMode.ID = 1;
    triggeringMode.triggerTime = 0;
    return triggeringMode;
  }
  uint16_t limit = 0;
  while(timestamp_S >= _nextEvent.getNextTriggerTime()){
    if(limit >= _events.size()){
      // TODO: alert error the _check limit has been reached
      break;
    }
    limit++;

    EventMappingStruct* event = _nextEvent.data;
    triggeringMode.ID = event->modeID;
    triggeringMode.triggerTime = event->nextTriggerTime;
    _previousEvent.ID = _nextEvent.ID;
    _previousEvent.triggerTime = _nextEvent.getNextTriggerTime();
    event->nextTriggerTime = findNextTriggerTime(UsefulTimeStruct(timestamp_S + 1), event);

    findNextEvent(_nextEvent, _events);
  }
  return triggeringMode;
}

eventError_t BackgroundEventSupervisor::addEvent(uint64_t timestamp_S, const EventDataPacket &newEvent){
  eventUUID newEventID = newEvent.eventID;
  if(_events.count(newEventID) != 0){
    return EventManagerErrors::bad_uuid;
  }

  const UsefulTimeStruct uts = UsefulTimeStruct(timestamp_S);
  auto pair = _events.emplace(newEventID, EventMappingStruct(newEvent));
  EventMappingStruct* event = &pair.first->second;

  event->nextTriggerTime = _findPreviousTriggerTime(*event, uts);
  _shouldEventBeNext(newEventID, event, uts);

  return EventManagerErrors::success;
}

void BackgroundEventSupervisor::_rebuildTriggerTimes(uint64_t timestamp_S){
  // TODO: accept UsefulTimeStruct instead of timestamp?
  const UsefulTimeStruct uts = UsefulTimeStruct(timestamp_S);
  
  _nextEvent.ID = 0;
  _nextEvent.data = nullptr;

  if(_events.size() == 0){
    return;
  }
  
  for(EventMap_t::iterator mapIt = _events.begin(); mapIt != _events.end(); mapIt++){
    eventUUID eventID = mapIt->first;
    EventMappingStruct* event = &mapIt->second;
    event->nextTriggerTime = _findPreviousTriggerTime(*event, uts);
    _shouldEventBeNext(eventID, event, uts);
  }
}

eventError_t BackgroundEventSupervisor::updateEvent(uint64_t timestamp_S, const EventDataPacket &eventPacket){
  eventUUID eventID = eventPacket.eventID;
  if(_events.count(eventID) == 0){
    return EventManagerErrors::event_not_found;
  }
  UsefulTimeStruct uts = UsefulTimeStruct(timestamp_S);
  auto pair = _events.find(eventID);
  EventMappingStruct* event = &pair->second;
  *event = EventMappingStruct(eventPacket);

  rebuildTriggerTimes(timestamp_S);
  return EventManagerErrors::success;
}

eventError_t BackgroundEventSupervisor::removeEvent(uint64_t timestamp_S, eventUUID eventID)
{
  if(_events.count(eventID) == 0){
    return EventManagerErrors::event_not_found;
  }

  _events.erase(eventID);
  rebuildTriggerTimes(timestamp_S);
  return EventManagerErrors::success;
}



/*############################################################
Active Event Container
############################################################*/

void ActiveEventSupervisor::_shouldEventBeNext(eventUUID eventID, EventMappingStruct *eventData, const UsefulTimeStruct &uts){
  if(_nextEvent.data == nullptr){
    _nextEvent.ID = eventID;
    _nextEvent.data = eventData;
    return;
  }

  if(eventData->nextTriggerTime < _previousEvent.triggerTime){
    eventData->nextTriggerTime = findNextTriggerTime(uts, eventData);
  }
  
  if(
    isEventCloserThanNext(eventData->nextTriggerTime, _nextEvent.getNextTriggerTime(), uts.startOfDay + uts.timeInDay)
  ){
    _nextEvent.data->nextTriggerTime = findNextTriggerTime(uts, _nextEvent.data);
    _nextEvent.ID = eventID;
    _nextEvent.data = eventData;
  }
}

TriggeringModeStruct ActiveEventSupervisor::check(uint64_t timestamp_S){
  TriggeringModeStruct triggeringMode;
  uint16_t limit = 0;
  while(timestamp_S >= _nextEvent.getNextTriggerTime()){
    if(limit >= _events.size()){
      // TODO: alert error the _check limit has been reached
      break;
    }
    limit++;

    EventMappingStruct* event = _nextEvent.data;

    if(timestamp_S <= _nextEvent.getNextTriggerTime() + _checkEventWindow(event->eventWindow)){
      // only trigger if the event hasn't expired
      triggeringMode.ID = event->modeID;
      triggeringMode.triggerTime = event->nextTriggerTime;
      
      _previousEvent.ID = _nextEvent.ID;
      _previousEvent.triggerTime = _nextEvent.getNextTriggerTime();
    }
    event->nextTriggerTime = findNextTriggerTime(UsefulTimeStruct(timestamp_S + 1), event);

    findNextEvent(_nextEvent, _events);
  }
  return triggeringMode;
}

eventError_t ActiveEventSupervisor::addEvent(uint64_t timestamp_S, const EventDataPacket &newEvent){
  eventUUID newEventID = newEvent.eventID;
  if(_events.count(newEventID) != 0){
    return EventManagerErrors::bad_uuid;
  }

  const UsefulTimeStruct uts = UsefulTimeStruct(timestamp_S);
  auto pair = _events.emplace(newEventID, EventMappingStruct(newEvent));
  EventMappingStruct* event = &pair.first->second;

  event->nextTriggerTime = _findNextTriggerWithWindow(timestamp_S, event);
  
  _shouldEventBeNext(newEventID, event, uts);

  return EventManagerErrors::success;
}

void ActiveEventSupervisor::rebuildTriggerTimes(uint64_t timestamp_S){
  // TODO: accept UsefulTimeStruct instead of timestamp?
  const UsefulTimeStruct uts = UsefulTimeStruct(timestamp_S);
  
  _nextEvent.ID = 0;
  _nextEvent.data = nullptr;

  if(_events.size() == 0){
    return;
  }
  
  for(EventMap_t::iterator mapIt = _events.begin(); mapIt != _events.end(); mapIt++){
    eventUUID eventID = mapIt->first;
    EventMappingStruct* event = &mapIt->second;
    event->nextTriggerTime = _findNextTriggerWithWindow(timestamp_S, event);
    _shouldEventBeNext(eventID, event, uts);
  }
}

eventError_t ActiveEventSupervisor::updateEvent(uint64_t timestamp_S, const EventDataPacket &eventPacket){
  eventUUID eventID = eventPacket.eventID;
  if(_events.count(eventID) == 0){
    return EventManagerErrors::event_not_found;
  }

  UsefulTimeStruct uts = UsefulTimeStruct(timestamp_S);
  auto pair = _events.find(eventID);
  EventMappingStruct* event = &pair->second;
  *event = EventMappingStruct(eventPacket);

  // if event just triggered, moving it forwards a few minutes but still in the past shouldn't re-trigger.
  const bool eventJustTriggered = (
    eventID == _previousEvent.ID
    && timestamp_S <= _previousEvent.triggerTime + _checkEventWindow(event->eventWindow)
  );
  event->nextTriggerTime = !eventJustTriggered
    ? _findNextTriggerWithWindow(timestamp_S, event)
    : findNextTriggerTime(uts, event);
  
  if(event->nextTriggerTime < _previousEvent.triggerTime){
    event->nextTriggerTime = findNextTriggerTime(uts, event);
  };
  _shouldEventBeNext(eventID, event, uts);
  return EventManagerErrors::success;
}

eventError_t ActiveEventSupervisor::removeEvent(uint64_t timestamp_S, eventUUID eventID){
  if(_events.count(eventID) == 0){
    return EventManagerErrors::event_not_found;
  }

  _events.erase(eventID);
  rebuildTriggerTimes(timestamp_S);
  return EventManagerErrors::success;
}

uint64_t ActiveEventSupervisor::_findNextTriggerWithWindow(const uint64_t timestamp_S, const EventMappingStruct *event){
  return findNextTriggerTime(UsefulTimeStruct(timestamp_S - _checkEventWindow(event->eventWindow)), event);
}
