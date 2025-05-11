#include "EventManager.h"

EventManager::EventManager(std::shared_ptr<ModalLightsInterface> modalLights, std::shared_ptr<ConfigManagerClass> configManager, std::shared_ptr<DeviceTimeClass> deviceTime, EventStorageIterator events)
  : _modalLights(modalLights), _configManager(configManager), _configs(configManager->getEventManagerConfigs()),
    _active(std::make_unique<ActiveEventSupervisor>(_configs)),
    _background(std::make_unique<BackgroundEventSupervisor>(_configs)), _deviceTime(deviceTime)
{
  const uint64_t timestamp_S = _deviceTime->getLocalTimestampSeconds();
  while(events.hasMore()){
    EventDataPacket event = events.getNext();
    eventError_t error = isEventDataPacketValid(event, false);
    if(error == EventManagerErrors::success){
      error = event.isActive
            ? _active->addEvent(timestamp_S, event)
            : _background->addEvent(timestamp_S, event);
    }
    if(error != EventManagerErrors::success){
      // TODO: alert server of the error
    };
  };
  _deviceTime->add_observer(*this);
  _check(timestamp_S);
}

eventError_t EventManager::isEventDataPacketValid(const EventDataPacket &newEvent, const bool updating){
  const bool duplicateEventID =
    _active->doesEventExist(newEvent.eventID)
    || _background->doesEventExist(newEvent.eventID);
  
  if(newEvent.eventID == 0
    || newEvent.modeID == 0
    || (duplicateEventID != updating)
  ){
    return EventManagerErrors::bad_uuid;
  }
  if(newEvent.timeOfDay > maxTimeOfDay
    || newEvent.eventWindow > maxTimeOfDay
    || (newEvent.daysOfWeek & daysOfWeekMask) == 0
  ){
    return EventManagerErrors::bad_time;
  }

  // IMPORTANT! this MUST be the final check, as it has nothing to with the EventDataPacket
  if(_active->getNumberOfEvents() + _background->getNumberOfEvents() >= MAX_NUMBER_OF_EVENTS){
    return EventManagerErrors::storage_full;
  }
  return EventManagerErrors::success;
}

void EventManager::_check(const uint64_t timestamp_S){
  {
    TriggeringModeStruct activeMode = _active->check(timestamp_S);
    if(activeMode.ID != 0){
      _modalLights->setModeByUUID(activeMode.ID, activeMode.triggerTime, true);
    }
  }
  {
    TriggeringModeStruct backgroundMode = _background->check(timestamp_S);
    if(backgroundMode.ID != 0){
      _modalLights->setModeByUUID(backgroundMode.ID, backgroundMode.triggerTime, false);
    }
  }
}


EventTimeStruct EventManager::getNextEvent(){
  const EventTimeStruct nextActive = _active->getNextEvent();
  const EventTimeStruct nextBackground = _background->getNextEvent();
  
  const bool backgroundTriggersFirst = (
    nextActive.triggerTime > nextBackground.triggerTime
    && nextBackground.triggerTime != 0
  );
  if(
    !backgroundTriggersFirst
    && nextActive.ID != 0
  ){
    return nextActive;
  }
  return nextBackground;
};

eventError_t EventManager::addEvent(const EventDataPacket& newEvent){
  eventError_t error = isEventDataPacketValid(newEvent, false);
  if(error != EventManagerErrors::success){
    return error;
  }
  uint64_t timestamp_S = _deviceTime->getLocalTimestampSeconds();
  error = newEvent.isActive
        ? _active->addEvent(timestamp_S, newEvent)
        : _background->addEvent(timestamp_S, newEvent);
  _check(timestamp_S);

  // TODO: add event to storage
  return error;
};

eventError_t EventManager::removeEvent(eventUUID eventID){
  const uint64_t timestamp_S = _deviceTime->getLocalTimestampSeconds();
  eventError_t error = _active->removeEvent(timestamp_S, eventID);
  if(error != EventManagerErrors::success){
    error = _background->removeEvent(timestamp_S, eventID);
  }
  // TODO: remove from device storage
  _check(timestamp_S);
  return error;
};

void EventManager::removeEvents(eventUUID *eventIDs, nEvents_t number){
  for(int i = 0; i < number; i++){
    removeEvent(eventIDs[i]);
  }
};

eventError_t EventManager::updateEvent(EventDataPacket event){
  eventError_t error = isEventDataPacketValid(event, true);
  if(error != EventManagerErrors::success){
    return error;
  }

  const uint64_t timestamp_S = _deviceTime->getLocalTimestampSeconds();
  if(event.isActive){
    error = _active->updateEvent(timestamp_S, event);
    if(error == EventManagerErrors::event_not_found){
      // if changing background to active
      if(_background->removeEvent(timestamp_S, event.eventID) == EventManagerErrors::success){
        error = _active->addEvent(timestamp_S, event);
      }
    }
  }
  else{
    // if event is background
    error = _background->updateEvent(timestamp_S, event);
    if(error == EventManagerErrors::event_not_found){
      // if changing active to background
      if(_active->removeEvent(timestamp_S, event.eventID)){
        error = _background->addEvent(timestamp_S, event);
      }
    }
  }
  // TODO: update in storage
  _check(timestamp_S);
  return error;
};

void EventManager::updateEvents(EventDataPacket *events, eventError_t *eventErrors, nEvents_t number){
  for(int i = 0; i < number; i++){
    eventErrors[i] = updateEvent(events[i]);
  }
  check();
};

void EventManager::rebuildTriggerTimes(){
  const uint64_t timestamp_S = _deviceTime->getLocalTimestampSeconds();
  _active->rebuildTriggerTimes(timestamp_S);
  _background->rebuildTriggerTimes(timestamp_S);
  _check(timestamp_S);
};

void EventManager::check(){
  _check(_deviceTime->getLocalTimestampSeconds());
};

void EventManager::notification(const TimeUpdateStruct& timeUpdates){
  uint64_t adjWindow_uS = _configs.defaultEventWindow_S * secondsToMicros;
  bool bigChange = abs(timeUpdates.localTimeChange_uS) > adjWindow_uS;
  bool negativeChange = timeUpdates.localTimeChange_uS < 0;

  uint64_t newTimestamp_S = roundMicrosToSeconds(timeUpdates.currentLocalTime_uS);
  
  /*
  desired behaviour:
    if negative adjustment:
      - small change rebuilds background events only
      - large change rebuilds active too
    if positive adjustment:
      - under event window gets ignored, but check for triggers
      - over event window gets rebuilt
    */
   
  if(bigChange){
    _active->updateTime(newTimestamp_S);
  }
  if(bigChange || negativeChange){
    _background->updateTime(newTimestamp_S);
  }
  _check(newTimestamp_S);
};
