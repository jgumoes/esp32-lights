#ifndef __STORAGE_ADAPTER_HPP__
#define __STORAGE_ADAPTER_HPP__

#include <stdint.h>

#include "errorCodes.h"
#include "moduleIDs.h"

typedef uint16_t storageAddress_t;
// reserved storage address are at the end of the range. the first reserved address must be storageOutOfBounds, so that anything above that is still out of bounds as far as StorageHAL is concerned
const storageAddress_t storageOutOfBounds = UINT16_MAX;

typedef uint8_t packetSize_t;

/**
 * @brief abstraction for the config HAL. will potentially target esp32 Preferences library, EEPROM/FRAM, etc.
 * If EEPROM is used, checksum will also need to be used
 * If LittleFS is used, checksums are unnecessary as they are an integral part of the filesystem
 * 
 * TODO: use the same Adapter as DataStorage
 * 
 * the structure of stored configs will be:
 *    - [0]: ModuleID (config type identifier)
 *    - [1:rawDataSize]: serialized config data
 *    - [checksumBytes]: optional bytes used for checksum
 * 
 */

class IStorageAdapter{
  protected:
  // max number of metadata packets, as read from storage (0th byte of metadata array). could be junk if no configs are stored yet
  uint16_t _configMetadataMaxIndex;

  ModuleID currentAccessor = ModuleID::null;
  public:

    const storageAddress_t configReservationSize;

    const uint8_t checksumSize;

    IStorageAdapter(storageAddress_t configReservationSize, uint8_t checksumSize) : checksumSize(checksumSize), configReservationSize(configReservationSize) {};
  
    virtual ~IStorageAdapter() = default;

    /**
     * @brief write a byte array to the storage area reserved for `reservation` type. location is a reference, so that if the read-back fails, location is changed to (address of last failed byte)+1. the accessor can then re-attempt at this hopeful un-worn location. This function should add a checksum, and perform multiple write-read attempts before returning an unsuccsessful error code
     * 
     * @param location relative address within the reservation
     * @param reservation i.e. a config value would be ModuleID::configManager
     * @param dataArray pointer to the serialized data array to be read from
     * @param size size of the byte array
     * @param attempts number of write-read attempts before writeFailed error
     * @return errorCode_t writeFailed if the read data doesn't match the written data
     */
    virtual errorCode_t writeData(storageAddress_t &location, const ModuleID reservation, const byte *dataArray, const packetSize_t size, uint8_t attempts=3) = 0;

    /**
     * @brief read a byte array in the storage area reserved for `reservation` type. performs a checksum on the data and, if fails, attempts to re-read the bytes. use this method when you know what to expect at the address
     * 
     * @param location relative location
     * @param reservation i.e. a config value would be ModuleID::configManager
     * @param dataArray pointer to the serialized data array to be written to
     * @param size size of the byte array, not including checksum bytes
     * @param attempts number of read attempts before checksumFailed error
     * @return errorCode_t
     */
    errorCode_t virtual readData(const storageAddress_t location, const ModuleID reservation, byte *dataArray, const packetSize_t size, uint8_t attempts=3) = 0;

    /**
     * @brief access the metadata for a given reservation, and write to metadataArray. the first two bytes should indicate the size of the metadata array as max number of packets that can be stored. StorageHAL doesn't manage the metadata in any way. once read, should re-read the data and check for sameness
     * 
     * @param address relative to the start of the metadata reserve
     * @param reservation 
     * @param metadataArray 
     * @param size 
     * @param offset offset the address relative to the size bytes? or, +2 to the address? ignore if you don't want the size bytes
     * @param attempts re-read attempts before failed
     * @return errorCode_t 
     */
    virtual errorCode_t readMetadata(const storageAddress_t address, const ModuleID reservation, byte *metadataArray, const packetSize_t size, uint8_t attempts=3) = 0;

    /**
     * @brief writes metadata for a given reservation. address is relative to the start. it should read back the written data to make sure it matches, and perform this write-read cycle a couple of times before it returns a bad write error. 
     * 
     * @param address relative to the start of the metadata reserve
     * @param reservation 
     * @param metadataPacket 
     * @param size 
     * @param attempts number of write-read attempts before writeFailed error
     * @return errorCode_t
     * @retval errorCode_t::success if write was successful
     * @retval errorCode_t::writeFailed if max number of write-read attempts reached
     * @retval errorCode_t::outOfBounds if the write attempt would have exceeded the reservation limits
     * @retval errorCode_t::illegalAddress if the reservation does not exist
     */
    virtual errorCode_t writeMetadata(const storageAddress_t address, const ModuleID reservation, const byte *metadataPacket, const storageAddress_t size, uint8_t attempts=3) = 0;

    virtual errorCode_t writeMetadataSize(const ModuleID reservation, const byte sizeBytes[2], uint8_t attempts=3) = 0;

    virtual errorCode_t readMetadataSize(const ModuleID reservation, byte sizeBytes[2], uint8_t attempts=3) = 0;

    /**
     * @brief close the file and reset the currentAccessor lock
     * 
     * @param accessorID 
     * @return errorCode_t 
     */
    virtual errorCode_t close(const ModuleID accessorID) = 0;

    /**
     * @brief set the access lock. returns true if lock is set, false if the lock is already set by a different accessor class
     * 
     * @param accessorID 
     * @return true 
     * @return false 
     */
    bool setAccessLock(const ModuleID accessorID){
      if(ModuleID::null == currentAccessor){
        currentAccessor = accessorID;
        return true;
      }
      return (accessorID == currentAccessor) || (ModuleID::null == currentAccessor); 
    }
};

#endif