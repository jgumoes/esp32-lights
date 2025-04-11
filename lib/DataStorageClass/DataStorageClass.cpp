#include "DataStorageClass.h"
#include "ProjectDefines.h"

// ModeStorageIterator DataStorageClass::getAllModes(){
//   ModeStorageIterator iterator(_storage, _storage->getNumberOfStoredModes());
//   return iterator;
// };

EventStorageIterator DataStorageClass::getAllEvents(){
  EventStorageIterator iterator(_storage, _storage->getNumberOfStoredEvents());
  return iterator;
};

bool DataStorageClass::getMode(modeUUID modeID, uint8_t dataPacket[modePacketSize]){
  // TODO: if a mode can't be loaded, it should be asynchroniously requested from network

  // if default constant brightness:
  if(modeID == 1){
    dataPacket[0] = 1;
    dataPacket[1] = static_cast<uint8_t>(ModeTypes::constantBrightness);
    for(uint8_t i = 2; i<nChannels+2; i++){
      dataPacket[i] = 255;
    }
    return true;
  }
  // if modeID not found:
  if(_storedModeIDs.count(modeID) == 0){
    return false;
  };
  bool success = _storage->getModeAt(_storedModeIDs[modeID], dataPacket);
  // if(success && CRC_checksOut){} // TODO
  return success;
};

EventDataPacket DataStorageClass::getEvent(eventUUID eventID){
  nEvents_t number = 0;
  bool foundIt = false;
  if(_storedEventIDs.count(eventID) != 0){
    return _storage->getEventAt(_storedEventIDs[eventID]);
  };
  EventDataPacket emptyPacket;
  return emptyPacket;
};

