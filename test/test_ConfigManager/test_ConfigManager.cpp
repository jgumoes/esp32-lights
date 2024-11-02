#include <unity.h>

#include "ConfigManager.h"
#include "DeviceTime.h"

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
  configs.setRTCConfigs(rtcConfigs);
  RTCConfigsStruct returnedConfigs = configs.getRTCConfigs();
  uint16_t retDST = returnedConfigs.DST;
  int32_t retTimezone = returnedConfigs.timezone;
  TEST_ASSERT_EQUAL(18092, retDST);
  TEST_ASSERT_EQUAL(92384, retTimezone);
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(ConfigManagerWorksWithMockHal);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif