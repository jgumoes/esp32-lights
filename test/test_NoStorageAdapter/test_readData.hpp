#ifndef __NO_STORAGE_ADAPTER_READ_DATA_TESTS_HPP__
#define __NO_STORAGE_ADAPTER_READ_DATA_TESTS_HPP__

#include "testHelpers.hpp"

namespace NoStorageAdapter_readData_tests
{

void test_configs(){
  using namespace NoStorageTestHelpers;
  const ModuleID testAccessor = ModuleID::configStorage;
  // with stored configs
  {
    const auto testConfigs = convertConfigsToNoStorageVector(ConfigManagerTestObjects::goodConfigs);

    NoStorageAdapter testClass(testConfigs, testModes, testEvents);

    TEST_ASSERT_TRUE(testClass.hasLock(ModuleID::null));

    const auto expectedAddresses = getExpectedAddresses(testConfigs);

    for(ExpectedAddressesStruct expected : expectedAddresses){
      std::string loopMessage = "reading config for " + moduleIDToString(expected.packetID);
      const packetSize_t testArrayLength = getConfigPacketSize(expected.packetID);
      auto testArrayContainer = EmptyArray<maxConfigSize>();
      byte *testArray = testArrayContainer.array;

      TEST_ASSERT_SUCCESS(testClass.readData(expected.packetLocation, testAccessor, testArray, testArrayLength), loopMessage.c_str());

      auto expConfig = ConfigManagerTestObjects::goodConfigs.at(expected.packetID);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expConfig.data(), testArray, expConfig.packetSize, loopMessage.c_str());
    }

    TEST_ASSERT_TRUE(testClass.hasLock(testAccessor));
    TEST_ASSERT_SUCCESS(testClass.close(testAccessor), "closing config access");
  }

  // without stored configs
  {
    NoStorageAdapter testClass({}, {}, {});

    TEST_ASSERT_TRUE(testClass.hasLock(ModuleID::null));

    const auto testConfigs = convertConfigsToNoStorageVector(ConfigManagerTestObjects::goodConfigs);
    const auto expectedAddresses = getExpectedAddresses(testConfigs);

    for(ExpectedAddressesStruct expected : expectedAddresses){
      std::string loopMessage = "there shouldn't be a config for " + moduleIDToString(expected.packetID);
      const packetSize_t testArrayLength = getConfigPacketSize(expected.packetID);
      auto testArrayContainer = EmptyArray<maxConfigSize>();
      byte *testArray = testArrayContainer.array;

      TEST_ASSERT_ERROR(
        errorCode_t::outOfBounds,
        testClass.readData(expected.packetLocation, testAccessor, testArray, testArrayLength),
        loopMessage.c_str()
      );

      auto expConfig = ConfigManagerTestObjects::goodConfigs.at(expected.packetID);
      TEST_WRAPPER(testArraysAreNotEqual(expConfig.data(), testArray, expConfig.packetSize, loopMessage.c_str()));
    }

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

} // namespace NoStorageAdapter_readData_tests

void test_readData(){
  using namespace NoStorageTestHelpers;
  using namespace NoStorageAdapter_readData_tests;

  // illegal address
  {
    const auto testConfigs = convertConfigsToNoStorageVector(ConfigManagerTestObjects::goodConfigs);

    NoStorageAdapter testClass(testConfigs, testModes, testEvents);

    const auto expectedAddresses = getExpectedAddresses(testConfigs);
    const packetSize_t packetSize = 4;
    auto testArrayContainer = EmptyArray<packetSize>();
    byte *testArray = testArrayContainer.array;

    TEST_ASSERT_ERROR(
      errorCode_t::illegalAddress,
      testClass.readData(0, ModuleID::oneButtonInterface, testArray, packetSize),
      "reading from illegal reservation address"
    );

    // lock shouldn't have been set
    TEST_ASSERT_TRUE(testClass.hasLock(ModuleID::null));
  }

  // configs
  test_configs();

  test_modalLights();
  test_eventManager();
}

#endif