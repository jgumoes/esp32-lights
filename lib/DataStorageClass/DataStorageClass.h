#ifndef __DATA_STORAGE_CLASS__
#define __DATA_STORAGE_CLASS__

#include <Arduino.h>
#include <map>

#include "ProjectDefines.h"
#include "storageHAL.h"
#include "DataStorageIterator.h"

typedef DataStorageIterator<nEvents_t, EventDataPacket> EventStorageIterator;

typedef DataStorageIterator<nModes_t, ModeDataPacket> ModeStorageIterator;

class DataStorageClass
{
private:
  std::shared_ptr<StorageHALInterface> _storage;

  std::map<modeUUID, nModes_t> _storedModeIDs;
  std::map<eventUUID, nEvents_t> _storedEventIDs;

public:
  DataStorageClass(std::shared_ptr<StorageHALInterface> storage) : _storage(std::move(storage)){
    _storage->getModeIDs(_storedModeIDs);
    _storage->getEventIDs(_storedEventIDs);
  };
  
  /**
   * @brief returns an iterator to get all of the stored modes
   * 
   * @return ModeStorageIterator 
   */
  ModeStorageIterator getAllModes();

  /**
   * @brief returns an iterator to get all of the stored events
   * 
   * @return EventStorageIterator 
   */
  EventStorageIterator getAllEvents();

  struct ModeDataPacket getMode(modeUUID modeID);

  struct EventDataPacket getEvent(eventUUID eventID);
};

#endif