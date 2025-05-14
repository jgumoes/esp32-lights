#ifndef __STORAGE_HAL__
#define __STORAGE_HAL__

#include <Arduino.h>
#include "ProjectDefines.h"

/*
TODO: the Mode and Event ID Maps should be constructed after all the other classes have initialised. Every class should exist for the lifecycle of the program, but the maps can resize, and I want to reduce the amount of holes they leave in the heap. ModalController doesn't need to access storage until a specific mode is set from EventManager, so if EventManager can be the last class that constructs, this would defer the storage access

if EventManager can request storage to pre-load a mode, the first request can trigger building the modeID map

TODO: is there a way of seeing how much RAM is used, so that the maps can be reset if they're using more than expected (i.e. they've fragmented the heap)?
*/
class StorageHALInterface{
  protected:
    modeUUID _numberOfStoredModes;
    eventUUID _numberOfStoredEvents;

  public:
    StorageHALInterface(){};
    
    // TODO: id maps could be stored to increase boot speed
    virtual void getModeIDs(storedModeIDsMap_t& storedIDs) = 0;
    virtual void getEventIDs(storedEventIDsMap_t& storedIDs) = 0;
    
    /**
     * @brief gets mode by storage location
     * TODO: change position to address in storage
     * 
     * @param position the position of a mode in storage.
     * @param buffer the buffer to fill
     * @return true if operation was successful
     */
    virtual bool getModeAt(nModes_t position, uint8_t buffer[modePacketSize]) = 0;

    /**
     * @brief Get the Number Of Modes in storage. this doesn't include default constant brightness, so remember to +1
     * 
     * @return nModes_t 
     */
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
    // virtual nModes_t fillChunk(ModeDataPacket (&buffer)[DataPreloadChunkSize], nModes_t modeNumber) = 0;


    /* add, update, and read methods need to update the UUID maps*/
    // virtual bool addMode(data, std::map<modeUUID, nModes_t>& storedIDs) = 0;
    // virtual bool updateMode(data, std::map<modeUUID, nModes_t>& storedIDs) = 0;
    // virtual bool deleteMode(data, std::map<modeUUID, nModes_t>& storedIDs) = 0;

    // virtual bool addEvent(data, std::map<eventUUID, nEvents_t>& storedIDs) = 0;
    // virtual bool updateEvent(data, std::map<eventUUID, nEvents_t>& storedIDs) = 0;
    // virtual bool deleteEvent(data, std::map<eventUUID, nEvents_t>& storedIDs) = 0;
};

#endif