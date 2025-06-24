#include <unity.h>
#include <etl/map.h>

#include "NoStorageAdapter.hpp"

#include "test_readData.hpp"
#include "test_readMetadata.hpp"
#include "testHelpers.hpp"

void setUp(void){}
void tearDown(void){}

void test_writeData(){
  using namespace NoStorageTestHelpers;

  const byte testArray[2] = {1, 2};
  const byte testSize = 2;
  const auto testConfigs = convertConfigsToNoStorageVector(ConfigManagerTestObjects::goodConfigs);
  storageAddress_t testLocation = 0;

  NoStorageAdapter testClass(testConfigs, testModes, testEvents);

  // module that doesn't use storage
  TEST_ASSERT_ERROR(errorCode_t::illegalAddress, testClass.writeData(testLocation, ModuleID::oneButtonInterface, testArray, testSize), "writing to illegal address");
  TEST_ASSERT_FALSE(testClass.hasLock(ModuleID::oneButtonInterface));

  // config storage
  TEST_ASSERT_ERROR(errorCode_t::outOfBounds, testClass.writeData(testLocation, ModuleID::configStorage, testArray, testSize), "writing to configs");
  TEST_ASSERT_SUCCESS(testClass.close(ModuleID::configStorage), "closing configs");

  // modal lights
  TEST_ASSERT_ERROR(errorCode_t::outOfBounds, testClass.writeData(testLocation, ModuleID::modalLights, testArray, testSize), "writing to modal lights");
  TEST_ASSERT_SUCCESS(testClass.close(ModuleID::modalLights), "closing modal lights");

  // event manager
  TEST_ASSERT_ERROR(errorCode_t::outOfBounds, testClass.writeData(testLocation, ModuleID::eventManager, testArray, testSize), "writing to event manager");
  TEST_ASSERT_SUCCESS(testClass.close(ModuleID::eventManager), "closing event manager");
}

void test_writeMetadata(){
  using namespace NoStorageTestHelpers;

  const byte testSize = 4;
  const byte testArray[testSize] = {1, 2, 3, 4};
  const auto testConfigs = convertConfigsToNoStorageVector(ConfigManagerTestObjects::goodConfigs);
  storageAddress_t testLocation = 0;

  NoStorageAdapter testClass(testConfigs, testModes, testEvents);

  // module that doesn't use storage
  TEST_ASSERT_ERROR(errorCode_t::illegalAddress, testClass.writeMetadata(testLocation, ModuleID::deviceTime, testArray, testSize), "writing to illegal address");
  TEST_ASSERT_TRUE(testClass.hasLock(ModuleID::null));

  // config storage
  TEST_ASSERT_ERROR(errorCode_t::outOfBounds, testClass.writeMetadata(testLocation, ModuleID::configStorage, testArray, testSize), "writing to configs");
  TEST_ASSERT_SUCCESS(testClass.close(ModuleID::configStorage), "closing configs");

  // modal lights
  TEST_ASSERT_ERROR(errorCode_t::outOfBounds, testClass.writeMetadata(testLocation, ModuleID::modalLights, testArray, testSize), "writing to modal lights");
  TEST_ASSERT_SUCCESS(testClass.close(ModuleID::modalLights), "closing modal lights");

  // event manager
  TEST_ASSERT_ERROR(errorCode_t::outOfBounds, testClass.writeMetadata(testLocation, ModuleID::eventManager, testArray, testSize), "writing to event manager");
  TEST_ASSERT_SUCCESS(testClass.close(ModuleID::eventManager), "closing event manager");
}

void test_close(){
  using namespace NoStorageTestHelpers;

  const auto testConfigs = convertConfigsToNoStorageVector(ConfigManagerTestObjects::goodConfigs);

  NoStorageAdapter testClass(testConfigs, testModes, testEvents);

  TEST_ASSERT_TRUE(testClass.setAccessLock(ModuleID::eventManager));
  TEST_ASSERT_ERROR(errorCode_t::busy, testClass.setAccessLock(ModuleID::configStorage), "");
  const packetSize_t packetSize = MetadataPacketWriter::size;
  byte testArray[packetSize];
  TEST_ASSERT_ERROR(
    errorCode_t::busy,
    testClass.readMetadata(0, ModuleID::configStorage, testArray, packetSize),
    "test class should be busy"
  );

  TEST_ASSERT_SUCCESS(testClass.close(ModuleID::eventManager), "lock should release");

  TEST_ASSERT_SUCCESS(
    testClass.readMetadata(0, ModuleID::configStorage, testArray, packetSize),
    "test class should be accessible"
  );
  TEST_ASSERT_TRUE(testClass.hasLock(ModuleID::configStorage));
  TEST_ASSERT_SUCCESS(testClass.close(ModuleID::configStorage), "");

  TEST_ASSERT_TRUE(testClass.hasLock(ModuleID::null));
  TEST_ASSERT_FALSE(testClass.hasLock(ModuleID::configStorage));

  TEST_ASSERT_ERROR(errorCode_t::illegalAddress, testClass.setAccessLock(ModuleID::oneButtonInterface), "");
  TEST_ASSERT_TRUE(testClass.hasLock(ModuleID::null));
}

#include "../DeviceTime/RTCMocksAndHelpers/setTimeTestArray.h"
#include "../DeviceTime/RTCMocksAndHelpers/helpers.h"

/**
 * @brief DeviceTime will always need to be adjustable. even if the offset changes don't get properly stored, they still need to be set and used
 * 
 */
void test_DeviceTimeCanHoldNewConfigs(){
  using namespace NoStorageTestHelpers;
  // setup
  const TestTimeParamsStruct testParams = testArray.at(11);
  TEST_ASSERT_EQUAL(0, testParams.DST);
  TEST_ASSERT_EQUAL(0, testParams.timezone);
  const uint64_t utcTimestamp_uS = testParams.localTimestamp * secondsToMicros;

  using namespace ConfigManagerTestObjects;
  const TimeConfigsStruct initialConfigs{
    .timezone = testParams.timezone,
    .DST = testParams.DST
  };
  etl::vector<NoStorageConfigWrapper, maxNumberOfModules> configsVector = {
    makeNoStorageConfig(initialConfigs)
  };

  auto noStorageAdapter = std::make_shared<NoStorageAdapter>(configsVector, testModes, testEvents);
  auto configClass = std::make_shared<ConfigStorageClass>(noStorageAdapter);
  auto deviceTime = std::make_shared<DeviceTimeClass>(configClass);

  configClass->loadAllConfigs();
  deviceTime->setLocalTimestamp2000(testParams.localTimestamp, initialConfigs.timezone, initialConfigs.DST);

  // offsets should be 0, so localTimestamp should equal utcTimestamp
  TEST_ASSERT_EQUAL(testParams.localTimestamp, deviceTime->getUTCTimestampSeconds());
  
  TimeChangeFinder updateCheck(utcTimestamp_uS, utcTimestamp_uS);
  TestObserver observer;
  deviceTime->add_observer(observer);

  // DST change
  TimeConfigsStruct configs{.DST = 60*60};
  deviceTime->setUTCTimestamp2000(testParams.localTimestamp, configs.timezone, configs.DST);
  {
    TEST_ASSERT_EQUAL(configs.DST, deviceTime->getConfigs().DST);
    TEST_ASSERT_EQUAL(configs.timezone, deviceTime->getConfigs().timezone);
    
    const int64_t localTime_uS = utcTimestamp_uS + static_cast<int64_t>((configs.DST + configs.timezone) * secondsToMicros);
    const TimeUpdateStruct expectedUpdate = updateCheck.setTimes_uS(utcTimestamp_uS, localTime_uS);
    const TimeUpdateStruct actualUpdate = observer.getUpdates();
    TEST_ASSERT_EQUAL_TimeUpdateStruct_MESSAGE(
      expectedUpdate,
      actualUpdate,
      "DST change"
    );
  }

  // timezone change
  configs.timezone = -8*60*60;
  deviceTime->setUTCTimestamp2000(testParams.localTimestamp, configs.timezone, configs.DST);
  {
    TEST_ASSERT_EQUAL(configs.DST, deviceTime->getConfigs().DST);
    TEST_ASSERT_EQUAL(configs.timezone, deviceTime->getConfigs().timezone);

    const int64_t localTime_uS = utcTimestamp_uS + static_cast<int64_t>((configs.DST + configs.timezone) * secondsToMicros);
    const TimeUpdateStruct expectedUpdate = updateCheck.setTimes_uS(utcTimestamp_uS, localTime_uS);
    const TimeUpdateStruct actualUpdate = observer.getUpdates();
    TEST_ASSERT_EQUAL_TimeUpdateStruct_MESSAGE(
      expectedUpdate,
      actualUpdate,
      "timezone change"
    );
  }

  // timezone and DST change
  configs.timezone = 10*60*60;
  configs.DST = 30*60;
  deviceTime->setUTCTimestamp2000(testParams.localTimestamp, configs.timezone, configs.DST);
  {
    TEST_ASSERT_EQUAL(configs.DST, deviceTime->getConfigs().DST);
    TEST_ASSERT_EQUAL(configs.timezone, deviceTime->getConfigs().timezone);

    const int64_t localTime_uS = utcTimestamp_uS + static_cast<int64_t>((configs.DST + configs.timezone) * secondsToMicros);
    const TimeUpdateStruct expectedUpdate = updateCheck.setTimes_uS(utcTimestamp_uS, localTime_uS);
    const TimeUpdateStruct actualUpdate = observer.getUpdates();
    TEST_ASSERT_EQUAL_TimeUpdateStruct_MESSAGE(
      expectedUpdate,
      actualUpdate,
      "timezone and DST change"
    );
  }
}

void noEmbeddedUnfriendlyLibraries(){
  #ifdef __PRINT_DEBUG_H__
    TEST_ASSERT_MESSAGE(false, "did you forget to remove the print debugs?");
  #else
    TEST_ASSERT(true);
  #endif

  #ifdef _GLIBCXX_MAP
    TEST_ASSERT_MESSAGE(false, "std::map is included");
  #else
    TEST_ASSERT(true);
  #endif
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(noEmbeddedUnfriendlyLibraries);
  RUN_TEST(test_writeData);
  RUN_TEST(test_readData);
  RUN_TEST(test_readMetadata);
  RUN_TEST(test_writeMetadata);
  RUN_TEST(test_close);
  RUN_TEST(test_DeviceTimeCanHoldNewConfigs);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif