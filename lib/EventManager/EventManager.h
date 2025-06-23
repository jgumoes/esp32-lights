#ifndef __EVENT_MANAGER_H__
#define __EVENT_MANAGER_H__

#include <Arduino.h>
#include <etl/flat_map.h>

#include "ProjectDefines.h"
#include "ConfigStorageClass.hpp"

#include "ModalLights.h"
#include "DeviceTime.h"

#include "EventSupervisor.h"

class EventManager : public TimeObserver, public ConfigUser
{
private:
  std::shared_ptr<ModalLightsInterface> _modalLights;
  std::shared_ptr<DeviceTimeClass> _deviceTime;
  std::shared_ptr<DataStorageClass> _storage;
  
  std::shared_ptr<ConfigStorageClass> _configManager;
  EventManagerConfigsStruct _configs;

  std::unique_ptr<ActiveEventSupervisor> _active;
  std::unique_ptr<BackgroundEventSupervisor> _background;

  void _check(const uint64_t timestamp_S);

public:
  // TODO: integrate ErrorManager
  EventManager(
    std::shared_ptr<ModalLightsInterface> modalLights,
    std::shared_ptr<ConfigStorageClass> configStorage,
    std::shared_ptr<DeviceTimeClass> deviceTime,
    std::shared_ptr<DataStorageClass> storage
  );
  ~EventManager(){
    _deviceTime->remove_observer(*this);
  };

  /**
   * @brief loads the events from storage. should be called AFTER ConfigStorage loads configs, and AFTER ModalLights loads modes
   * 
   */
  void loadAllEvents();

  /**
   * @brief checks the validity of the new event data packet, and returns the first error it runs in to. the final check is if the storage is full
   * 
   * @param newEvent 
   * @param updating if true, eventID should already exist. if false, it shouldn't
   * @return errorCode_t 
   */
  errorCode_t isEventDataPacketValid(const EventDataPacket &newEvent, const bool updating);
  
  /**
   * @brief Get the ID and time of the next event
   * 
   * @return EventTimeStruct 
   */
  EventTimeStruct getNextEvent();

  EventTimeStruct getNextActiveEvent(){return _active->getNextEvent();}

  EventTimeStruct getNextBackgroundEvent(){
    return _background->getNextEvent();
  }
  
  /**
   * @brief checks if a new event is valid, and adds it to the map then checks for triggers.
   * TODO: adds the event to storage
   * 
   * @param newEvent 
   * @return errorCode_t 
   */
  errorCode_t addEvent(const EventDataPacket& newEvent);
  
  /**
   * @brief removes an event from the map, and re-builds trigger times.
   * TODO: also removes event from storage
   * 
   * @param eventID 
   * @return errorCode_t 
   */
  errorCode_t removeEvent(eventUUID eventID);

  /**
   * @brief removes a list of events.
   * TODO: deleteme
   * 
   * @param eventIDs 
   * @param number 
   */
  void removeEvents(eventUUID *eventIDs, nEvents_t number);
  
  /**
   * @brief checks an event and updates it. updates trigger times, then checks for triggers.
   * 
   * @param event 
   * @return errorCode_t 
   */
  errorCode_t updateEvent(EventDataPacket event);
  
  /**
   * @brief loops through a list of events and calls updateEvent()
   * TODO: delete me
   * 
   * @param events 
   * @param eventErrors 
   * @param number 
   */
  void updateEvents(EventDataPacket *events, errorCode_t *eventErrors, nEvents_t number);
  
  /**
   * @brief rebuilds the trigger times, checking for missed active events, then checks for triggering events. can cause background modes to retrigger, but how that gets handled is mode-dependant so really its a ModalController problem.
   * 
   */
  void rebuildTriggerTimes();

  void check();

  void notification(const TimeUpdateStruct& timeUpdates);

  EventManagerConfigsStruct getConfigs(){
    return _configs;
  }

  void newConfigs(const byte newConfig[maxConfigSize]) override {
    ConfigStructFncs::deserialize(newConfig, _configs);
    const uint64_t timestamp = _deviceTime->getLocalTimestampSeconds();
    _active->rebuildTriggerTimes(timestamp);
    _background->rebuildTriggerTimes(timestamp);
    _check(timestamp);
  }

  packetSize_t getConfigs(byte config[maxConfigSize]) override {
    ConfigStructFncs::serialize(config, _configs);
    return _configs.rawDataSize+1;
  }

};

#endif