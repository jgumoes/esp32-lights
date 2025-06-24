#ifndef __NO_STORAGE_ADAPTER_HPP__
#define __NO_STORAGE_ADAPTER_HPP__

#include <etl/vector.h>

#include "StorageAdapter.hpp"
#include "ConfigStorageClass.hpp"

struct NoStorageConfigWrapper{
  byte data[maxConfigSize];

  ModuleID ID(){return static_cast<ModuleID>(data[0]);}

  NoStorageConfigWrapper(const byte serializedConfig[maxConfigSize]){
    memcpy(data, serializedConfig, maxConfigSize);
  }
};

// typedef TimeConfigsStruct ConfigStructType;
/**
 * @brief serialize a config into a NoStorageConfigWrapper. doesn't perform any checks or validations
 * 
 * @tparam ConfigStructType 
 * @param configStruct 
 * @return NoStorageConfigWrapper 
 */
template <class ConfigStructType>
NoStorageConfigWrapper makeNoStorageConfig(ConfigStructType configStruct){
  byte serializedConfig[maxConfigSize];
  ConfigStructFncs::serialize(serializedConfig, configStruct);
  return NoStorageConfigWrapper(serializedConfig);
};

/**
 * @brief constructor accepts vector arrays of stored data packets, but any write attempt will be met with errorCode_T::outOfBounds
 * 
 */
class NoStorageAdapter : public IStorageAdapter{
public:
  NoStorageAdapter(
    etl::vector<NoStorageConfigWrapper, maxNumberOfModules> configs,
    etl::vector<ModeDataStruct, MAX_NUMBER_OF_MODES> modes,
    etl::vector<EventDataPacket, MAX_NUMBER_OF_EVENTS> events
  );
  ~NoStorageAdapter() = default;

  errorCode_t writeData(storageAddress_t &address, const ModuleID reservation, const byte *dataArray, const packetSize_t size, uint8_t attempts=3) override;

  errorCode_t readData(const storageAddress_t address, const ModuleID reservation, byte *dataArray, const packetSize_t size, uint8_t attempts=3) override;

  errorCode_t readMetadata(const storageAddress_t address, const ModuleID reservation, byte *metadataArray, const storageAddress_t size, uint8_t attempts=3) override;

  errorCode_t writeMetadata(const storageAddress_t address, const ModuleID reservation, const byte *metadataPacket, const storageAddress_t size, uint8_t attempts=3) override;

  errorCode_t close(const ModuleID accessorID) override;

  bool hasLock(const ModuleID accessorID){return accessorID == this->currentAccessor;}

private:

  etl::flat_map<storageAddress_t, NoStorageConfigWrapper, maxNumberOfModules> _configs;
  etl::flat_map<storageAddress_t, ModeDataStruct, MAX_NUMBER_OF_MODES> _modes;
  etl::flat_map<storageAddress_t, EventDataPacket, MAX_NUMBER_OF_EVENTS> _events;


  errorCode_t _readConfigMetadata(const storageAddress_t address, byte *metadataArray, const storageAddress_t size);
};


#endif