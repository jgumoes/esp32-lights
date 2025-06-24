#ifndef __NO_STORAGE_ADAPTER_TEST_HELPERS_HPP__
#define __NO_STORAGE_ADAPTER_TEST_HELPERS_HPP__

#include <unity.h>
#include <etl/map.h>

#include "NoStorageAdapter.hpp"
#include "../ConfigStorageClass/testConfigs.hpp"
#include "../nativeMocksAndHelpers/GlobalTestHelpers.hpp"
#include "../EventManager/test_EventManager/testEvents.h"
#include "../ModalLights/test_ModalLights/testModes.h"

namespace NoStorageTestHelpers
{
  
  etl::vector<NoStorageConfigWrapper, maxNumberOfModules> convertConfigsToNoStorageVector(etl::map<ModuleID, ConfigManagerTestObjects::GenericConfigStruct, 255> genericConfigsMap){
    etl::vector<NoStorageConfigWrapper, maxNumberOfModules> configVector;

    for(auto mapIT : genericConfigsMap){
      configVector.push_back(NoStorageConfigWrapper(mapIT.second.data()));
    }
    return configVector;
  }

  etl::vector<ModeDataStruct, MAX_NUMBER_OF_MODES> makeTestModes(){
    etl::vector<ModeDataStruct, MAX_NUMBER_OF_MODES> out;
    etl::vector<TestModeDataStruct, 255ULL> allTestingModes = getAllTestingModes();

    const uint8_t maxIndex = (allTestingModes.size() >= out.max_size())
                    ? out.max_size()
                    : allTestingModes.size();
    for(uint16_t index = 0; index <= maxIndex; index++){
      out.push_back(
        convertTestModeStruct(allTestingModes.at(index), TestChannels::white)
      );
    }
    return out;
  }
  
  const etl::vector<ModeDataStruct, MAX_NUMBER_OF_MODES> testModes = makeTestModes();

  const etl::vector<EventDataPacket, MAX_NUMBER_OF_EVENTS> testEvents = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent6, testEvent7, testEvent8};

  struct ExpectedAddressesStruct{
    ModuleID packetID;
    storageAddress_t packetLocation;
    storageAddress_t metadataLocation;
  };
  
  etl::vector<ExpectedAddressesStruct, maxNumberOfModules> getExpectedAddresses(etl::vector<NoStorageConfigWrapper, maxNumberOfModules> testArray){
    etl::vector<ExpectedAddressesStruct, maxNumberOfModules> out;
    storageAddress_t currentPacketLocation = 0;
    storageAddress_t currentMetaLocation = 0;
    for(NoStorageConfigWrapper config : testArray){
      out.push_back(ExpectedAddressesStruct{
        .packetID = config.ID(),
        .packetLocation = currentPacketLocation,
        .metadataLocation = currentMetaLocation
      });
      currentPacketLocation += getConfigPacketSize(config.ID());
      currentMetaLocation += MetadataPacketWriter::size;
    }
    return out;
  }
} // namespace NoStorageTestHelpers

#endif