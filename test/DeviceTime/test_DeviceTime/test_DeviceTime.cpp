#define BUILD_TIMESTAMP 637609298000000

#include <unity.h>

#include "../RTCMocksAndHelpers/RTCMockWire.h"
#include "../RTCMocksAndHelpers/setTimeTestArray.h"
#include "../../nativeMocksAndHelpers/mockConfig.h"
#include "../RTCMocksAndHelpers/helpers.h"

#include "DeviceTime.h"
#include "onboardTimestamp.h"

std::shared_ptr<ConfigManagerClass> globalConfigs;

DeviceTimeClass deviceTimeFactory(TestTimeParamsStruct initTimeParams = testArray.at(0)){
  OnboardTimestamp onboardTime;
  RTCConfigsStruct configsStruct = {initTimeParams.timezone, initTimeParams.DST};
  globalConfigs->setRTCConfigs(configsStruct);
  DeviceTimeClass deviceTime(globalConfigs);
  onboardTime.setTimestamp_S(initTimeParams.localTimestamp - initTimeParams.DST - initTimeParams.timezone);
  return deviceTime;
}

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

// i.e. test getLocalTimestampMillis() and getLocalTimestampMicros()
void getTimestampRounding(void){
  // setup
  OnboardTimestamp testingTimer;

  TestTimeParamsStruct time1 = testArray.at(0); // "during_british_winter_time_2023"
  DeviceTimeClass deviceTime = deviceTimeFactory(time1);

  // gets correct time when initiated
  {
    uint64_t actualSeconds = deviceTime.getLocalTimestampSeconds();
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp, actualSeconds, time1.testName.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp * 1000, deviceTime.getLocalTimestampMillis(), time1.testName.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp * secondsToMicros, deviceTime.getLocalTimestampMicros(), time1.testName.c_str());
  }

  // rounds timestamp properly
  {
    uint64_t timestamp2 = time1.localTimestamp * secondsToMicros + 513296;
    testingTimer.setTimestamp_uS(timestamp2);
    TEST_ASSERT_EQUAL_MESSAGE(timestamp2, deviceTime.getLocalTimestampMicros(), time1.testName.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp + 1, deviceTime.getLocalTimestampSeconds(), time1.testName.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp * 1000 + 513, deviceTime.getLocalTimestampMillis(), time1.testName.c_str());

    uint64_t timestamp3 = time1.localTimestamp * secondsToMicros + 13896;
    testingTimer.setTimestamp_uS(timestamp3);
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp, deviceTime.getLocalTimestampSeconds(), time1.testName.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp * 1000 + 14, deviceTime.getLocalTimestampMillis(), time1.testName.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(timestamp3, deviceTime.getLocalTimestampMicros(), time1.testName.c_str());
  }
}

