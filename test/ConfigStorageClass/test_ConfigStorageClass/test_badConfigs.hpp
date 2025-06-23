#ifndef __BAD_CONFIGS_TESTS_HPP__
#define __BAD_CONFIGS_TESTS_HPP__

#include <unity.h>

#include <etl/list.h>
#include <etl/map.h>

#include "ConfigStorageClass.hpp"
#include "../testObjects.hpp"
#include "../MetadataFileReader.hpp"
#include "../testConfigs.hpp"

#define ETL_CHECK_PUSH_POP

namespace ConfigStorage_badConfigsTests
{

struct ExpectedErrorStruct{
  errorCode_t expectedError;
  ConfigManagerTestObjects::GenericConfigStruct genericStruct;
  std::string message;
};

void testDeviceTimeConfigs(){
  using namespace ConfigManagerTestObjects;
  const std::string testName = "Device Time configs";
  auto storageHal = std::make_shared<FakeStorageAdapter>();
  ConfigStorageClass configClass(storageHal);
  TEST_ASSERT_EQUAL(ModuleID::configStorage, storageHal->getLock());
  GenericUser timeUser(configClass, makeGenericConfigStruct(TimeConfigsStruct{}), testName);
  configClass.loadAllConfigs();
  TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());

  timeUser.resetCounts();

  etl::vector<ExpectedErrorStruct, UINT8_MAX> errors = {
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(TimeConfigsStruct{.timezone = -49*15*60}),
      "negative out-of-bounds timezone"
    },
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(TimeConfigsStruct{.timezone = 57*15*60}),
      "positive out-of-bounds timezone"
    },
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(TimeConfigsStruct{.timezone = 0*15*60 + 7}),
      "timezone not a multiple of 15 minutes"
    },
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(TimeConfigsStruct{.DST = 1*60}),
      "bad DST 1 minte"
    },
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(TimeConfigsStruct{.DST = 59*60}),
      "bad DST 59 minutes"
    },
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(TimeConfigsStruct{.DST = 122*60}),
      "bad DST 122 minutes"
    },
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(TimeConfigsStruct{.DST = (2*60 + 30)*60}),
      "bad DST 2.5 hours"
    },
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(TimeConfigsStruct{.maxSecondsBetweenSyncs = 0}),
      "no seconds between syncs"
    },

  };

  for(ExpectedErrorStruct errorStruct : errors)
  {
    errorCode_t expectedError = errorStruct.expectedError;
    const byte *badConfig = errorStruct.genericStruct.data();
    std::string message = errorStruct.message;

    TEST_ASSERT_ERROR(expectedError, configClass.setConfig(badConfig), message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(expectedError, TimeConfigsStruct::isDataValid(badConfig), message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(0, timeUser.getNewConfigsCount(), message.c_str());
  }

}

void testModalLightsConfigs(){
  using namespace ConfigManagerTestObjects;
  const std::string testName = "Modal Lights configs";
  auto storageHal = std::make_shared<FakeStorageAdapter>();
  ConfigStorageClass configClass(storageHal);
  TEST_ASSERT_EQUAL(ModuleID::configStorage, storageHal->getLock());
  GenericUser modalUser(configClass, makeGenericConfigStruct(ModalConfigsStruct{}), testName);
  configClass.loadAllConfigs();
  TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());

  modalUser.resetCounts();

  etl::vector<ExpectedErrorStruct, UINT8_MAX> errors = {
    {
      errorCode_t::badValue,
      makeGenericConfigStruct(ModalConfigsStruct{.minOnBrightness = 0}),
      "min on brightness == 0"
    },
    {
      errorCode_t::badValue,
      makeGenericConfigStruct(ModalConfigsStruct{.softChangeWindow = 16}),
      "soft window is too large (== 16)"
    },
    {
      errorCode_t::badValue,
      makeGenericConfigStruct(ModalConfigsStruct{.softChangeWindow = 255}),
      "soft window is way too large (== 255)"
    },
    {
      errorCode_t::badValue,
      makeGenericConfigStruct(ModalConfigsStruct{.softChangeWindow = static_cast<uint8_t>((int8_t)-10)}),
      "soft window is too large (-10 which is 246)"
    },
  };

  for(ExpectedErrorStruct errorStruct : errors)
  {
    errorCode_t expectedError = errorStruct.expectedError;
    const byte *badConfig = errorStruct.genericStruct.data();
    std::string message = errorStruct.message;

    TEST_ASSERT_ERROR(expectedError, configClass.setConfig(badConfig), message.c_str());
    TEST_ASSERT_ERROR(expectedError, ModalConfigsStruct::isDataValid(badConfig), message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(0, modalUser.getNewConfigsCount(), message.c_str());
  }
}

void testOneButtonConfigs(){
  using namespace ConfigManagerTestObjects;
  const std::string testName = "One Button Interface configs";
  auto storageHal = std::make_shared<FakeStorageAdapter>();
  ConfigStorageClass configClass(storageHal);
  TEST_ASSERT_EQUAL(ModuleID::configStorage, storageHal->getLock());
  GenericUser oneButtonUser(configClass, makeGenericConfigStruct(OneButtonConfigStruct{}), testName);
  configClass.loadAllConfigs();
  TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());

  oneButtonUser.resetCounts();

  etl::vector<ExpectedErrorStruct, UINT8_MAX> errors = {
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(OneButtonConfigStruct{.timeUntilLongPress_mS = 0}),
      "timeUntilLongPress_mS == 0"
    },
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(OneButtonConfigStruct{.timeUntilLongPress_mS = 199}),
      "timeUntilLongPress_mS == 199"
    },
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(OneButtonConfigStruct{.longPressWindow_mS = 199}),
      "longPressWindow_mS == 199"
    },
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(OneButtonConfigStruct{.longPressWindow_mS = 999}),
      "longPressWindow_mS == 999"
    },
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(OneButtonConfigStruct{.longPressWindow_mS = 10001}),
      "longPressWindow_mS == 10001"
    },
  };

  for(ExpectedErrorStruct errorStruct : errors)
  {
    errorCode_t expectedError = errorStruct.expectedError;
    const byte *badConfig = errorStruct.genericStruct.data();
    std::string message = errorStruct.message;

    TEST_ASSERT_ERROR(expectedError, configClass.setConfig(badConfig), message.c_str());
    TEST_ASSERT_ERROR(expectedError, OneButtonConfigStruct::isDataValid(badConfig), message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(0, oneButtonUser.getNewConfigsCount(), message.c_str());
  }
}

void testEventManagerConfigs(){
  using namespace ConfigManagerTestObjects;
  const std::string testName = "Event Manager configs";
  auto storageHal = std::make_shared<FakeStorageAdapter>();
  ConfigStorageClass configClass(storageHal);
  TEST_ASSERT_EQUAL(ModuleID::configStorage, storageHal->getLock());
  GenericUser user(configClass, makeGenericConfigStruct(EventManagerConfigsStruct{}), testName);
  configClass.loadAllConfigs();
  TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());

  user.resetCounts();

  etl::vector<ExpectedErrorStruct, UINT8_MAX> errors = {
    {
      errorCode_t::bad_time,
      makeGenericConfigStruct(EventManagerConfigsStruct{.defaultEventWindow_S = 0}),
      "defaultEventWindow_S == 0"
    },
  };

  for(ExpectedErrorStruct errorStruct : errors){
    errorCode_t expectedError = errorStruct.expectedError;
    const byte *badConfig = errorStruct.genericStruct.data();
    std::string message = errorStruct.message;

    TEST_ASSERT_ERROR(expectedError, configClass.setConfig(badConfig), message.c_str());
    TEST_ASSERT_ERROR(expectedError, EventManagerConfigsStruct::isDataValid(badConfig), message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(0, user.getNewConfigsCount(), message.c_str());
  }
}

void run_badConfigsTests(){
  UNITY_BEGIN();
  RUN_TEST(testDeviceTimeConfigs);
  RUN_TEST(testModalLightsConfigs);
  RUN_TEST(testOneButtonConfigs);
  RUN_TEST(testEventManagerConfigs);
  UNITY_END();
}

} // namespace ConfigStorage_badConfigsTests
#endif