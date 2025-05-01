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

EventManager::EventManager(
  std::shared_ptr<ModalLightsInterface> modalLights,
  std::shared_ptr<ConfigManagerClass> configs,
  std::shared_ptr<DeviceTimeClass> deviceTime,
  EventStorageIterator events
) : _modalLights(modalLights), _configs(configs), _deviceTime(deviceTime)
{
  uint32_t defaultEventWindow = _configs->getEventManagerConfigs().defaultEventWindow;
  uint64_t timestampS = _deviceTime->getLocalTimestampSeconds();
  // for(EventDataPacket event : eventStructs){
  while(events.hasMore()){
    // timestampS - eventWindow in case there was an alarm that should have triggered just before reboot
    if(_addEvent(timestampS, events.getNext()) != EventManagerErrors::success){
      // TODO: alert server of the error
    };
  };
  eventUUID activeEvent = _findInitialTriggers(timestampS);
  _findNextEvent();
  check();
};

eventError_t EventManager::_addEvent(uint64_t timestampS, EventDataPacket newEvent, bool updating)
{
  eventError_t error = _isNewEventValid(newEvent, updating);
  if(error != EventManagerErrors::success){return error;}
  eventUUID newEventID = newEvent.eventID;

  EventMappingStruct event = {
    0,
    newEvent.modeID,
    newEvent.timeOfDay,
    newEvent.daysOfWeek,
    newEvent.eventWindow,
    newEvent.isActive
  };
  _events[newEventID] = event;

  uint32_t window = newEvent.isActive
    ? _checkEventWindow(newEvent.eventWindow)
    : 0;

  _events[newEventID].nextTriggerTime = _findNextTriggerTime(timestampS - window, newEventID);

  if(_events[newEventID].isActive == false){
    uint64_t previousTriggerTime = findPreviousTriggerTime(_events[newEventID], makeUsefulTimeStruct(timestampS));
    if(previousTriggerTime > _previousBackgroundEventTime){
      _events[newEventID].nextTriggerTime = previousTriggerTime;
    }
  }
  if(_events[newEventID].nextTriggerTime < _nextEventTime){
    _nextEventID = newEventID;
    _nextEventTime = _events[newEventID].nextTriggerTime;
  }
  return error;
}

eventError_t EventManager::_isNewEventValid(EventDataPacket newEvent, bool updating)
{
  // TODO: alert server of bad event packets
  if(newEvent.eventID == 0
    || newEvent.modeID == 0
    || (updating ^ (_events.count(newEvent.eventID) != 0))
  ){
    return EventManagerErrors::bad_uuid;
  }
  if(newEvent.timeOfDay > maxTimeOfDay
    || newEvent.eventWindow > maxTimeOfDay
    || (newEvent.daysOfWeek & daysOfWeekMask) == 0
  ){
    return EventManagerErrors::bad_time;
  }
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
  // TODO: alert system of error because this should be innaccessible
  return ~0;
}

void EventManager::_callEvent(uint64_t timestampS, eventUUID eventID)
{
  modeUUID modeID = _events[eventID].modeID;
  _modalLights->setModeByUUID(modeID, _events[eventID].nextTriggerTime, _events[eventID].isActive);

  if(_events[eventID].isActive == false){
    _previousBackgroundEventTime = _events[eventID].nextTriggerTime;
  }
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

eventError_t EventManager::addEvent(EventDataPacket newEvent)
{
  uint64_t timestampS = _deviceTime->getLocalTimestampSeconds();
  eventUUID newEventID = newEvent.eventID;
  eventError_t error = _addEvent(timestampS, newEvent);
  if(error != EventManagerErrors::success){
    return error;
  }
  _findNextEvent();
  check();
  return error;
}

void EventManager::removeEvents(eventUUID *eventIDs, nEvents_t number)
{
  uint64_t timestampS = _deviceTime->getLocalTimestampSeconds();
  for(int i = 0; i < number; i++){
    _events.erase(eventIDs[i]);
  }
  rebuildTriggerTimes(false);
}

void EventManager::removeEvent(eventUUID eventID)
{
  uint64_t timestampS = _deviceTime->getLocalTimestampSeconds();
  _events.erase(eventID);
  rebuildTriggerTimes(false);
}

void EventManager::updateEvents(EventDataPacket *events, eventError_t *eventErrors, nEvents_t number)
{
  uint64_t timestampS = _deviceTime->getLocalTimestampSeconds();
  for(int i = 0; i < number; i++){
    eventErrors[i] = _addEvent(timestampS, events[i], true);
  }
  _findInitialTriggers(timestampS);
  check();
}

eventError_t EventManager::updateEvent(EventDataPacket event)
{
  uint64_t timestampS = _deviceTime->getLocalTimestampSeconds();
  eventError_t error = _addEvent(timestampS, event, true);
  if(
    error != EventManagerErrors::success
    || _events.count(event.eventID) == 0
  ){
    return error;
  }
  _findInitialTriggers(timestampS);
  check();
  return error;
}

void EventManager::check()
{
  uint64_t timestampS = _deviceTime->getLocalTimestampSeconds();
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

eventUUID EventManager::_findInitialTriggers(uint64_t timestampS)
{
  eventUUID currentBackgroundEventID = 0;
  uint64_t currentBackgroundEventTime = 0;

  eventUUID currentActiveEventID = 0;
  uint64_t currentActiveEventTime = 0;
  std::vector<eventUUID> skippedEvents = {};

  RTCConfigsStruct rtcConfigs = _configs->getRTCConfigs();
  UsefulTimeStruct timeStruct = makeUsefulTimeStruct(timestampS);

  for(auto [eventID, event] : _events){
    if(event.isActive == true){
      uint64_t triggerTime = _events[eventID].nextTriggerTime;
      if(triggerTime <= timestampS
        && triggerTime > currentActiveEventTime
      ){
        if(currentActiveEventID != 0){
          // reset the old currentActiveEventID
          _events[currentActiveEventID].nextTriggerTime = _findNextTriggerTime(timestampS, currentActiveEventID);
        }
        // ear-mark the new most recent active event
        currentActiveEventID = eventID;
        currentActiveEventTime = triggerTime;
      }
      else if(triggerTime <= currentActiveEventTime
      ){
        _events[eventID].nextTriggerTime = _findNextTriggerTime(timestampS, eventID);
      }
    } 
    else {
      // if event is background, check the previous trigger time to see which event should be current
      uint64_t triggerTime = findPreviousTriggerTime(event, timeStruct);
      if(triggerTime > currentBackgroundEventTime){
        if(currentBackgroundEventID != 0){
          _events[currentBackgroundEventID].nextTriggerTime = _findNextTriggerTime(timestampS, currentBackgroundEventID);
        }
        currentBackgroundEventTime = triggerTime;
        currentBackgroundEventID = eventID;
      }
      else if (_events[eventID].nextTriggerTime <= timestampS)
      {
        _events[eventID].nextTriggerTime = _findNextTriggerTime(timestampS, eventID);
      }
      
    }
  }
  if(currentBackgroundEventID != 0){
    _events[currentBackgroundEventID].nextTriggerTime = currentBackgroundEventTime;
  }
  if(currentActiveEventID != 0){
    // the activeEvent should be triggered first, then search for the backgroundEvent
    _nextEventID = currentActiveEventID;
    _nextEventTime = currentActiveEventTime;
    return currentActiveEventID;
  }
  if(currentBackgroundEventID != 0){
    _nextEventID = currentBackgroundEventID;
    _nextEventTime = currentBackgroundEventTime;
    return currentBackgroundEventID;
  }
  return 0;
};

void EventManager::rebuildTriggerTimes(bool checkMissedActive)
{
  // reset all trigger times
  uint64_t timestampS = _deviceTime->getLocalTimestampSeconds();
  for(auto [eventID, event] : _events){
    _events[eventID].nextTriggerTime = _findNextTriggerTime(timestampS - (_checkEventWindow(event.eventWindow) * event.isActive * checkMissedActive), eventID);
  }
  eventUUID activeEvent = _findInitialTriggers(timestampS);
  if(activeEvent != 0){
    _callEvent(timestampS, activeEvent);
  }
  _findNextEvent();
  check();
}