void setLocalTimestamp2000(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeClass deviceTime = deviceTimeFactory();

  TimeChangeFinder updateCheck(deviceTime.getLocalTimestampMicros(), deviceTime.getUTCTimestampMicros());
  TestObserver observer;
  deviceTime.add_observer(observer);
  
  // test setLocalTimestamp without config changes
  {
    const TestTimeParamsStruct testParams = {"july_31_25_1hr_dst_minus_8hr_timezone", 807244446, 6, 34, 2, 4, 31, 7, 25, -8*60*60, 60*60};
    TEST_ASSERT_TRUE(deviceTime.setLocalTimestamp2000(testParams.localTimestamp, testParams.timezone, testParams.DST));
    std::string message = testParams.testName;
    TEST_TIME_UPDATE_OBSERVER_MESSAGE(observer, updateCheck, testParams, message.c_str());

    const uint64_t change_S = 420;
    deviceTime.setLocalTimestamp2000(testParams.localTimestamp + 420);
    TEST_ASSERT_EQUAL(testParams.localTimestamp + change_S, deviceTime.getLocalTimestampSeconds());

    const uint64_t expUTC_S = change_S + testParams.localTimestamp - (testParams.DST + testParams.timezone);
    TEST_ASSERT_EQUAL(expUTC_S, deviceTime.getUTCTimestampSeconds());

    TimeUpdateStruct expectedUpdates = updateCheck.setTimes_S(expUTC_S, expUTC_S + testParams.DST + testParams.timezone);
    TEST_ASSERT_EQUAL_TimeUpdateStruct(expectedUpdates, observer.getUpdates());
    TEST_ASSERT_EQUAL(1, observer.getCallCountAndReset());

    RTCConfigsStruct configs = deviceTime.getConfigs();
    TEST_ASSERT_EQUAL(testParams.DST, configs.DST);
    TEST_ASSERT_EQUAL(testParams.timezone, configs.timezone);
  }
  
  // test with config changes
  for(int i = 0; i < testArray.size(); i++)
  {
    TestTimeParamsStruct testParams = testArray.at(i);
    const std::string message = "i = " + std::to_string(i) + "; " + testParams.testName;
    TEST_ASSERT(deviceTime.setLocalTimestamp2000(testParams.localTimestamp, testParams.timezone, testParams.DST));
    TEST_TIME_UPDATE_OBSERVER_MESSAGE(observer, updateCheck, testParams, message.c_str());

    // configs were written
    RTCConfigsStruct rtcConfigs = globalConfigs->getRTCConfigs();
    TEST_ASSERT_EQUAL(rtcConfigs.DST, testParams.DST);
    TEST_ASSERT_EQUAL(rtcConfigs.timezone, testParams.timezone);

    // local getters
    testLocalGetters_helper(testParams, deviceTime);
  }

  // time can't be set before build time
  {
    TestTimeParamsStruct testParams = testArray.at(1);
    // with configs
    TEST_ASSERT(deviceTime.setLocalTimestamp2000(testParams.localTimestamp, testParams.timezone, testParams.DST));
    observer.resetParams();
    
    TEST_ASSERT_FALSE(deviceTime.setLocalTimestamp2000(beginningOfTime.localTimestamp, beginningOfTime.timezone, beginningOfTime.DST));
    testLocalGetters_helper(testParams, deviceTime);
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);

    // without configs
    TEST_ASSERT_FALSE(deviceTime.setLocalTimestamp2000(beginningOfTime.localTimestamp));
    testLocalGetters_helper(testParams, deviceTime);
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);
  }
  
  // time can't be set past timestamp overflow, with configs
  {
    // at overflow
    TEST_ASSERT_FALSE(deviceTime.setLocalTimestamp2000(ONBOARD_TIMESTAMP_OVERFLOW, 0, 0));
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);

    TEST_ASSERT_TRUE_MESSAGE(ONBOARD_TIMESTAMP_OVERFLOW < (UINT64_MAX/2), "ONBOARD_TIMESTAMP_OVERFLOW is too high, this will break tests");
    // after overflow
    TEST_ASSERT_FALSE(deviceTime.setLocalTimestamp2000(ONBOARD_TIMESTAMP_OVERFLOW*2, 0, 0));
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);
  }

  // time can't be set past timestamp overflow, without configs
  {
    // at overflow
    TEST_ASSERT_FALSE(deviceTime.setLocalTimestamp2000(ONBOARD_TIMESTAMP_OVERFLOW));
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);

    // after overflow
    TEST_ASSERT_FALSE(deviceTime.setLocalTimestamp2000(ONBOARD_TIMESTAMP_OVERFLOW*2));
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);
  }
}

void setLocalTimestamp1970(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeClass deviceTime = deviceTimeFactory();

  TimeChangeFinder updateCheck(deviceTime.getLocalTimestampMicros(), deviceTime.getUTCTimestampMicros());
  TestObserver observer;
  deviceTime.add_observer(observer);

  for(int i = 0; i < testArray.size(); i++)
  {
    TestTimeParamsStruct testParams = testArray.at(i);
    uint64_t testTimestamp = testParams.localTimestamp + SECONDS_BETWEEN_1970_2000;
    TEST_ASSERT(deviceTime.setLocalTimestamp1970(testTimestamp, testParams.timezone, testParams.DST));
    TEST_TIME_UPDATE_OBSERVER(observer, updateCheck, testParams);

    // local getters
    testLocalGetters_helper(testParams, deviceTime);

    // configs were written
    RTCConfigsStruct rtcConfigs = globalConfigs->getRTCConfigs();
    TEST_ASSERT_EQUAL(rtcConfigs.DST, testParams.DST);
    TEST_ASSERT_EQUAL(rtcConfigs.timezone, testParams.timezone);
  }

  // time can't be set before build time
  {
    TestTimeParamsStruct testParams = testArray.at(1);
    uint64_t goodTimestamp = testParams.localTimestamp + SECONDS_BETWEEN_1970_2000;
    TEST_ASSERT(deviceTime.setLocalTimestamp1970(goodTimestamp, testParams.timezone, testParams.DST));
    TEST_ASSERT_EQUAL(testParams.localTimestamp, deviceTime.getLocalTimestampSeconds());

    observer.resetParams();
    TEST_ASSERT_FALSE(deviceTime.setLocalTimestamp1970(beginningOfTime.localTimestamp, beginningOfTime.timezone, beginningOfTime.DST));
    testLocalGetters_helper(testParams, deviceTime);
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);
  }

  // time can't be set past timestamp overflow
  {
    // at overflow
    TEST_ASSERT_FALSE(deviceTime.setLocalTimestamp1970(ONBOARD_TIMESTAMP_OVERFLOW, 0, 0));
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);

    // after overflow
    TEST_ASSERT_FALSE(deviceTime.setLocalTimestamp1970(ONBOARD_TIMESTAMP_OVERFLOW*2, 0, 0));
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);
  }
}

