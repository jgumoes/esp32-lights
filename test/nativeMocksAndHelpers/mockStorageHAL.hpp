#ifndef __MOCK_STORAGE_HAL_HPP__
#define __MOCK_STORAGE_HAL_HPP__

#include "ProjectDefines.h"
#include "DataStorageClass.h"

class MockStorageHAL : public StorageHALInterface{
  private:
    std::vector<ModeDataPacket> _storedModes;
    std::vector<EventDataPacket> _storedEvents;

  public:
    ModeDataPacket modeBuffer[DataPreloadChunkSize];
    nModes_t modeChunkNumber;
    uint8_t fillModeChunkCallCount = 0;
    EventDataPacket eventBuffer[DataPreloadChunkSize];
    nEvents_t eventChunkNumber;
    uint8_t fillEventChunkCallCount = 0;
    
    MockStorageHAL(std::vector<ModeDataPacket> modeDataPackets,
      std::vector<EventDataPacket> eventDataPackets)
      : _storedModes(modeDataPackets), _storedEvents(eventDataPackets){
        ModeDataPacket emptyMode;
        EventDataPacket emptyEvent;
        for(int i = 0; i<DataPreloadChunkSize; i++){
          modeBuffer[i] = emptyMode;
          eventBuffer[i] = emptyEvent;
        }
      }

    void getModeIDs(std::map<modeUUID, nModes_t>& storedIDs){
      for(int i = 0; i < _storedModes.size(); i++){
        storedIDs[_storedModes.at(i).modeID] = i;
      }
    };
    void getEventIDs(std::map<eventUUID, nEvents_t>& storedIDs){
      for(int i = 0; i < _storedEvents.size(); i++){
        storedIDs[_storedEvents.at(i).eventID] = i;
      }
    };

    ModeDataPacket getModeAt(nModes_t position){
      return _storedModes.at(position);
    }

    nModes_t getNumberOfStoredModes(){return _storedModes.size();}

    EventDataPacket getEventAt(nEvents_t position){
      return _storedEvents.at(position);
    }
    nEvents_t getNumberOfStoredEvents(){return _storedEvents.size();}

    nEvents_t fillChunk(EventDataPacket (&buffer)[DataPreloadChunkSize], nEvents_t eventNumber){
      fillEventChunkCallCount++;
      eventChunkNumber = eventNumber / DataPreloadChunkSize;
      nEvents_t number = 0;
      for(int i = 0; i < DataPreloadChunkSize; i++){
        if(i + eventNumber >= _storedEvents.size()){
          EventDataPacket emptyEvent;
          eventBuffer[i] = emptyEvent;
        }
        else{
          eventBuffer[i] = _storedEvents.at(i + eventNumber);
          buffer[i] = _storedEvents.at(i + eventNumber);
          number++;
        }
      };
      return number;
    }

    nModes_t fillChunk(ModeDataPacket (&buffer)[DataPreloadChunkSize], nModes_t modeNumber){
      fillModeChunkCallCount++;
      modeChunkNumber = modeNumber / DataPreloadChunkSize;
      nModes_t number;
      for(int i = 0; i < DataPreloadChunkSize; i++){
        if(i + modeNumber >= _storedModes.size()){
          ModeDataPacket emptyMode;
          modeBuffer[i] = emptyMode;
        }
        else{
          modeBuffer[i] = _storedModes.at(i + modeNumber);
          buffer[i] = _storedModes.at(i + modeNumber);
          number++;
        }
      }
      return number;
    };
};

#endif