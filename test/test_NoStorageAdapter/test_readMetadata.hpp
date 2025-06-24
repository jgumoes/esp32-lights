#ifndef __NO_STORAGE_ADAPTER_READ_METADATA_TESTS_HPP__
#define __NO_STORAGE_ADAPTER_READ_METADATA_TESTS_HPP__

#include "testHelpers.hpp"

namespace NoStorageAdapter_readMetadata_tests
{

void test_configStorage(){
  using namespace NoStorageTestHelpers;
  const ModuleID testAccessor = ModuleID::configStorage;
  // with stored configs
  {
    const auto testConfigs = convertConfigsToNoStorageVector(ConfigManagerTestObjects::goodConfigs);

    NoStorageAdapter testClass(testConfigs, testModes, testEvents);

    TEST_ASSERT_TRUE(testClass.hasLock(ModuleID::null));
    
    const packetSize_t packetSize = MetadataPacketWriter::size;
    const storageAddress_t testArraySize = maxNumberOfModules * packetSize;
    auto testArrayContainer = EmptyArray<testArraySize>();
    byte *testArray = testArrayContainer.array;

    const auto expectedAddresses = getExpectedAddresses(testConfigs);
    const storageAddress_t expectedArrayLength = packetSize*testConfigs.size();

    // read expected number of packets
    {
      const storageAddress_t readSize = expectedArrayLength;
      TEST_ASSERT_SUCCESS(testClass.readMetadata(0, testAccessor, testArray, readSize), "read expected number of configs");
      for(uint8_t i = 0; i < testConfigs.size(); i++){
        std::string loopMessage = "i = " + std::to_string(i);
        MetadataPacketReader reader(&testArray[i*packetSize]);
        const ExpectedAddressesStruct expected = expectedAddresses.at(i);

        TEST_ASSERT_EQUAL_MESSAGE(expected.packetID, reader.ID(), loopMessage.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(expected.packetLocation, reader.address(), loopMessage.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(expected.metadataLocation, reader.metadataAddress(), loopMessage.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(i, reader.metaIndex(), loopMessage.c_str());
      }
    }

    // NoStorageAdapter returns outOfBounds when read size is greater than the actual size of packets stored
    {
      const storageAddress_t readSize = testArraySize;
      TEST_ASSERT_TRUE(readSize > expectedArrayLength);
      
      TEST_ASSERT_ERROR(
        errorCode_t::outOfBounds,
        testClass.readMetadata(0, testAccessor, testArray, readSize),
        "read expected number of configs"
      );
    }
  
    // NoStorageAdapter reads the correct number of packets when read size is less than the actual size of packets stored
    {
      const storageAddress_t readSize = expectedArrayLength - packetSize;
      TEST_ASSERT_TRUE(readSize < expectedArrayLength);
      
      TEST_ASSERT_SUCCESS(testClass.readMetadata(0, testAccessor, testArray, readSize), "read expected number of configs");
      for(uint8_t i = 0; i < (testConfigs.size() - 1); i++){
        std::string loopMessage = "i = " + std::to_string(i);
        MetadataPacketReader reader(&testArray[i*packetSize]);
        const ExpectedAddressesStruct expected = expectedAddresses.at(i);
        TEST_ASSERT_EQUAL_MESSAGE(expected.packetID, reader.ID(), loopMessage.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(expected.packetLocation, reader.address(), loopMessage.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(expected.metadataLocation, reader.metadataAddress(), loopMessage.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(i, reader.metaIndex(), loopMessage.c_str());
      }
    }

    // read individual packets
    {
      
      for(ExpectedAddressesStruct expected : expectedAddresses){
        std::string loopMessage = "read individual packets; ID = " + std::to_string(static_cast<uint16_t>(expected.packetID));

        EmptyArray<packetSize> emptyPacket = EmptyArray<packetSize>();
        byte *packetArray = emptyPacket.array;
        
        TEST_ASSERT_SUCCESS(testClass.readMetadata(expected.metadataLocation, testAccessor, packetArray, packetSize), loopMessage.c_str());

        MetadataPacketReader reader(packetArray);
        TEST_ASSERT_EQUAL_MESSAGE(expected.packetID, reader.ID(), loopMessage.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(expected.packetLocation, reader.address(), loopMessage.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(expected.metadataLocation, reader.metadataAddress(), loopMessage.c_str());
      }
    }

    TEST_ASSERT_TRUE(testClass.hasLock(testAccessor));
    TEST_ASSERT_SUCCESS(testClass.close(testAccessor), "closing config access");
  }

  // without stored configs
  {
    NoStorageAdapter testClass({}, {}, {});
    const packetSize_t packetSize = MetadataPacketWriter::size;
    auto testArrayContainer = EmptyArray<packetSize>();
    byte *testArray = testArrayContainer.array;

    TEST_ASSERT_ERROR(
      errorCode_t::outOfBounds,
      testClass.readMetadata(0, testAccessor, testArray, packetSize), "reading no configs"
    );
    
    TEST_ASSERT_TRUE(testClass.hasLock(testAccessor));
    TEST_ASSERT_SUCCESS(testClass.close(testAccessor), "closing config access");
  }
}

void test_modalLights(){
  TEST_IGNORE_MESSAGE("TODO: events and modes");
}

void test_eventManager(){
  TEST_IGNORE_MESSAGE("TODO: events and modes");
}
} // namespace NoStorageAdapter_readMetadata_tests

void test_readMetadata(){
  using namespace NoStorageAdapter_readMetadata_tests;
  using namespace NoStorageTestHelpers;

  // illegal address
  {
    const auto testConfigs = convertConfigsToNoStorageVector(ConfigManagerTestObjects::goodConfigs);

    NoStorageAdapter testClass(testConfigs, testModes, testEvents);

    const packetSize_t packetSize = MetadataPacketWriter::size;
    auto testArrayContainer = EmptyArray<packetSize>();
    byte *testArray = testArrayContainer.array;

    TEST_ASSERT_ERROR(
      errorCode_t::illegalAddress,
      testClass.readMetadata(0, ModuleID::oneButtonInterface, testArray, packetSize),
      "reading from illegal reservation address"
    );

    // lock shouldn't have been set
    TEST_ASSERT_TRUE(testClass.hasLock(ModuleID::null));
  }

  test_configStorage();  

  test_modalLights();
  test_eventManager();
};

#endif