void setUTCTimestamp2000(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeClass deviceTime = deviceTimeFactory();

  TimeChangeFinder updateCheck(deviceTime.getLocalTimestampMicros(), deviceTime.getUTCTimestampMicros());
  TestObserver observer;
  deviceTime.add_observer(observer);
  
  for(int i = 0; i < testArray.size(); i++)
  {
    TestTimeParamsStruct testParams = testArray.at(i);
    uint64_t testTimestamp = testParams.localTimestamp - testParams.DST - testParams.timezone;
    TEST_ASSERT(deviceTime.setUTCTimestamp2000(testTimestamp, testParams.timezone, testParams.DST));
    TEST_TIME_UPDATE_OBSERVER(observer, updateCheck, testParams);

    // local getters
    testLocalGetters_helper(testParams, deviceTime);
    TEST_ASSERT_EQUAL(testTimestamp, deviceTime.getUTCTimestampSeconds());

    // configs were written
    RTCConfigsStruct rtcConfigs = globalConfigs->getRTCConfigs();
    TEST_ASSERT_EQUAL(rtcConfigs.DST, testParams.DST);
    TEST_ASSERT_EQUAL(rtcConfigs.timezone, testParams.timezone);
  }

  // time can't be set before build time
  {
    TestTimeParamsStruct testParams = testArray.at(1);
    uint64_t testTimestamp = testParams.localTimestamp - testParams.DST - testParams.timezone;
    TEST_ASSERT(deviceTime.setUTCTimestamp2000(testTimestamp, testParams.timezone, testParams.DST));

    observer.resetParams();
    TEST_ASSERT_FALSE(deviceTime.setUTCTimestamp2000(beginningOfTime.localTimestamp, beginningOfTime.timezone, beginningOfTime.DST));
    testLocalGetters_helper(testParams, deviceTime);
    TEST_ASSERT_EQUAL(testTimestamp, deviceTime.getUTCTimestampSeconds());
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);
  }

  // time can't be set past timestamp overflow
  {
    // at overflow
    TEST_ASSERT_FALSE(deviceTime.setUTCTimestamp2000(ONBOARD_TIMESTAMP_OVERFLOW, 0, 0));
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);

    // after overflow
    TEST_ASSERT_FALSE(deviceTime.setUTCTimestamp2000(ONBOARD_TIMESTAMP_OVERFLOW*2, 0, 0));
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);
  }
}

void setUTCTimestamp1970(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeClass deviceTime = deviceTimeFactory();

  TimeChangeFinder updateCheck(deviceTime.getLocalTimestampMicros(), deviceTime.getUTCTimestampMicros());
  TestObserver observer;
  deviceTime.add_observer(observer);

  for(int i = 0; i < testArray.size(); i++)
  {
    TestTimeParamsStruct testParams = testArray.at(i);
    uint64_t testTimestamp = testParams.localTimestamp - testParams.DST - testParams.timezone + SECONDS_BETWEEN_1970_2000;
    TEST_ASSERT(deviceTime.setUTCTimestamp1970(testTimestamp, testParams.timezone, testParams.DST));
    TEST_TIME_UPDATE_OBSERVER(observer, updateCheck, testParams);

    // local getters
    testLocalGetters_helper(testParams, deviceTime);
    TEST_ASSERT_EQUAL(testTimestamp - SECONDS_BETWEEN_1970_2000, deviceTime.getUTCTimestampSeconds());

    // configs were written
    RTCConfigsStruct rtcConfigs = globalConfigs->getRTCConfigs();
    TEST_ASSERT_EQUAL(rtcConfigs.DST, testParams.DST);
    TEST_ASSERT_EQUAL(rtcConfigs.timezone, testParams.timezone);
  }

  // time can't be set before build time
  {
    TestTimeParamsStruct testParams = testArray.at(1);
    uint64_t testTimestamp = testParams.localTimestamp - testParams.DST - testParams.timezone + SECONDS_BETWEEN_1970_2000;
    TEST_ASSERT(deviceTime.setUTCTimestamp1970(testTimestamp, testParams.timezone, testParams.DST));
    TEST_ASSERT_EQUAL(testParams.localTimestamp, deviceTime.getLocalTimestampSeconds());

    observer.resetParams();
    TEST_ASSERT_FALSE(deviceTime.setUTCTimestamp1970(beginningOfTime.localTimestamp, beginningOfTime.timezone, beginningOfTime.DST));
    testLocalGetters_helper(testParams, deviceTime);
    TEST_ASSERT_EQUAL(testParams.localTimestamp, deviceTime.getLocalTimestampSeconds());
    TEST_ASSERT_EQUAL(testTimestamp - SECONDS_BETWEEN_1970_2000, deviceTime.getUTCTimestampSeconds());
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);
  }

  // time can't be set past timestamp overflow
  {
    // at overflow
    TEST_ASSERT_FALSE(deviceTime.setUTCTimestamp1970(ONBOARD_TIMESTAMP_OVERFLOW, 0, 0));
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);

    // after overflow
    TEST_ASSERT_FALSE(deviceTime.setUTCTimestamp1970(ONBOARD_TIMESTAMP_OVERFLOW*2, 0, 0));
    TEST_ASSERT_EQUAL(0, observer.getCallCount());
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);
  }
}

void testTimeFault(){
  uint64_t testTimestamp = buildTimestampS - 1;
  TestTimeParamsStruct faultTimeParams;
  {
    faultTimeParams.testName = "Time Fault";
    faultTimeParams.localTimestamp = testTimestamp;
    faultTimeParams.seconds = 37;
    faultTimeParams.minutes = 41;
    faultTimeParams.hours = 17;
    faultTimeParams.dayOfWeek = 7;
    faultTimeParams.date = 15;
    faultTimeParams.month = 3;
    faultTimeParams.years = 20;
    faultTimeParams.timezone = 0;
    faultTimeParams.DST = 0;
  }  
  OnboardTimestamp onboardTime;


  // DeviceTimeNoRTC has a time fault on construction
  RTCConfigsStruct configsStruct = {0, 0, secondsInDay};
  globalConfigs->setRTCConfigs(configsStruct);
  DeviceTimeClass testClass(globalConfigs);
  TEST_ASSERT_EQUAL(true, testClass.hasTimeFault());

  // setting a good time clears the fault
  TestTimeParamsStruct goodTime = testArray.at(12);
  const uint64_t goodUTCTimestamp_S = utcTimeFromTestParams(goodTime);
  testClass.setLocalTimestamp2000(goodTime.localTimestamp, goodTime.timezone, goodTime.DST);
  TEST_ASSERT_EQUAL(false, testClass.hasTimeFault());
  TEST_ASSERT_EQUAL(secondsInDay*secondsToMicros, testClass.getTimeOfNextSync() - testClass.getTimeOfLastSync());

  // timeFault is raised after max time between syncs
  onboardTime.setTimestamp_S(goodUTCTimestamp_S + secondsInDay);
  TEST_ASSERT_EQUAL(goodTime.localTimestamp + secondsInDay, testClass.getLocalTimestampSeconds());
  TEST_ASSERT_EQUAL(true, testClass.hasTimeFault());

  // clear the time fault
  testClass.setLocalTimestamp2000(goodTime.localTimestamp + secondsInDay, goodTime.timezone, goodTime.DST);
  TEST_ASSERT_EQUAL(false, testClass.hasTimeFault());

  TEST_ASSERT_EQUAL(goodUTCTimestamp_S+secondsInDay, testClass.getUTCTimestampSeconds());
  
  // shortening max time between syncs gets a timefault sooner
  const uint32_t newMaxTime = secondsInDay/2;

  TestObserver observer;
  testClass.add_observer(observer);

  TEST_ASSERT_TRUE(newMaxTime < testClass.getConfigs().maxSecondsBetweenSyncs);
  const uint64_t newUTCTimestamp_S = testClass.getUTCTimestampSeconds() + newMaxTime;
  onboardTime.setTimestamp_S(newUTCTimestamp_S);
  TEST_ASSERT_EQUAL(newUTCTimestamp_S, testClass.getUTCTimestampSeconds());
  TEST_ASSERT_TRUE(newUTCTimestamp_S < testClass.getTimeOfNextSync());
  TEST_ASSERT_EQUAL(false, testClass.hasTimeFault());
  testClass.setMaxTimeBetweenSyncs(newMaxTime);
  TEST_ASSERT_EQUAL(newMaxTime*secondsToMicros, testClass.getTimeOfNextSync() - testClass.getTimeOfLastSync());
  TEST_ASSERT_EQUAL(true, testClass.hasTimeFault());
  TEST_ASSERT_EQUAL(newMaxTime, testClass.getConfigs().maxSecondsBetweenSyncs);

  // test subscribers haven't been notified
  TEST_ASSERT_EQUAL(0, observer.getCallCount());
  TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);

  // increasing max time between syncs clears the time fault
  testClass.setMaxTimeBetweenSyncs(newMaxTime + 1);
  TEST_ASSERT_EQUAL(false, testClass.hasTimeFault());
  TEST_ASSERT_EQUAL(newMaxTime + 1, testClass.getConfigs().maxSecondsBetweenSyncs);

  // test subscribers haven't been notified
  TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);

  // set onboard timestamp to a bad time
  onboardTime.setTimestamp_S(faultTimeParams.localTimestamp);
  TEST_ASSERT_EQUAL(buildTimestampS, testClass.getLocalTimestampSeconds());
  TEST_ASSERT_EQUAL(true, testClass.hasTimeFault());
}

