#ifndef __CONFIGMANAGER_H__
#define __CONFIGMANAGER_H__

#include <Arduino.h>
#include <memory>

#include "projectDefines.h"
#include "StorageAdapter.hpp"
#include "MetadataPacket.hpp"

/**
 * @brief use the ModuleID to get the size of the serialized data (including ID, excluding checksum). returns 0 if ModuleID doesn't have a config
 * 
 * @param configType 
 * @return packetSize_t 
 */
static packetSize_t getConfigPacketSize(ModuleID configType){
  if(
    (configType >= ModuleID::max)
    || (configType == ModuleID::null)
  ){
    return 0;
  };

  switch (configType)
  {
  case ModuleID::deviceTime:
    return TimeConfigsStruct::rawDataSize + sizeof(ModuleID);
  case ModuleID::eventManager:
    return EventManagerConfigsStruct::rawDataSize + sizeof(ModuleID);
  case ModuleID::modalLights:
    return ModalConfigsStruct::rawDataSize + sizeof(ModuleID);
  case ModuleID::oneButtonInterface:
    return OneButtonConfigStruct::rawDataSize + sizeof(ModuleID);
  default:
    return 0;
  }
};

static packetSize_t getConfigPacketSize(const uint8_t configType){
  return getConfigPacketSize(static_cast<ModuleID>(configType));
}

// size of the largest possible config data, including ID but excluding checksum
const uint8_t maxConfigSize = 11;

static packetSize_t findMaxConfigSize(){
  // TODO: include bitflag
  packetSize_t maxVal = 0;
  for(uint8_t i = 1; i < static_cast<uint8_t>(ModuleID::max); i++){
    packetSize_t size = getConfigPacketSize(static_cast<ModuleID>(i));
    if(size > maxVal){
      maxVal = size;
    }
  }
  return maxVal;
};



class MetadataMapClass{
  private:
    uint8_t _metadataSize = 0;

    etl::flat_map<ModuleID, MetadataPacketContainer, maxNumberOfModules> _metadataMap;

    MetadataPacketContainer emptyPacket;
  
  public:

    const uint8_t checksumSize;

    MetadataMapClass(uint8_t nOfChecksumBytes) : checksumSize(nOfChecksumBytes){};
  
    uint8_t getSize(){return _metadataSize;}
    void setSize(uint8_t size){_metadataSize = size;}

    /**
     * @brief Set the packet in the map. there are no checks
     * 
     * @param packetReader 
     */
    void setPacket(MetadataPacketReader packetReader){
      MetadataPacketContainer *metaRef = &_metadataMap[packetReader.ID()];
      *metaRef = MetadataPacketContainer(packetReader);
    }

    bool contains(ModuleID ID){
      return _metadataMap.contains(ID);
    }

    /**
     * @brief reads the metadata to find the next availlable config address
     * 
     * @return storageAddress_t 
     */
    storageAddress_t getNextConfigAddress(){
      if(_metadataMap.size()==0){
        return 0;
      }
      storageAddress_t address = 0;
      for(auto itPair : _metadataMap){
        MetadataPacketReader reader = itPair.second.getReader();
        if(reader.address() >= address){
          address = reader.address() + getConfigPacketSize(reader.ID()) + checksumSize;
        }
      }
      return address;
    }

    /**
     * @brief reads the metadata to find the next available metadata index
     * 
     * @return uint8_t 
     */
    uint8_t getNextIndex(){
      uint8_t index = 0;
      for(auto itPair : _metadataMap){
        MetadataPacketReader reader = itPair.second.getReader();
        if(
          (reader.metaIndex() >= index)
          && (reader.metaIndex() != 255)
        ){
          index = reader.metaIndex() + 1;
        }
      }
      return index;
    }

    /**
     * @brief Get the Config Address for the given ID. if none exists, returns storageOutOfBounds
     * 
     * @param ID 
     * @return storageAddress_t 
     */
    storageAddress_t getConfigAddress(const ModuleID ID){
      auto mapIT = _metadataMap.find(ID);

      if(_metadataMap.end() == mapIT){
        return storageOutOfBounds;
      }

      return mapIT->second.getReader().address();
    }

