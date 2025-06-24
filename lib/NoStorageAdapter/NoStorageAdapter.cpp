#include "NoStorageAdapter.hpp"

namespace NoStorageAdapterFuncs
{
  storageAddress_t getConfigReservationSize(etl::vector<NoStorageConfigWrapper, maxNumberOfModules> configs){
    storageAddress_t size = 0;
    for(auto confIT : configs){
      size += getConfigPacketSize(confIT.ID());
    }
    return size;
  }

  /**
   * @brief find the reservation size from a vector
   * 
   * @param vectorSize 
   * @param packetSize defaults to metadata packet size
   * @return storageAddress_t 
   */
  storageAddress_t findReservationSize(
    const size_t vectorSize,
    const packetSize_t packetSize = MetadataPacketWriter::size
  ){
    const uint8_t actualVectorSize = static_cast<uint8_t>(vectorSize);
    return actualVectorSize * packetSize;
  }

  storageReservationMap_t makeStorageResMap(
    etl::vector<NoStorageConfigWrapper, maxNumberOfModules> configs,
    etl::vector<ModeDataStruct, MAX_NUMBER_OF_MODES> modes,
    etl::vector<EventDataPacket, MAX_NUMBER_OF_EVENTS> events
  ){
    return storageReservationMap_t{
      {ModuleID::configStorage, StorageReservationSizeStruct{
        .reservationSize = getConfigReservationSize(configs),
        .metadataSize = findReservationSize(configs.size())
      }},
      {ModuleID::modalLights, StorageReservationSizeStruct{
        .reservationSize = findReservationSize(modes.size(), modePacketSize),
        .metadataSize = findReservationSize(modes.size())
      }},
      {ModuleID::eventManager, StorageReservationSizeStruct{
        .reservationSize = findReservationSize(events.size(), eventPacketSize),
        .metadataSize = findReservationSize(events.size())
      }}
    };
  }
} // namespace NoStorageAdapterFuncs

NoStorageAdapter::NoStorageAdapter(
  etl::vector<NoStorageConfigWrapper, maxNumberOfModules> configs,
  etl::vector<ModeDataStruct, MAX_NUMBER_OF_MODES> modes,
  etl::vector<EventDataPacket, MAX_NUMBER_OF_EVENTS> events
)
  : IStorageAdapter(
    NoStorageAdapterFuncs::makeStorageResMap(configs, modes, events),
    0
  )
{
  storageAddress_t currentAddress = 0;
  for(NoStorageConfigWrapper config : configs){
    _configs.emplace(currentAddress, config);
    currentAddress += getConfigPacketSize(config.ID());
  }
}

errorCode_t NoStorageAdapter::writeData(storageAddress_t &address, const ModuleID reservation, const byte *dataArray, const packetSize_t size, uint8_t attempts){
  errorCode_t error = setAccessLock(reservation);
  if(errorCode_t::success != error){
    return error;
  }

  if(
    (reservation == ModuleID::configStorage)
    || (reservation == ModuleID::eventManager)
    || (reservation == ModuleID::modalLights)
  ){
    return errorCode_t::outOfBounds;
  }
  return errorCode_t::illegalAddress;
};

errorCode_t NoStorageAdapter::readData(const storageAddress_t address, const ModuleID reservation, byte *dataArray, const packetSize_t size, uint8_t attempts){
  errorCode_t error = setAccessLock(reservation);
  if(errorCode_t::success != error){
    return error;
  }

  if((address + size) > this->storageReservations.at(reservation).reservationSize){
    return errorCode_t::outOfBounds;
  }

  switch (reservation)
  {
  case ModuleID::configStorage:{
    const auto confIT = _configs.find(address);
    if(
      (confIT == _configs.end())
      || (size > maxConfigSize)
    ){
      return errorCode_t::outOfBounds;
    }
    memcpy(dataArray, confIT->second.data, size);
    return errorCode_t::success;
  }
  
  default:
    return errorCode_t::illegalAddress;
  }
  return errorCode_t::illegalAddress;
};

errorCode_t NoStorageAdapter::_readConfigMetadata(const storageAddress_t address, byte *metadataArray, const storageAddress_t size)
{
  MetadataPacketWriter writer(metadataArray);
  if(size%writer.size != 0){
    return errorCode_t::outOfBounds;
  }

  const uint16_t startIndex = address / writer.size;
  const uint16_t endIndex = (address + size) / writer.size;
  storageAddress_t currentIndex = 0;
  
  for(auto confIT : _configs){
    if(endIndex <= currentIndex){
      return errorCode_t::success;
    }

    if(currentIndex >= startIndex){
      const uint8_t writeIndex = currentIndex - startIndex;
      writer.setPacket(&metadataArray[writeIndex*writer.size]);
      writer.setID(confIT.second.ID());
      writer.setAddress(confIT.first);
      writer.setMetaIndex(currentIndex);
    }
    
    currentIndex++;
  }
  return errorCode_t::success;
}

errorCode_t NoStorageAdapter::readMetadata(const storageAddress_t address, const ModuleID reservation, byte *metadataArray, const storageAddress_t size, uint8_t attempts){
  errorCode_t error = setAccessLock(reservation);
  if(errorCode_t::success != error){
    return error;
  }

  if((address + size) > this->storageReservations.at(reservation).metadataSize){
    return errorCode_t::outOfBounds;
  }

  switch (reservation)
  {
  case ModuleID::configStorage:{
    return _readConfigMetadata(address, metadataArray, size);
  }
  
  default:
    return errorCode_t::illegalAddress;
  }
};

errorCode_t NoStorageAdapter::writeMetadata(const storageAddress_t address, const ModuleID reservation, const byte *metadataPacket, const storageAddress_t size, uint8_t attempts){
  errorCode_t error = setAccessLock(reservation);
  if(errorCode_t::success != error){
    return error;
  }

  if(
    (reservation == ModuleID::configStorage)
    || (reservation == ModuleID::eventManager)
    || (reservation == ModuleID::modalLights)
  ){
    return errorCode_t::outOfBounds;
  }
  return errorCode_t::illegalAddress;
};

errorCode_t NoStorageAdapter::close(const ModuleID accessorID){
  if(
    (currentAccessor != accessorID)
    && (currentAccessor != ModuleID::null)
  ){
    return errorCode_t::busy;
  }
  currentAccessor = ModuleID::null;
  return errorCode_t::success;
};