void testErrorsCatching(void){
  // test the timzone and DST according to BLE-SIG's absolute banger of a read, GATT Specification Supplement
  // bad timestamps and time faults already been tested in the other tests
  TestTimeParamsStruct goodTime = testArray.at(0);
  DeviceTimeClass testClass = deviceTimeFactory(goodTime);

  TestObserver observer;
  testClass.add_observer(observer);
  // bad timezone
  {
    // negative out-of-bounds
    TestTimeParamsStruct badTimezone1 = testArray.at(6);
    badTimezone1.testName = "bad timezone 1";
    badTimezone1.timezone = -49*15*60;
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badTimezone1.localTimestamp, badTimezone1.timezone, badTimezone1.DST));
    TEST_ASSERT_NOT_EQUAL(badTimezone1.localTimestamp, testClass.getLocalTimestampSeconds());
    TEST_ASSERT_NOT_EQUAL(badTimezone1.timezone, globalConfigs->getRTCConfigs().timezone);
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);

    // positive out-of-bounds
    TestTimeParamsStruct badTimezone2 = testArray.at(6);
    badTimezone2.testName = "bad timezone 2";
    badTimezone2.timezone = 57*15*60;
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badTimezone2.localTimestamp, badTimezone2.timezone, badTimezone2.DST));
    TEST_ASSERT_NOT_EQUAL(badTimezone2.localTimestamp, testClass.getLocalTimestampSeconds());
    TEST_ASSERT_NOT_EQUAL(badTimezone2.timezone, globalConfigs->getRTCConfigs().timezone);
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);

    // not a multiple of 15 minutes
    TestTimeParamsStruct badTimezone3 = testArray.at(6);
    badTimezone3.testName = "bad timezone 3";
    badTimezone3.timezone = 10*15*60 + 7;
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badTimezone3.localTimestamp, badTimezone3.timezone, badTimezone3.DST));
    TEST_ASSERT_NOT_EQUAL(badTimezone3.localTimestamp, testClass.getLocalTimestampSeconds());
    TEST_ASSERT_NOT_EQUAL(badTimezone3.timezone, globalConfigs->getRTCConfigs().timezone);
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);
  }

  // bad DST
  {
    TestTimeParamsStruct badDST1 = testArray.at(6);
    badDST1.testName = "bad DST 1";
    badDST1.DST = 1;
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badDST1.localTimestamp, badDST1.timezone, badDST1.DST));
    TEST_ASSERT_NOT_EQUAL(badDST1.localTimestamp, testClass.getLocalTimestampSeconds());
    TEST_ASSERT_NOT_EQUAL(badDST1.DST, globalConfigs->getRTCConfigs().DST);
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);

    TestTimeParamsStruct badDST2 = testArray.at(6);
    badDST2.testName = "bad DST 2";
    badDST2.DST = 59;
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badDST2.localTimestamp, badDST2.timezone, badDST2.DST));
    TEST_ASSERT_NOT_EQUAL(badDST2.localTimestamp, testClass.getLocalTimestampSeconds());
    TEST_ASSERT_NOT_EQUAL(badDST2.DST, globalConfigs->getRTCConfigs().DST);
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);

    TestTimeParamsStruct badDST3 = testArray.at(6);
    badDST3.testName = "bad DST 3";
    badDST3.DST = 122;
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badDST3.localTimestamp, badDST3.timezone, badDST3.DST));
    TEST_ASSERT_NOT_EQUAL(badDST3.localTimestamp, testClass.getLocalTimestampSeconds());
    TEST_ASSERT_NOT_EQUAL(badDST3.DST, globalConfigs->getRTCConfigs().DST);
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);
  }

  // bad time
  {
    TestTimeParamsStruct badTime1 = badTime;
    TEST_ASSERT(badTime1.localTimestamp * secondsToMicros < BUILD_TIMESTAMP);
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badTime1.localTimestamp, badTime1.timezone, badTime1.DST));
    RTCConfigsStruct testConfigs = globalConfigs->getRTCConfigs();
    TEST_ASSERT_NOT_EQUAL(testConfigs.DST, badTime1.DST);
    TEST_ASSERT_NOT_EQUAL(testConfigs.timezone, badTime1.timezone);
    TEST_TIME_OBSERVER_NOT_NOTIFIED(observer);
  }
}

void moreObserverTests(){
  // test observer is notified by DST and Timezone updates
  // timestamp changes and rejected changes are already tested for in the above tests
  OnboardTimestamp testingTimer;
  DeviceTimeClass deviceTime = deviceTimeFactory();
  const uint64_t utcTimestamp_uS = deviceTime.getUTCTimestampMicros();
  TimeChangeFinder updateCheck(deviceTime.getLocalTimestampMicros(), utcTimestamp_uS);
  TestObserver observer;
  deviceTime.add_observer(observer);

  // test DST change, no UTC change
  const uint16_t newDST = 60*60;
  deviceTime.setUTCTimestamp2000(roundMicrosToSeconds(utcTimestamp_uS), 0, newDST);
  const uint64_t expLocalTime_uS = utcTimestamp_uS + newDST*secondsToMicros;
  const TimeUpdateStruct dstUpdate = updateCheck.setTimes_uS(utcTimestamp_uS, expLocalTime_uS);
  TEST_ASSERT_EQUAL(1, observer.getCallCount());
  TEST_ASSERT_EQUAL_TimeUpdateStruct(dstUpdate, observer.getUpdates());

  // test timezone change, no local change
  const int32_t newTimezone = -8*60*60;
  deviceTime.setLocalTimestamp2000(roundMicrosToSeconds(expLocalTime_uS), newTimezone, newDST);
  const uint64_t expUTCTime_uS = expLocalTime_uS - ((newDST + newTimezone)*secondsToMicros);
  const TimeUpdateStruct localUpdate = updateCheck.setTimes_uS(expUTCTime_uS, expLocalTime_uS);
  TEST_ASSERT_EQUAL(2, observer.getCallCount());
  TEST_ASSERT_EQUAL_TimeUpdateStruct(dstUpdate, observer.getUpdates());

  // TODO: test updates to just DST and timezone, without timestamp changes (not implemented yet)
}

void noPrintDebug(){
  #ifdef __PRINT_DEBUG_H__
    TEST_ASSERT_MESSAGE(false, "did you forget to remove the print debugs?");
  #else
    TEST_ASSERT(true);
  #endif
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(noPrintDebug);
  RUN_TEST(makeSureBuildTimeIsAsExpected);
  RUN_TEST(getTimestampRounding);
  RUN_TEST(setLocalTimestamp2000);
  RUN_TEST(setLocalTimestamp1970);
  RUN_TEST(setUTCTimestamp2000);
  RUN_TEST(setUTCTimestamp1970);
  RUN_TEST(testTimeFault);
  RUN_TEST(testErrorsCatching);
  UNITY_END();
};

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif