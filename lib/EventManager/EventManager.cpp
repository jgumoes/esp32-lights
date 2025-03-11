#include "EventManager.h"

#define maxTimeOfDay 86399
#define daysOfWeekMask 0b01111111

uint64_t inline findPreviousTriggerTime(EventMappingStruct event, UsefulTimeStruct timeStruct){
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

EventManager::EventManager(std::shared_ptr<ModalLightsInterface> modalLights, std::shared_ptr<ConfigManagerClass> configs, uint64_t timestampS, std::vector<EventDataPacket> eventStructs) : _modalLights(modalLights), _configs(configs)
{
  uint32_t defaultEventWindow = _configs->getEventManagerConfigs().defaultEventWindow;
  for(EventDataPacket event : eventStructs){
    // timestampS - eventWindow in case there was an alarm that should have triggered just before reboot
    if(_addEvent(timestampS - (_checkEventWindow(event.eventWindow) * event.isActive), event) != EventManagerErrors::success){
      // TODO: alert server of the error
    };
  };
  eventUUID activeEvent = _findInitialTriggers(timestampS);
  if(activeEvent != 0){
    _callEvent(timestampS, activeEvent);
  };
  _findNextEvent();
  check(timestampS);
};

eventError_t EventManager::_addEvent(uint64_t timestampS, EventDataPacket newEvent)
{
  if(newEvent.eventID == 0
    || newEvent.modeID == 0
    || _events.count(newEvent.eventID) != 0
  ){
    return EventManagerErrors::bad_uuid;
  }
  if(newEvent.timeOfDay > maxTimeOfDay
    || newEvent.eventWindow > maxTimeOfDay
    || (newEvent.daysOfWeek & daysOfWeekMask) == 0
  ){
    return EventManagerErrors::bad_time;
  }
  // TODO: alert server of bad event packets

  EventMappingStruct event = {
    0,
    newEvent.modeID,
    newEvent.timeOfDay,
    newEvent.daysOfWeek,
    newEvent.eventWindow,
    newEvent.isActive
  };
  _events[newEvent.eventID] = event;
  _events[newEvent.eventID].nextTriggerTime = _findNextTriggerTime(timestampS, newEvent.eventID);
  return EventManagerErrors::success;
}

uint64_t EventManager::_findNextTriggerTime(uint64_t timestampS, eventUUID eventID)
{
  // TODO: replace with UsefulTimeStruct
  DateTimeStruct dt;

  // get timeInDay and current day
  convertFromLocalTimestamp(timestampS, &dt);
  uint32_t timeInDay = dt.seconds + 60*(dt.minutes + 60*dt.hours);
  uint8_t today = dt.dayOfWeek - 1;

  // get the timestamp for the start of the day
  dt.hours = 0;
  dt.minutes = 0;
  dt.seconds = 0;
  uint64_t startOfDay = convertToLocalTimestamp(&dt);
  for(int x = 0; x < 7; x++){
    bool doesEventTriggerOnDay = (1 << ((today + x)%7)) & _events[eventID].daysOfWeek;
    bool doesEventTriggerAfterTimeInDay = _events[eventID].timeOfDay > timeInDay;
    if(doesEventTriggerOnDay
        && doesEventTriggerAfterTimeInDay
      ){
        
        uint64_t triggerTime = startOfDay + _events[eventID].timeOfDay + 60*60*24*x;
        return triggerTime;
    };
    timeInDay = 0;
  }
  return ~0;
}

void EventManager::_callEvent(uint64_t timestampS, eventUUID eventID)
{
  modeUUID modeID = _events[eventID].modeID;
  _modalLights->setModeByUUID(modeID);
  if(_events[eventID].isActive == false){_previousBackgroundEventTime = _events[eventID].nextTriggerTime;}
  _events[_nextEventID].nextTriggerTime = _findNextTriggerTime(timestampS + 1, _nextEventID);
}

void EventManager::_findNextEvent()
{
  _nextEventID = 0;
  _nextEventTime = ~0;
  for(const auto& [eventID, event] : _events){
    if(event.nextTriggerTime < _nextEventTime
      || (event.nextTriggerTime == _nextEventTime && event.isActive)
    ){
      _nextEventID = eventID;
      _nextEventTime = event.nextTriggerTime;
    }
  }
}

eventError_t EventManager::addEvent(uint64_t timestampS, EventDataPacket newEvent)
{
  eventUUID newEventID = newEvent.eventID;
  uint64_t searchTimestamp = newEvent.isActive
    ? searchTimestamp 
    : searchTimestamp;
  eventError_t error = _addEvent(timestampS - (newEvent.isActive*_checkEventWindow(newEvent.eventWindow)), newEvent);
  if(error != EventManagerErrors::success){
    return error;
  }
  if(_events[newEventID].isActive == false){
    uint64_t previousTriggerTime = findPreviousTriggerTime(_events[newEventID], getUsefulTimeStruct(timestampS));
    if(previousTriggerTime > _previousBackgroundEventTime){
      _callEvent(timestampS, newEventID);
    }
  }
  if(_events[newEventID].nextTriggerTime < _nextEventTime){
    _nextEventID = newEventID;
    _nextEventTime = _events[newEventID].nextTriggerTime;
  }
  check(timestampS);
  return EventManagerErrors::success;
}

void EventManager::check(uint64_t timestampS)
{
  {
    while(timestampS >= _nextEventTime){
      if(_events[_nextEventID].isActive == false
        || (_events[_nextEventID].isActive
        && timestampS <= (_nextEventTime + _checkEventWindow(_events[_nextEventID].eventWindow)))
      ){
        _callEvent(timestampS, _nextEventID);
      }
      else {
        _events[_nextEventID].nextTriggerTime = _findNextTriggerTime(timestampS, _nextEventID);
      }
      _findNextEvent();
    }
  }
}

eventUUID EventManager::_findInitialTriggers(uint64_t timestampS)
{
  eventUUID currentBackgroundEventID = 0;
  uint64_t currentBackgroundEventTime = 0;

  eventUUID currentActiveEventID = 0;
  uint64_t currentActiveEventTime = 0;
  std::vector<eventUUID> skippedEvents = {};

  UsefulTimeStruct timeStruct = getUsefulTimeStruct(timestampS);

  for(auto [eventID, event] : _events){
    if(event.isActive == true){
      // uint64_t triggerTime = _findNextTriggerTime(timestampS - event.eventWindow, eventID);
      uint64_t triggerTime = _events[eventID].nextTriggerTime;
      if(triggerTime <= timestampS
        && triggerTime > currentActiveEventTime
      ){
        // ear-mark the most recent active event
        if(currentActiveEventID != 0){
          _events[currentActiveEventID].nextTriggerTime = _findNextTriggerTime(timestampS, eventID);
        }
        currentActiveEventID = eventID;
        currentActiveEventTime = triggerTime;
      }
    } 
    else {
      // if event is background, check the previous trigger time to see which event should be current
      uint64_t triggerTime = findPreviousTriggerTime(event, timeStruct);
      if(triggerTime > currentBackgroundEventTime){
        currentBackgroundEventTime = triggerTime;
        currentBackgroundEventID = eventID;
      }
    }
  }
  if(currentBackgroundEventID != 0){
    _events[currentBackgroundEventID].nextTriggerTime = currentBackgroundEventTime;
  }
  if(currentActiveEventID != 0){
    _nextEventID = currentActiveEventID;
    return currentActiveEventID;
  }
  return 0;
};

void EventManager::rebuildTriggerTimes(uint64_t timestampS)
{
  // reset all trigger times
  for(auto [eventID, event] : _events){
    _events[eventID].nextTriggerTime = _findNextTriggerTime(timestampS - (_checkEventWindow(event.eventWindow) * event.isActive), eventID);
  }
  eventUUID activeEvent = _findInitialTriggers(timestampS);
  if(activeEvent != 0){
    _callEvent(timestampS, activeEvent);
  }
  _findNextEvent();
  check(timestampS);
}