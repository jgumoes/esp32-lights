#include "DataStorageClass.h"

ModeStorageIterator DataStorageClass::getAllModes(){
  ModeStorageIterator iterator(_storage, _storage->getNumberOfStoredModes());
  return iterator;
};

EventStorageIterator DataStorageClass::getAllEvents(){
  EventStorageIterator iterator(_storage, _storage->getNumberOfStoredEvents());
  return iterator;
};

ModeDataPacket DataStorageClass::getMode(modeUUID modeID){
  nModes_t number = 0;
  bool foundIt = false;
  if(_storedModeIDs.count(modeID) != 0){
    return _storage->getModeAt(_storedModeIDs[modeID]);
  };
  ModeDataPacket emptyPacket;
  return emptyPacket;
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