    /**
     * @brief construct a metadata packet and write it to storage, with wear-levelling checks. the metadata can be new or existing.
     * 
     * @param ID 
     * @param newAddress 
     * @param storage 
     * @param wearLevelAttempts 
     * @retval errorCode_t::success if write was successful, or new packet matched old packet
     * @retval errorCode_t::storage_full if metadata index exceeds array size
     */
    errorCode_t writeConfigAddress(
      const ModuleID ID,
      const storageAddress_t newAddress,
      std::shared_ptr<IStorageAdapter> storage,
      const uint8_t wearLevelAttempts = 5
    ){
      MetadataPacketContainer *packet = &_metadataMap[ID];
      MetadataPacketWriter writer(packet->packet);

      // if the packet was just created
      if(writer.ID() != ID){
        writer.setID(ID);
        writer.setMetaIndex(getNextIndex());
      }
      // elif the stored packet is already correct
      else if(writer.address() == newAddress){
        return errorCode_t::success;
      }

      writer.setAddress(newAddress);

      #ifdef native_env
      assert(wearLevelAttempts > 0);
      #endif
      errorCode_t error;
      for(uint8_t attempt = 0; attempt < wearLevelAttempts; attempt++){
        error = storage->writeMetadata(writer.metadataAddress(), ModuleID::configStorage, writer.getPacket(), writer.size);
        
        if(
          (errorCode_t::writeFailed != error)
        ){
          // if the error is not wear-related, hopefully because it's success
          return error;
        }

        // increment the metaIndex (i.e. meta address) before moving on
        writer.setMetaIndex(max(writer.metaIndex()+1, getNextIndex()));
        if(writer.metaIndex() >= _metadataSize){
          return errorCode_t::storage_full;
        }
      }

      // I don't know how you got here
      return errorCode_t::failed;
    }
};







class ConfigUser{
  public:
  const ModuleID type;

  ConfigUser(ModuleID configType) : type(configType){}
  
  virtual void newConfigs(const byte newConfig[maxConfigSize]) = 0;

  virtual packetSize_t getConfigs(byte config[maxConfigSize]) = 0;
};

struct ConfigMappingStruct{
  private:
  byte _data[maxConfigSize];
  uint8_t _packetSize = 0; // size of ModuleID + raw data
  ConfigUser *_user;
  public:
  byte* defaultConfig(){return _data;}

  // size of ModuleID + raw data
  uint8_t packetSize(){return _packetSize;}
  ConfigUser* user(){return _user;}

  void copyDefault(byte *out){
    memcpy(out, _data, _packetSize);
  }

  ConfigMappingStruct(ConfigUser *userRef, const byte defaultPacket[maxConfigSize], const uint8_t serializedConfigSize): _packetSize(serializedConfigSize), _user(userRef) {
    memcpy(_data, defaultPacket, maxConfigSize);
  };

  ConfigMappingStruct(const ConfigMappingStruct& old){
    memcpy(_data, old._data, maxConfigSize);
    _packetSize = old._packetSize;
    _user = old._user;
  }
};

/**
 * @brief ConfigStorageClass manages access to storage HAL for the configs. Config CRUD happens through the ConfigStorageClass.
 * 
 * A ConfigUser registers itself as a user, passing a this reference. ConfigStorageClass calls the setValidationFunction() method, which sets a pointer to a function that checks the config values. Since all config users will register on boot, any network attempt to set an unused config will fail.
 * 
 * When a new config is set in ConfigStorageClass, it checks the config values with the static method in the config struct, and notifies the associated user of the update. the user performs the change logic. ConfigStorageClass then attempts to commit the configs to storage.
 * 
 * TODO: config storage class needs to know what configs exist at compile time
 * 
 */
class ConfigStorageClass {
  private:
    std::shared_ptr<IStorageAdapter> _storage;
    MetadataMapClass _metadataMap;

    std::unique_ptr<ConfigStructFncs::ConfigValidatorClass> validator = std::make_unique<ConfigStructFncs::ConfigValidatorClass>();

    uint32_t _registeredConfigsBitflag = 0;

    etl::flat_map<ModuleID, ConfigMappingStruct, static_cast<uint8_t>(ModuleID::max) - 1> _storedConfigs;

    void _alertUser(ModuleID ID, const byte newConfig[maxConfigSize]){
      auto iterator = _storedConfigs.find(ID);
      if(_storedConfigs.end() == iterator){
        return;
      }
      ConfigMappingStruct *mapStruct = &iterator->second;
      mapStruct->user()->newConfigs(newConfig);
      return;
    }

    void _alertUser(const byte newConfig[maxConfigSize]){
      _alertUser(static_cast<ModuleID>(newConfig[0]), newConfig);
    }

    /**
     * @brief performs the write operations for the new config and its metadata
     * 
     * @param serializedStruct 
     * @return errorCode_t 
     */
    errorCode_t _writeConfigToStorage(const byte *serializedStruct);
    
  public:

    static const ModuleID ID = ModuleID::configStorage;
    
    ConfigStorageClass(
      std::shared_ptr<IStorageAdapter> storageAdapter
    ) : _storage(storageAdapter), _metadataMap(_storage->checksumSize) {
      // hog the storage adapter until loadAllConfigs is called
      _storage->setAccessLock(ID);
    };

    /**
     * @brief loads a config from storage and writes it to serializedOut, packetOverhead and all. result array has structure [ModuleID][serialized data][checksum bytes]. if config can't be loaded, serializedOut might be filled with junk. 
     * 
     * @param configType 
     * @param serializedOut 
     * @return errorCode_t
     */
    errorCode_t loadConfig(ModuleID configType, byte *serializedOut);

    errorCode_t getDefaultConfig(ModuleID configType, byte *serializedOut);

    /**
     * @brief Set and check a new config as a serialized byte array. if valid, alerts the registered ConfigUser of the change. first element must be ModuleID, 
     * 
     * @param serializedStruct 
     * @retval errorCode_t::IDNotInUse if ID is not in user map
     * @retval errorCode_t::storage_full if address of attempted wear-levelling has fallen out of range
     * @retval errorCode_t::wearAttemptLimitReached if attempt limit to find a new storage location has been reached
     */
    errorCode_t setConfig(const byte* serializedStruct);

    /**
     * @brief load all the configs from storage, and notify the relavent classes
     * 
     */
    void loadAllConfigs();

    /**
     * @brief sets the ConfigUser, default config, and validation function. this is only really for testing, the template version will create and serialize a default config struct of the correct type
     * 
     * @param thisRef 
     * @param defaultPacket this *must* be the default! it is very important that this is the default!
     * @param serializedConfigSize 
     * @param validation 
     */
    void registerUser(ConfigUser *thisRef, 
                      const byte defaultPacket[maxConfigSize], 
                      uint8_t serializedConfigSize,
                      ConfigStructFncs::validationFunc validation){
      // set the validation function
      validator->setValidationFunction(thisRef->type, validation);
      

      _storedConfigs.emplace(thisRef->type, thisRef, defaultPacket, serializedConfigSize);

      // set in bitflag
      _registeredConfigsBitflag |= moduleIDBit(thisRef->type);
    }

    // typedef TimeConfigsStruct ConfigStructType;
    /**
     * @brief sets: default config in map; reference to a config user (there can only be one per config type); config validation function
     * 
     * @tparam ConfigStructType 
     * @param thisRef 
     * @param configStruct 
     * @return errorCode_t 
     */
    template <class ConfigStructType>
    void registerUser(ConfigUser *thisRef, ConfigStructType &configStruct){
      ConfigStructType defaultConfig;
      packetSize_t size = defaultConfig.rawDataSize+sizeof(ModuleID);
      byte packet[maxConfigSize];
      ConfigStructFncs::serialize(packet, defaultConfig);
      
      registerUser(thisRef, packet, size, ConfigStructType::isDataValid);
    }
    
    /**
     * @brief gets the bitflag of registered config users
     * 
     * @return uint32_t 
     */
    uint32_t getRegisteredClasses(){
      uint32_t bitflag = 0;
      for(auto it : _storedConfigs){
        bitflag += moduleIDBit(it.first);
      }
      return bitflag;
    }

    /**
     * @brief This method is for DeviceTime only! DeviceTime WILL NOT be notified of changes! Time configs are a special case, as they can be set by DeviceTime (i.e. through DeviceTimeService) alongside a new timestamp. Since the user has already handled the changes, (a) it does not need to be notified, and (b) checking the new configs against device time will look like duplicates. This method lets DeviceTime handle the changes as fast as possible, and bypass the duplicity check as it should have already occured.
     * 
     * @param newConfigs 
     * @return errorCode_t 
     * @retval errorCode_t::illegalAddress if ConfigUser is not device time
     */
    errorCode_t setTimeConfigs(ConfigUser *deviceTime);
};

#endif