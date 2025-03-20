#include <unity.h>

#include "ConfigManager.h"
// #include "DeviceTime.h"

#include "../nativeMocksAndHelpers/mockConfig.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void ConfigManagerWorksWithMockHal(void){
  ConfigManagerClass configs(makeConcreteConfigHal<MockConfigHal>());

  RTCConfigsStruct rtcConfigs;
  rtcConfigs.DST = 18092;
  rtcConfigs.timezone = 92384;
  rtcConfigs.maxSecondsBetweenSyncs = 69*69*12;
  configs.setRTCConfigs(rtcConfigs);
  RTCConfigsStruct returnedConfigs = configs.getRTCConfigs();
  uint16_t retDST = returnedConfigs.DST;
  int32_t retTimezone = returnedConfigs.timezone;
  uint32_t retSecondsBetweenSyncs = returnedConfigs.maxSecondsBetweenSyncs;
  TEST_ASSERT_EQUAL(18092, retDST);
  TEST_ASSERT_EQUAL(92384, retTimezone);
  TEST_ASSERT_EQUAL(69*69*12, retSecondsBetweenSyncs);
}

void ConfigManagerEventManager(void){
  // TODO: rejects eventWindow = 0
  // TODO: uses hardwareDefaultEventWindow if configs can't be loaded
  TEST_IGNORE();
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(ConfigManagerWorksWithMockHal);
  RUN_TEST(ConfigManagerEventManager);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif