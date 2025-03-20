#define BUILD_TIMESTAMP 637609298000000

#include <unity.h>
// #include "../RTCMocksAndHelpers/RTCMockWire.h"
// #include "../RTCMocksAndHelpers/setTimeTestArray.h"
// #include "../../nativeMocksAndHelpers/mockConfig.h"
// #include "../RTCMocksAndHelpers/helpers.h"

// #include "DeviceTime.h"
// #include "onboardTimestamp.h"

// // #include <iostream>

// std::shared_ptr<ConfigManagerClass> globalConfigs;

#include "deviceTimeTestIncludes.h"

#include "test_DeviceTimeWithRTC.hpp"
#include "test_DeviceTimeNoRTC.hpp"

void setUp(void) {
  // set stuff up here
  globalConfigs.reset();
  auto mockConfigHal = makeConcreteConfigHal<MockConfigHal>();
  globalConfigs = std::make_shared<ConfigManagerClass>(std::move(mockConfigHal));
}

void tearDown(void) {
  // clean stuff up here
  globalConfigs.reset();
}

void makeSureBuildTimeIsAsExpected(void){
  TEST_ASSERT_EQUAL(637609298000000, BUILD_TIMESTAMP);  // otherwise, tests break
  TEST_ASSERT_EQUAL(637609298, buildTimeParams.localTimestamp);
}

// void noStrayPrintDebugs(void){
//   // arduino fake imports iostream, so this test will always fail on desktop
// #ifdef _GLIBCXX_IOSTREAM
//   TEST_ASSERT_MESSAGE(false, "you forgot to remove the print debugs");
// #else
//   TEST_ASSERT(true);
// #endif
// }

void RUN_UNITY_TESTS_common(){
  UNITY_BEGIN();
  RUN_TEST(makeSureBuildTimeIsAsExpected);
  UNITY_END();
};

// void RUN_UNITY_TESTS_withRTC(){
//   UNITY_BEGIN();
//   RUN_TEST(makeSureBuildTimeIsAsExpected);
//   // tests with RTC
//   RUN_TEST(getTimestampRounding_withRTC);
//   RUN_TEST(setLocalTimestamp2000_withRTC);
//   RUN_TEST(setLocalTimestamp1970_withRTC);
//   RUN_TEST(setUTCTimestamp2000_withRTC);
//   RUN_TEST(setUTCTimestamp1970_withRTC);
//   RUN_TEST(testTimeFault_withRTC);
//   RUN_TEST(testErrorsCatching_withRTC);
//   RUN_TEST(testTimeBetweenSyncs_withRTC);

//   // tests without RTC
//   RUN_TEST(getTimestampRounding_noRTC);
//   RUN_TEST(setLocalTimestamp2000_noRTC);
//   RUN_TEST(setLocalTimestamp1970_noRTC);
//   RUN_TEST(setUTCTimestamp2000_noRTC);
//   RUN_TEST(setUTCTimestamp1970_noRTC);
//   RUN_TEST(testTimeFault_noRTC);
//   RUN_TEST(testErrorsCatching_noRTC);
//   RUN_TEST(testTimeBetweenSyncs_noRTC);
//   UNITY_END();
// };

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS_common();
  RUN_UNITY_TESTS_withRTC();
  RUN_UNITY_TESTS_noRTC();
}
#endif