#ifndef __CONFIG_MANAGER_TEST_OBJECTS_HPP__
#define __CONFIG_MANAGER_TEST_OBJECTS_HPP__

#include <stdint.h>
#include <string>
#include <etl/list.h>
#include <etl/map.h>
#include <unity.h>

#include "ProjectDefines.h"
#include "ConfigStorageClass.hpp"
#include "MetadataFileReader.hpp"
#include "testConfigs.hpp"

/*
When adding new configs or modules, ctrl+f the line below
"add new configs here"
*/

#define TEST_ASSERT_SUCCESS(functionCall, message) (\
  {\
    errorCode_t _private_error_code_pls_dont_use_this_name_anywhere_else_ = functionCall;\
    TEST_ASSERT_EQUAL_MESSAGE(\
      errorCode_t::success,\
      _private_error_code_pls_dont_use_this_name_anywhere_else_,\
      addErrorToMessage(_private_error_code_pls_dont_use_this_name_anywhere_else_, message).c_str()\
    );\
  }\
)

namespace ConfigManagerTestObjects
{  
  /**
   * @brief i.e. null -> "ModuleID::null"
   * 
   * @param moduleID 
   * @return std::string 
   */
  std::string moduleIDToString(ModuleID moduleID){
    switch (moduleID)
    {
    case ModuleID::null:
      return "ModuleID::null";

    case ModuleID::deviceTime:
      return "ModuleID::deviceTime";

    case ModuleID::modalLights:
      return "ModuleID::modalLights";

    case ModuleID::eventManager:
      return "ModuleID::eventManager";

    case ModuleID::dataStorageClass:
      return "ModuleID::dataStorageClass";

    case ModuleID::configManager:
      return "ModuleID::configManager";

    /* optional modules: these may or may not be included, depending on physical implementation */

    case ModuleID::oneButtonInterface:
      return "ModuleID::oneButtonInterface";

    case ModuleID::max:
      return "ModuleID::max";
    
    /* add new Module here */
    default:
      throw("ModuleID doesn't exist: " + std::to_string(static_cast<uint16_t>(moduleID)));
      return "ModuleID doesn't exist";
    }
  }

  std::string errorCodeToString(errorCode_t error){
    switch (error)
    {
    case errorCode_t::failed:
      return "failed";
    case errorCode_t::success:
      return "success";
    case errorCode_t::badValue:
      return "badValue";
    case errorCode_t::bad_time:
      return "badTime";
    case errorCode_t::badID:
      return "badID";
    case errorCode_t::bad_uuid:
      return "badUUID";

    case errorCode_t::event_not_found:
      return "eventNotFound";
    case errorCode_t::modeNotFound:
      return "modeNotFound";
    case errorCode_t::IDNotInUse:
      return "IDNotInUse";
    case errorCode_t::notFound:
      return "notFound";

    case errorCode_t::readFailed:
      return "readFailed";
    case errorCode_t::writeFailed:
      return "writeFailed";
    case errorCode_t::wearAttemptLimitReached:
      return "wearAttemptLimitReached";
    case errorCode_t::storage_full:
      return "storageFull";
    case errorCode_t::outOfBounds:
      return "outOfBounds";
    case errorCode_t::illegalAddress:
      return "illegalAddress";

    case errorCode_t::checksumFailed:
      return "checksumFailed";
    case errorCode_t::busy:
      return "busy";
    
    
    default:
      return "unknown error code: " + std::to_string(error);
    }
  }

  std::string addErrorToMessage(errorCode_t error, std::string message){
    return message + "; error = " + errorCodeToString(error);
  }

  
  

  

  struct MetadataPacket{
    byte *rawPacket;

    static const storageAddress_t idLocation = 0;
    static const storageAddress_t addressLocation = sizeof(ModuleID);
    static const storageAddress_t indexLocation = sizeof(storageAddress_t) + addressLocation;

    static const storageAddress_t packetSize = indexLocation;
    
    ModuleID *ID = reinterpret_cast<ModuleID*>(rawPacket[0]);
    storageAddress_t *address = reinterpret_cast<storageAddress_t*>(&rawPacket[addressLocation]);
    ModuleID_t *index = reinterpret_cast<ModuleID_t*>(&rawPacket[indexLocation]);
  };
  
  typedef etl::map<ModuleID, GenericConfigStruct, 255> genConfMap_t;
  
  class FakeStorageAdapter : public IStorageAdapter{
  public:
    static const uint8_t configMetaOutOfRange = maxNumberOfModules;
    static const uint16_t metadataArraySizeBytes = configMetaOutOfRange*MetadataPacketWriter::size;
    static const storageAddress_t configOutOfRange = 512;
    
  private:
    byte _storedConfigArray[configOutOfRange];
    
    byte _metaFileSize[2] = {255, 255};
    byte _configMetadataArray[metadataArraySizeBytes] = {255, 255};
    MetadataFileReader metadataFile;
    
  public:
    
    struct {
      uint16_t writeDataCalls = 69;
      uint16_t writeMetadataCalls = 69;
    } testingVars;
    
    FakeStorageAdapter() : IStorageAdapter(126, 0), metadataFile(_metaFileSize, _configMetadataArray){};

    FakeStorageAdapter(
      genConfMap_t storedConfigsList,
      bool checkStoredAreValid,
      std::string testMessage,
      uint8_t metadataSize = maxNumberOfModules
    ) : IStorageAdapter(126, 0), metadataFile(_metaFileSize, _configMetadataArray) {
      metadataFile.setSize(metadataSize);
      std::string message = "FakeStorageAdapter: " + testMessage;
      // test the size
      {
        uint8_t compSize = ~metadataSize;
        TEST_ASSERT_EQUAL_MESSAGE(metadataSize, _metaFileSize[0], message.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(compSize, _metaFileSize[1], message.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(0, (metadataSize & compSize), message.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(UINT8_MAX, (metadataSize ^ compSize), message.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(metadataSize, metadataFile.getSize(), message.c_str());
      }

      
      storageAddress_t address = 0;
      uint8_t metaIndex = 0;
      for(auto item : storedConfigsList){
        GenericConfigStruct config = item.second;
        if(
          checkStoredAreValid
          && (config.isValid(config.data()) != errorCode_t::success)
        ){throw("config was accidently invalid");}
        if(metadataFile.getPacket(config.ID).ID() == config.ID){
          throw("duplicate initial configs");
        }

        std::string loopMessage = message + "; config = " + moduleIDToString(item.first);

        byte metaBuffer[4];
        errorCode_t error = metadataFile.setMetadataPacket(config.ID, address, metaIndex);
        TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::success, error, addErrorToMessage(error, loopMessage).c_str());
        {
          byte *packet = &_configMetadataArray[metaIndex*MetadataPacketWriter::size];
          MetadataPacketReader reader(packet);
          TEST_ASSERT_EQUAL_MESSAGE(config.ID, reader.ID(), loopMessage.c_str());
          TEST_ASSERT_EQUAL_MESSAGE(address, reader.address(), loopMessage.c_str());
          TEST_ASSERT_EQUAL_MESSAGE(metaIndex, reader.metaIndex(), loopMessage.c_str());
        }
        
        memcpy(&_storedConfigArray[address], config.data(), config.packetSize);
        // TODO: checksum
        address += config.packetSize;
        metaIndex++;
      }
    }

    uint16_t writeCount = 0;
    
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
    errorCode_t writeData(storageAddress_t &location, const ModuleID reservation, const byte *dataArray, const packetSize_t size, uint8_t attempts=3) override {
      if(!setAccessLock(reservation)){
        return errorCode_t::busy;
      }
      writeCount++;
      if(ModuleID::configManager != reservation){
        throw("wrong reservation");
      }
      setAccessLock(reservation);
      memcpy(&_storedConfigArray[location], dataArray, size);
      // TODO: checksum
      return errorCode_t::success;
    }

    uint16_t readCount = 0;
    
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
    errorCode_t readData(const storageAddress_t location, const ModuleID reservation, byte *dataArray, const packetSize_t size, uint8_t attempts=3) override {
      if(!setAccessLock(reservation)){
        return errorCode_t::busy;
      }
      readCount++;
      if(ModuleID::configManager != reservation){
        throw("wrong reservation");
      }
      setAccessLock(reservation);
      memcpy(dataArray, &_storedConfigArray[location], size);
      // TODO: checksum
      return errorCode_t::success;
    };

    uint16_t readMetadataCount = 0;
    
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
    errorCode_t readMetadata(const storageAddress_t address, const ModuleID reservation, byte *metadataArray, const packetSize_t size, uint8_t attempts=3) override {
      if(!setAccessLock(reservation)){
        return errorCode_t::busy;
      }
      readMetadataCount++;
      if(ModuleID::configManager != reservation){
        throw("wrong reservation");
      }
      setAccessLock(reservation);
      memcpy(metadataArray, _configMetadataArray, size);
      return errorCode_t::success;
    };

    uint16_t metadataWriteCount = 0;

     /**
     * @brief writes metadata for a given reservation. address is relative to the start. it should read back the written data to make sure it matches, and perform this write-read cycle a couple of times before it returns a bad write error. 
     * 
     * @param address relative to the start of the metadata reserve
     * @param reservation 
     * @param metadataPacket 
     * @param size 
     * @param offset offset the address relative to the size bytes? or, +2 to the address?
     * @param attempts number of write-read attempts before writeFailed error
     * @return errorCode_t 
     */
    errorCode_t writeMetadata(const storageAddress_t address, const ModuleID reservation, const byte *metadataPacket, const storageAddress_t size, uint8_t attempts=3) override {
      if(!setAccessLock(reservation)){
        return errorCode_t::busy;
      }
      metadataWriteCount++;
      if(ModuleID::configManager != reservation){
        throw("wrong reservation");
      }
      memcpy(&_configMetadataArray[address], metadataPacket, size);
      return errorCode_t::success;
    };

    errorCode_t writeMetadataSize(const ModuleID reservation, const byte sizeBytes[2], uint8_t attempts=3) override {
      if(!setAccessLock(reservation)){
        return errorCode_t::busy;
      }
      metadataWriteCount++;
      if(ModuleID::configManager != reservation){
        throw("wrong reservation");
      }
      memcpy(_metaFileSize, sizeBytes, 2);
      return errorCode_t::success;
    }

    errorCode_t readMetadataSize(const ModuleID reservation, byte sizeBytes[2], uint8_t attempts=3) override {
      if(!setAccessLock(reservation)){
        return errorCode_t::busy;
      }
      readMetadataCount++;
      if(ModuleID::configManager != reservation){
        throw("wrong reservation");
      }
      memcpy(sizeBytes, _metaFileSize, 2);
      return errorCode_t::success;
    }

    uint8_t closeCount = 0;

    /**
     * @brief close the file and reset the currentAccessor lock
     * 
     * @param accessorID 
     * @return errorCode_t 
     */
    errorCode_t close(const ModuleID accessorID) override {
      closeCount++;
      if(currentAccessor != accessorID){
        return errorCode_t::busy;
      }
      // TODO: modes and events
      currentAccessor = ModuleID::null;
      return errorCode_t::success;
    };


    /*================================

              Testing methods
    
    ================================== */
    ModuleID getLock(){return currentAccessor;}

    void resetCounts(){
      readCount = 0;
      readMetadataCount = 0;
      writeCount = 0;
      metadataWriteCount = 0;
    }

    uint32_t totalReads(){
      return readCount + readMetadataCount;
    }

    uint32_t totalWrites(){
      return writeCount + metadataWriteCount;
    }
    
    uint32_t totalCounts(){
      return totalReads() + totalWrites();
    }

    MetadataArrayIterator getMetadataArrayReader(){
      return MetadataArrayIterator(_configMetadataArray, metadataArraySizeBytes);
    }

    void destoryMetadata(uint8_t overwrite = 255){
      for(int i = 0; i < metadataArraySizeBytes; i++){
        _configMetadataArray[i] = overwrite;
      }
    }

    void printMetadata(std::string message = "metadata in test storage adapter"){
      #ifdef __PRINT_DEBUG_H__
      PrintDebug_UINT8_array(message, _configMetadataArray, metadataArraySizeBytes);
      #else
      throw("PrintDebug must be included");
      #endif
    }

    const byte* getConfig(ModuleID ID){
      MetadataPacketReader reader = metadataFile.getPacket(ID);
      if(reader.ID() != ID){
        throw("ID not in metadata");
      }
      return &_storedConfigArray[reader.address()];
    }
  };
  
  struct EmptyConfigPacket{
    public:
      byte packet[maxConfigSize];

      EmptyConfigPacket(ModuleID ID){
        memcpy(packet, &ID, sizeof(ModuleID));
        for(uint16_t i = sizeof(ModuleID); i < maxConfigSize; i++){
          packet[i] = 0;
        }
      }
  };
  
  class GenericUser : public ConfigUser{
    // TODO: implement folk punk
  private:
    std::string _message = "GenericUser";
    ConfigStorageClass &configClass;
    uint16_t newConfigsCount = 0;
  public:
      GenericConfigStruct configStruct;
      const std::string typeStr(){return moduleIDToString(type);};
      const std::string testMessage(){return _message + "; " + typeStr();};

      GenericUser(ConfigStorageClass &configClass, const byte configPacket[maxConfigSize], ConfigStructFncs::validationFunc validation, std::string message) : configClass(configClass), configStruct(configPacket, validation), _message("GenericUser: " + message), ConfigUser(static_cast<ModuleID>(configPacket[0]))
      {
        configClass.registerUser(this, configStruct.data(), configStruct.packetSize, configStruct.validationFunction);
      };

      GenericUser(ConfigStorageClass &configClass, const GenericConfigStruct config, std::string message) : configClass(configClass), configStruct(config), _message("GenericUser: " + message), ConfigUser(config.ID)
      {
        configClass.registerUser(this, configStruct.data(), configStruct.packetSize, configStruct.validationFunction);
      };
      
      uint16_t getNewConfigsCount(){
        return newConfigsCount;
      }

      void resetCounts(){
        newConfigsCount = 0;
      }
      
      void newConfigs(const byte newConfig[maxConfigSize]){
        newConfigsCount++;
        configStruct.newDataPacket(newConfig);
        // TEST_ASSERT_SUCCESS(configStruct.isValid(newConfig), testMessage());
      };

      packetSize_t getConfigs(byte config[maxConfigSize]){
        configStruct.copy(config);
        return configStruct.packetSize;
      }
  };

  struct NullStruct{
    ModuleID ID = ModuleID::null;
  };
  
  /**
   * @brief create mock user instances, that register themselves with the ConfigStorageClass instance
   * 
   */
  struct GenericUsersStruct{
    // TODO: more steps?
    etl::flat_map<ModuleID, GenericUser, maxNumberOfModules> userMap;

    const std::string message;
    
    GenericUsersStruct(
      ConfigStorageClass &configClass,
      std::string message
    )
      : message("GenericUsersStruct: " + message)
      {
      uint32_t expectedBitflag = 0;
      for(auto config : defaultConfigs){
        auto pairIT = userMap.emplace(config.first, configClass, config.second.data(), config.second.validationFunction, message);
        volatile GenericUser *userRef = &pairIT.first->second;
        expectedBitflag += moduleIDBit(config.first);
      }

      TEST_ASSERT_EQUAL_MESSAGE(expectedBitflag, configClass.getRegisteredClasses(), message.c_str());
    };

    packetSize_t getConfig(const ModuleID type, byte outBuffer[maxConfigSize]){
      if(!userMap.contains(type)){
        throw("type is not in the map");
      }
      return userMap.at(type).getConfigs(outBuffer);
    }
  };
} // namespace ConfigManagerTestObjects



#endif