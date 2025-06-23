#ifndef __MOCK_STORAGE_HAL_HPP__
#define __MOCK_STORAGE_HAL_HPP__

#include "ProjectDefines.h"
#include "DataStorageClass.h"
#include "../ModalLights/test_ModalLights/testModes.h"

class MockStorageHAL : public StorageHALInterface{
  private:
    modesVector _storedModes;
    eventsVector _storedEvents;

  public:
    EventDataPacket eventBuffer[DataPreloadChunkSize];
    nEvents_t eventChunkNumber;
    uint8_t fillEventChunkCallCount = 0;
        
    MockStorageHAL(
      const modesVector modeDataPackets,
      const eventsVector eventDataPackets
    // ) : _storedModes(modeDataPackets), _storedEvents(eventDataPackets){
    ){
      // ModeDataPacket emptyMode;
      for(auto mode : modeDataPackets){
        _storedModes.push_back(mode);
      }
      for(auto event : eventDataPackets){
        _storedEvents.push_back(event);
      }
      EventDataPacket emptyEvent;
      for(int i = 0; i<DataPreloadChunkSize; i++){
        // modeBuffer[i] = emptyMode;
        eventBuffer[i] = emptyEvent;
      }
    }

    /**
     * @brief fill the storedIDs map with the ID and index of the modes in the list
     * 
     * @param storedIDs 
     */
    void getModeIDs(storedModeIDsMap_t& storedIDs){
      storedIDs.clear();
      for(uint8_t i = 0; i < _storedModes.size(); i++){
        storedIDs[_storedModes.at(i).ID] = i;
      }
    };

    void getEventIDs(storedEventIDsMap_t& storedIDs){
      storedIDs.clear();
      storedIDs[1] = 0;
      for(uint8_t i = 0; i < _storedEvents.size(); i++){
        storedIDs[_storedEvents.at(i).eventID] = i;
      }
    };

    uint16_t getModeCount = 0;
    
    bool getModeAt(nModes_t position, uint8_t buffer[modePacketSize]){
      if(position >= _storedModes.size() || position < 0){
        return false;
      }
      // memcpy(buffer, _storedModes.at(position), modePacketSize);
      serializeModeDataStruct(_storedModes.at(position), buffer);
      getModeCount++;
      return true;
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

    // nModes_t fillChunk(ModeDataPacket (&buffer)[DataPreloadChunkSize], nModes_t modeNumber){
    //   fillModeChunkCallCount++;
    //   modeChunkNumber = modeNumber / DataPreloadChunkSize;
    //   nModes_t number;
    //   for(int i = 0; i < DataPreloadChunkSize; i++){
    //     if(i + modeNumber >= _storedModes.size()){
    //       ModeDataPacket emptyMode;
    //       modeBuffer[i] = emptyMode;
    //     }
    //     else{
    //       modeBuffer[i] = _storedModes.at(i + modeNumber);
    //       buffer[i] = _storedModes.at(i + modeNumber);
    //       number++;
    //     }
    //   }
    //   return number;
    // };
};

#endif