#ifndef __DATA_STORAGE_CLASS__
#define __DATA_STORAGE_CLASS__

#include <Arduino.h>
#include <etl/flat_map.h>

#include "ProjectDefines.h"
#include "storageHAL.h"
#include "DataStorageIterator.h"

typedef DataStorageIterator<nEvents_t, EventDataPacket> EventStorageIterator;

// typedef DataStorageIterator<nModes_t, ModeDataPacket> ModeStorageIterator;

class DataStorageClass
{
private:
  std::shared_ptr<StorageHALInterface> _storage;

  storedEventIDsMap_t _storedEventIDs;         // TODO: ID should map to an address in FRAM/EEPROM
  storedModeIDsMap_t _storedModeIDs; // default constant brightness should always be in progmem

public:
  DataStorageClass(std::shared_ptr<StorageHALInterface> storage) : _storage(std::move(storage)){
    // TODO: defer until all classes are constructed
    _storage->getModeIDs(_storedModeIDs);
    _storage->getEventIDs(_storedEventIDs);
  };
  
  /**
   * @brief returns an iterator to get all of the stored modes
   * TODO: will this ever get used
   * @return ModeStorageIterator 
   */
  // ModeStorageIterator getAllModes();

  /**
   * @brief returns an iterator to get all of the stored events
   * 
   * @return EventStorageIterator 
   */
  EventStorageIterator getAllEvents();

  bool getMode(modeUUID modeID, uint8_t dataPacket[modePacketSize]);

  struct EventDataPacket getEvent(eventUUID eventID);

  bool doesModeExist(modeUUID modeID){
    return _storedModeIDs.contains(modeID) || (modeID == 1);
  }
};

#endif