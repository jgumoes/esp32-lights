#include <unity.h>

#include <etl/list.h>
#include <etl/map.h>

#include "ConfigStorageClass.hpp"
#include "../testObjects.hpp"
#include "../MetadataFileReader.hpp"
#include "../testConfigs.hpp"

#define ETL_CHECK_PUSH_POP

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

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
  TEST_ASSERT_EQUAL(ModuleID::configManager, storageHal->getLock());
  GenericUsersStruct genericUsers(configClass, testName);
  configClass.loadAllConfigs();
  TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());

  GenericUser timeUser(configClass, makeGenericConfigStruct(TimeConfigsStruct{}), testName);
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
  RUN_TEST(testDeviceTimeConfigs);

  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif