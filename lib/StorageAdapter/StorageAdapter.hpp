#ifndef __STORAGE_ADAPTER_HPP__
#define __STORAGE_ADAPTER_HPP__

#include <stdint.h>

#include "ProjectDefines.h"

/**
 * @brief abstraction for the config HAL. will potentially target LittleFS, EEPROM/FRAM, etc. Or not, maybe there'll be NoStorageAdapter.
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

  ModuleID currentAccessor = ModuleID::null;
  public:

    // map of reservation sizes, but not the start addresses
    const storageReservationMap_t storageReservations;

    const uint8_t checksumSize;

    IStorageAdapter(
      const storageReservationMap_t storageReservationSizes,
      const uint8_t checksumSize
    ) : checksumSize(checksumSize), storageReservations(storageReservationSizes) {};
  
    virtual ~IStorageAdapter() = default;

    /**
     * @brief write a byte array to the storage area reserved for `reservation` type. address is a reference, so that if the read-back fails, address is changed to (address of last failed byte)+1. the accessor can then re-attempt at this hopeful un-worn address. This function should add a checksum, and perform multiple write-read attempts before returning an unsuccsessful error code
     * 
     * @param address relative address within the reservation
     * @param reservation i.e. a config value would be ModuleID::configManager
     * @param dataArray pointer to the serialized data array to be read from
     * @param size size of the byte array
     * @param attempts number of write-read attempts before writeFailed error
     * @return errorCode_t
     * @retval errorCode_t::success if write was successful
     * @retval errorCode_t::writeFailed if max number of write-read attempts reached
     * @retval errorCode_t::outOfBounds if the write attempt would have exceeded the reservation limits
     * @retval errorCode_t::illegalAddress if the reservation does not exist
     */
    virtual errorCode_t writeData(storageAddress_t &address, const ModuleID reservation, const byte *dataArray, const packetSize_t size, uint8_t attempts=3) = 0;

    /**
     * @brief read a byte array in the storage area reserved for `reservation` type. performs a checksum on the data and, if fails, attempts to re-read the bytes.
     * 
     * @param address relative address
     * @param reservation i.e. a config value would be ModuleID::configManager
     * @param dataArray pointer to the serialized data array to be written to
     * @param size size of the byte array, not including checksum bytes
     * @param attempts number of read attempts before checksumFailed error
     * @return errorCode_t
     * @retval errorCode_t::success
     * @retval errorCode_t::readFailed if max number of write-read attempts reached
     * @retval errorCode_t::outOfBounds if the read attempt would have exceeded the reservation limits
     * @retval errorCode_t::illegalAddress if the reservation does not exist
     */
    virtual errorCode_t readData(const storageAddress_t address, const ModuleID reservation, byte *dataArray, const packetSize_t size, uint8_t attempts=3) = 0;

    /**
     * @brief access the metadata for a given reservation, and write to metadataArray. the first two bytes should indicate the size of the metadata array as max number of packets that can be stored. StorageHAL doesn't manage the metadata in any way. TODO: hamming code
     * 
     * @param address relative to the start of the metadata reserve
     * @param reservation 
     * @param metadataArray 
     * @param size size to read in bytes
     * @param attempts re-read attempts before failed
     * @return errorCode_t 
     * @retval errorCode_t::success
     * @retval errorCode_t::readFailed if max number of read attempts reached
     * @retval errorCode_t::outOfBounds if the read attempt would have exceeded the reservation limits
     * @retval errorCode_t::illegalAddress if the reservation does not exist
     */
    virtual errorCode_t readMetadata(const storageAddress_t address, const ModuleID reservation, byte *metadataArray, const storageAddress_t size, uint8_t attempts=3) = 0;

    /**
     * @brief writes metadata for a given reservation. address is relative to the start. it should read back the written data to make sure it matches, and perform this write-read cycle a couple of times before it returns a bad write error. 
     * 
     * @param address relative to the start of the metadata reserve
     * @param reservation 
     * @param metadataPacket 
     * @param size size to write in bytes
     * @param attempts number of write-read attempts before writeFailed error
     * @return errorCode_t
     * @retval errorCode_t::success if write was successful
     * @retval errorCode_t::writeFailed if max number of write-read attempts reached
     * @retval errorCode_t::outOfBounds if the write attempt would have exceeded the reservation limits
     * @retval errorCode_t::illegalAddress if the reservation does not exist
     */
    virtual errorCode_t writeMetadata(const storageAddress_t address, const ModuleID reservation, const byte *metadataPacket, const storageAddress_t size, uint8_t attempts=3) = 0;

    StorageReservationSizeStruct getReservationSize(const ModuleID reservation){
      auto mapIT = storageReservations.find(reservation);
      if(mapIT == storageReservations.end()){
        return StorageReservationSizeStruct{};
      }
      return mapIT->second;
    }

    /**
     * @brief close the file and reset the currentAccessor lock
     * 
     * @param accessorID 
     * @return errorCode_t 
     * @retval errorCode_t::success
     * @retval errorCode_t::busy if lock is different to accessorID
     */
    virtual errorCode_t close(const ModuleID accessorID) = 0;

    /**
     * @brief set the access lock. returns true if lock is set, false if the lock is already set by a different accessor class
     * 
     * @param accessorID 
     * @return errorCode_t
     * @retval errorCode_t::success
     * @retval errorCode_t::busy is lock is set by a different class
     * @retval errorCode_t::illegalAddress if accessorID doesn't have a reservation
     */
    errorCode_t setAccessLock(const ModuleID accessorID){
      if(!storageReservations.contains(accessorID)){
        return errorCode_t::illegalAddress;
      }
      if(
        (accessorID == currentAccessor)
        || (ModuleID::null == currentAccessor)
      ){
        currentAccessor = accessorID;
        return errorCode_t::success;
      }
      return errorCode_t::busy; 
    }
};

#endif