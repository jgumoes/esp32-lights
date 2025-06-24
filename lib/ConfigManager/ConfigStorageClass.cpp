#include "ConfigStorageClass.hpp"

errorCode_t ConfigStorageClass::loadConfig(ModuleID configType, byte *serializedOut){
  // check if requested config is actually registered
  if(!_storedConfigs.contains(configType)){
    return errorCode_t::notFound;
  }

  using namespace ConfigStructFncs;
  errorCode_t error = errorCode_t::failed;
  
  // if storage location in the metadata is valid
  storageAddress_t location = _metadataMap.getConfigAddress(configType);
  if(location < storageOutOfBounds){
    storageAddress_t dataSize = getConfigPacketSize(configType);
    // uint8_t dataArray[dataSize];
    error = _storage->readData(location, ID, serializedOut, dataSize);
    if(error == errorCode_t::success){
      // config.serialize(dataArray);
      return errorCode_t::success;
    }
  }

  // otherwise, try getting the default
  _storedConfigs.at(configType).copyDefault(serializedOut);
  // return validator->isValid(serializedOut); // all defaults are valid, so what should it return? notFound?
  return errorCode_t::notFound;

  // whelp, either it didn't get registered or it's not supported on this device then
  // return errorCode_t::failed;
}

errorCode_t ConfigStorageClass::getDefaultConfig(ModuleID configType, byte *serializedOut){
  auto iterator = _storedConfigs.find(configType);
  if(_storedConfigs.end() == iterator){
    return errorCode_t::badID;
  }

  iterator->second.copyDefault(serializedOut);
  return errorCode_t::success;
}

errorCode_t ConfigStorageClass::_writeConfigToStorage(const byte *serializedStruct){
  ModuleID type = static_cast<ModuleID>(serializedStruct[0]);
  // get the address to write to
  storageAddress_t address = _metadataMap.getConfigAddress(type);
  if(storageOutOfBounds == address){
    address = _metadataMap.getNextConfigAddress();
  }

  errorCode_t error;
  // attempt to write to storage
  for(uint16_t attempt = 0; attempt < 10; attempt++){
    if(address == storageOutOfBounds){
      return errorCode_t::storage_full;
    }

    error = _storage->writeData(
      address, ModuleID::configStorage, serializedStruct, getConfigPacketSize(type)
    );

    // set metadata if successful
    if(errorCode_t::success == error){
      error = _metadataMap.writeConfigAddress(type, address, _storage);
      _storage->close(ID);
      return error;
    }

    // return if error isn't due to wear
    if(errorCode_t::writeFailed != error){
      _storage->close(ID);
      return error;
    }
  }

  _storage->close(ID);
  return errorCode_t::wearAttemptLimitReached;
}

errorCode_t ConfigStorageClass::setConfig(const byte *serializedStruct){
  using namespace ConfigStructFncs;
  const ModuleID type = static_cast<ModuleID>(serializedStruct[0]);
  // TODO: some mechanism to avoid re-writing identical packets

  if(!_storedConfigs.contains(type)){
    return errorCode_t::IDNotInUse;
  }

  // does config pass validation?
  errorCode_t error = validator->isValid(serializedStruct);
  if(errorCode_t::success != error){return error;}

  // notify user
  _alertUser(type, serializedStruct);
  
  return _writeConfigToStorage(serializedStruct);
};

void ConfigStorageClass::loadAllConfigs(){
  // load metadata
  const uint16_t metadataBufferSize = max(
    maxNumberOfModules*MetadataPacketWriter::size,
    _metadataMap.getSize()
  );
  byte metadataBuffer[metadataBufferSize];
  errorCode_t error = _storage->readMetadata(0, ID, metadataBuffer, metadataBufferSize);

  // scan the metadata
  byte configBuffer[maxConfigSize];
  MetadataArrayIterator metaParser(metadataBuffer, metadataBufferSize);
  for(
    uint8_t metaIndex = 0;
    metaIndex < (metadataBufferSize/MetadataPacketWriter::size);
    metaIndex++
  ){
    // it's not excessive nesting, its actually the Chain Of Responsibility pattern, highly optimised for embedded environments
    MetadataPacketReader packetReader = metaParser.getNext();
    // is index and address valid?
    if(
      (packetReader.metaIndex() == metaIndex)
      && (packetReader.address() != storageOutOfBounds)
    ){
      // is ID in map?
      auto mapIT = _storedConfigs.find(packetReader.ID());
      if(_storedConfigs.end() != mapIT){
        error = _storage->readData(packetReader.address(), ID, configBuffer, getConfigPacketSize(packetReader.ID()));
        // can config be read?
        if(errorCode_t::success == error){
          error = validator->isValid(configBuffer);
          // is config valid?
          if(errorCode_t::success == error){
            // config and metadata packet are both good
            _metadataMap.setPacket(packetReader);
            _alertUser(configBuffer);
          }
        }
      }
    }
  }
  
  _storage->close(ID);
}

errorCode_t ConfigStorageClass::setTimeConfigs(ConfigUser *deviceTime)
{
  if(deviceTime->type != ModuleID::deviceTime){
    return errorCode_t::illegalAddress;
  }
  byte serialized[maxConfigSize];
  deviceTime->getConfigs(serialized);
  return _writeConfigToStorage(serialized);
}
