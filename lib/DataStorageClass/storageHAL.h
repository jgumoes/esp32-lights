#ifndef __STORAGE_HAL__
#define __STORAGE_HAL__

#include <Arduino.h>
#include "ProjectDefines.h"

class StorageHALInterface{
  protected:
    modeUUID _numberOfStoredModes;
    eventUUID _numberOfStoredEvents;

    std::map<modeUUID, nModes_t> _storedModeIDs;
    std::map<eventUUID, nEvents_t> _storedEventIDs;

  public:
    StorageHALInterface(){};
    
    virtual void getModeIDs(std::map<modeUUID, nModes_t>& storedIDs) = 0;
    virtual void getEventIDs(std::map<eventUUID, nEvents_t>& storedIDs) = 0;
    
    /**
     * @brief gets modes by order of storage.
     * it can pre-load the next few modes to save calls on the I2C/SPI bus
     * 
     * @param position the position of a mode in storage.
     * @return ModeDataPacket
     */
    virtual ModeDataPacket getModeAt(nModes_t position) = 0;
    virtual nModes_t getNumberOfStoredModes() = 0;

    /**
     * @brief gets an event stored at location "number"
     * 
     * @param position the position of a mode in storage.
     * @return EventDataPacket 
     */
    virtual EventDataPacket getEventAt(nEvents_t position) = 0;

    virtual nEvents_t getNumberOfStoredEvents() = 0;

    /**
     * @brief fills a buffer with StoredEventStructs. buffer length must be DataPreloadChunkSize. should also fill _storedEventIds map as it is called
     * 
     * @param eventNumber the event number to start with
     * @return nEvents_t: the number of elements in the buffer
     */
    virtual nEvents_t fillChunk(EventDataPacket (&buffer)[DataPreloadChunkSize], nEvents_t eventNumber) = 0;

    /**
     * @brief fills a buffer with ModeDataPacket. buffer length must be DataPreloadChunkSize. should also fill _storedModeIds map as it is called
     * 
     * @param modeNumber the mode number to start with
     * @return nModes_t the number of elements in the buffer
     */
    virtual nModes_t fillChunk(ModeDataPacket (&buffer)[DataPreloadChunkSize], nModes_t modeNumber) = 0;
};

#endif