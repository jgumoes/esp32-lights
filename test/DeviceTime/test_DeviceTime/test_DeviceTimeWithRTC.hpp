#pragma once

#include "deviceTimeTestIncludes.h"

DeviceTimeWithRTCClass<MockWire> deviceTimeWireFactory(TestParamsStruct initTimeParams = testArray.at(0)){
  std::shared_ptr<MockWire> wire = std::make_shared<MockWire>(initTimeParams.seconds, initTimeParams.minutes, initTimeParams.hours, initTimeParams.dayOfWeek, initTimeParams.date, initTimeParams.month, initTimeParams.years);

  RTCConfigsStruct configsStruct = {initTimeParams.timezone, initTimeParams.DST};
  globalConfigs->setRTCConfigs(configsStruct);
  DeviceTimeWithRTCClass<MockWire> deviceTime(globalConfigs, wire);
  return deviceTime;
}

// i.e. test getLocalTimestampMillis() and getLocalTimestampMicros()
void getTimestampRounding_withRTC(void){
  // setup
  OnboardTimestamp testingTimer;

  TestParamsStruct time1 = testArray.at(0); // "during_british_winter_time_2023"
  DeviceTimeWithRTCClass<MockWire> deviceTime = deviceTimeWireFactory(time1);

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

void setLocalTimestamp2000_withRTC(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeWithRTCClass<MockWire> deviceTime = deviceTimeWireFactory();
  for(int i = 0; i < testArray.size(); i++)
  {
    TestParamsStruct testParams = testArray.at(i);
    TEST_ASSERT(deviceTime.setLocalTimestamp2000(testParams.localTimestamp, testParams.timezone, testParams.DST));

    // configs were written
    RTCConfigsStruct rtcConfigs = globalConfigs->getRTCConfigs();
    TEST_ASSERT_EQUAL(rtcConfigs.DST, testParams.DST);
    TEST_ASSERT_EQUAL(rtcConfigs.timezone, testParams.timezone);

    // local getters
    testLocalGetters_helper(testParams, deviceTime);
  }

  // time can't be set before build time
  {
    TestParamsStruct testParams = testArray.at(1);
    TEST_ASSERT(deviceTime.setLocalTimestamp2000(testParams.localTimestamp, testParams.timezone, testParams.DST));

    TEST_ASSERT_FALSE(deviceTime.setLocalTimestamp2000(beginningOfTime.localTimestamp, beginningOfTime.timezone, beginningOfTime.DST));
    testLocalGetters_helper(testParams, deviceTime);
  }
}

void setLocalTimestamp1970_withRTC(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeWithRTCClass<MockWire> deviceTime = deviceTimeWireFactory();
  for(int i = 0; i < testArray.size(); i++)
  {
    TestParamsStruct testParams = testArray.at(i);
    uint64_t testTimestamp = testParams.localTimestamp + SECONDS_BETWEEN_1970_2000;
    TEST_ASSERT(deviceTime.setLocalTimestamp1970(testTimestamp, testParams.timezone, testParams.DST));

    // local getters
    testLocalGetters_helper(testParams, deviceTime);

    // configs were written
    RTCConfigsStruct rtcConfigs = globalConfigs->getRTCConfigs();
    TEST_ASSERT_EQUAL(rtcConfigs.DST, testParams.DST);
    TEST_ASSERT_EQUAL(rtcConfigs.timezone, testParams.timezone);
  }

  // time can't be set before build time
  {
    TestParamsStruct testParams = testArray.at(1);
    uint64_t goodTimestamp = testParams.localTimestamp + SECONDS_BETWEEN_1970_2000;
    TEST_ASSERT(deviceTime.setLocalTimestamp1970(goodTimestamp, testParams.timezone, testParams.DST));
    TEST_ASSERT_EQUAL(testParams.localTimestamp, deviceTime.getLocalTimestampSeconds());

    TEST_ASSERT_FALSE(deviceTime.setLocalTimestamp1970(beginningOfTime.localTimestamp, beginningOfTime.timezone, beginningOfTime.DST));
    testLocalGetters_helper(testParams, deviceTime);
  }
}

void setUTCTimestamp2000_withRTC(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeWithRTCClass<MockWire> deviceTime = deviceTimeWireFactory();

  for(int i = 0; i < testArray.size(); i++)
  {
    TestParamsStruct testParams = testArray.at(i);
    uint64_t testTimestamp = testParams.localTimestamp - testParams.DST - testParams.timezone;
    TEST_ASSERT(deviceTime.setUTCTimestamp2000(testTimestamp, testParams.timezone, testParams.DST));

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
    TestParamsStruct testParams = testArray.at(1);
    uint64_t testTimestamp = testParams.localTimestamp - testParams.DST - testParams.timezone;
    TEST_ASSERT(deviceTime.setUTCTimestamp2000(testTimestamp, testParams.timezone, testParams.DST));

    TEST_ASSERT_FALSE(deviceTime.setUTCTimestamp2000(beginningOfTime.localTimestamp, beginningOfTime.timezone, beginningOfTime.DST));
    testLocalGetters_helper(testParams, deviceTime);
    TEST_ASSERT_EQUAL(testTimestamp, deviceTime.getUTCTimestampSeconds());
  }
}

void setUTCTimestamp1970_withRTC(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeWithRTCClass<MockWire> deviceTime = deviceTimeWireFactory();

  for(int i = 0; i < testArray.size(); i++)
  {
    TestParamsStruct testParams = testArray.at(i);
    uint64_t testTimestamp = testParams.localTimestamp - testParams.DST - testParams.timezone + SECONDS_BETWEEN_1970_2000;
    TEST_ASSERT(deviceTime.setUTCTimestamp1970(testTimestamp, testParams.timezone, testParams.DST));

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
    TestParamsStruct testParams = testArray.at(1);
    uint64_t testTimestamp = testParams.localTimestamp - testParams.DST - testParams.timezone + SECONDS_BETWEEN_1970_2000;
    TEST_ASSERT(deviceTime.setUTCTimestamp1970(testTimestamp, testParams.timezone, testParams.DST));
    TEST_ASSERT_EQUAL(testParams.localTimestamp, deviceTime.getLocalTimestampSeconds());

    TEST_ASSERT_FALSE(deviceTime.setUTCTimestamp1970(beginningOfTime.localTimestamp, beginningOfTime.timezone, beginningOfTime.DST));
    testLocalGetters_helper(testParams, deviceTime);
    TEST_ASSERT_EQUAL(testParams.localTimestamp, deviceTime.getLocalTimestampSeconds());
    TEST_ASSERT_EQUAL(testTimestamp - SECONDS_BETWEEN_1970_2000, deviceTime.getUTCTimestampSeconds());
  }
}

void testTimeBetweenSyncs_withRTC(void){
  TestParamsStruct initTimeParams = {"mar_1_2024", 762630313, 13, 45, 17, 5, 1, 3, 24, 0, 0};
  uint32_t secondsInADay = 60*60*24;

  std::shared_ptr<MockWire> wire = std::make_shared<MockWire>(initTimeParams.seconds, initTimeParams.minutes, initTimeParams.hours, initTimeParams.dayOfWeek, initTimeParams.date, initTimeParams.month, initTimeParams.years);
  auto configHal = std::make_unique<MockConfigHal>();
  auto mockConfigHal = makeConcreteConfigHal<MockConfigHal>();
  std::shared_ptr<ConfigManagerClass> configs = std::make_shared<ConfigManagerClass>(std::move(mockConfigHal));

  RTCConfigsStruct rtcConfigs{0, 0, secondsInADay};
  configs->setRTCConfigs(rtcConfigs);
  
  std::unique_ptr<OnboardTimestamp> onboardTimestamp = std::make_unique<OnboardTimestamp>();
  DeviceTimeWithRTCClass<MockWire> deviceTime(configs, wire);

  // check test is properly initialise
  TEST_ASSERT_EQUAL(initTimeParams.localTimestamp, deviceTime.getLocalTimestampSeconds());

  // increase time in RTCInterface by a day plus a seconds
  TestParamsStruct finalTimeParams = {"mar_2_2024", 762716713, 13, 45, 17, 6, 2, 3, 24, 0, 0};
  wire->setBufferVals(
    initTimeParams.seconds,
    initTimeParams.minutes,
    initTimeParams.hours,
    initTimeParams.dayOfWeek,
    initTimeParams.date,
    initTimeParams.month,
    initTimeParams.years,
    true
  );
  TEST_ASSERT_EQUAL(secondsInADay, configs->getRTCConfigs().maxSecondsBetweenSyncs);
  TEST_ASSERT_EQUAL(initTimeParams.localTimestamp*secondsToMicros, deviceTime.getTimeOfLastSync());
  TEST_ASSERT_EQUAL((initTimeParams.localTimestamp + secondsInADay)*secondsToMicros, deviceTime.getTimeOfNextSync());
  // increment onboardTimer by a day minus a second
  uint64_t timestamp1 = initTimeParams.localTimestamp + secondsInADay - 1;
  onboardTimestamp->setTimestamp_S(timestamp1);
  TEST_ASSERT_EQUAL(timestamp1 * secondsToMicros, onboardTimestamp->getTimestamp_uS());
  TEST_ASSERT_EQUAL(timestamp1, deviceTime.getLocalTimestampSeconds());

  // change onboardTimer to initial timestamp + secondsInADay
  wire->setBufferVals(
    finalTimeParams.seconds,
    finalTimeParams.minutes,
    finalTimeParams.hours,
    finalTimeParams.dayOfWeek,
    finalTimeParams.date,
    finalTimeParams.month,
    finalTimeParams.years,
    true
  );
  uint64_t timestamp2 = initTimeParams.localTimestamp + secondsInADay + 1;
  onboardTimestamp->setTimestamp_S(timestamp2);
  TEST_ASSERT_EQUAL_MESSAGE(1, timestamp2 - finalTimeParams.localTimestamp, "testTimeParams have gotten changed");  // just making sure
  TEST_ASSERT_NOT_EQUAL(timestamp2, deviceTime.getLocalTimestampSeconds());
  TEST_ASSERT_EQUAL(finalTimeParams.localTimestamp, deviceTime.getLocalTimestampSeconds());
}

void testTimeFault_withRTC(){
  uint64_t testTimestamp = buildTimestampS - 1;
  TestParamsStruct faultTimeParams;
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

  // RTC chip has fault on construction
  {
    onboardTime.setTimestamp_S(0);

    std::shared_ptr<MockWire> wire = std::make_shared<MockWire>(faultTimeParams.seconds, faultTimeParams.minutes, faultTimeParams.hours, faultTimeParams.dayOfWeek, faultTimeParams.date, faultTimeParams.month, faultTimeParams.years);
    auto configHal = std::make_unique<MockConfigHal>();
    auto mockConfigHal = makeConcreteConfigHal<MockConfigHal>();
    std::shared_ptr<ConfigManagerClass> configs = std::make_shared<ConfigManagerClass>(std::move(mockConfigHal));

    wire->writeHasModifiedBuffer = false;
    DeviceTimeWithRTCClass<MockWire> deviceTime(configs, wire);
    TEST_ASSERT_EQUAL(true, wire->writeHasModifiedBuffer);
    testLocalGetters_helper(buildTimeParams, deviceTime);
    TEST_ASSERT_EQUAL(true, deviceTime.hasTimeFault());
  }

  // RTC chip develops fault during runtime
  {
    TestParamsStruct testTime = testArray.at(7);
    std::shared_ptr<MockWire> wire = std::make_shared<MockWire>(testTime.seconds, testTime.minutes, testTime.hours, testTime.dayOfWeek, testTime.date, testTime.month, testTime.years);
    auto configHal = std::make_unique<MockConfigHal>();
    auto mockConfigHal = makeConcreteConfigHal<MockConfigHal>();
    std::shared_ptr<ConfigManagerClass> configs = std::make_shared<ConfigManagerClass>(std::move(mockConfigHal));

    DeviceTimeWithRTCClass<MockWire> deviceTime(configs, wire);
    testLocalGetters_helper(testTime, deviceTime);
    TEST_ASSERT_EQUAL(false, deviceTime.hasTimeFault());

    // change onboard time to a later timestamp (later than min time between syncs)
    TestParamsStruct testTime2 = testArray.at(9);
    TEST_ASSERT(testTime2.localTimestamp - testTime.localTimestamp > 60*60*24); // make sure to force a sync
    onboardTime.setTimestamp_S(testTime2.localTimestamp);

    wire->setBufferVals(faultTimeParams.seconds, faultTimeParams.minutes, faultTimeParams.hours, faultTimeParams.dayOfWeek, faultTimeParams.date, faultTimeParams.month, faultTimeParams.years);

    wire->writeHasModifiedBuffer = false;
    uint64_t actualTime = deviceTime.getLocalTimestampSeconds();
    TEST_ASSERT_EQUAL(true, deviceTime.hasTimeFault());
    testLocalGetters_helper(testTime2, deviceTime);  // should revert to onboard time
    TEST_ASSERT_EQUAL(true, wire->writeHasModifiedBuffer);  // should write the onboard time to RTC

    onboardTime.setTimestamp_S(0);
    testLocalGetters_helper(testTime2, deviceTime); // should be the same if time was written
    TEST_ASSERT_EQUAL(true, deviceTime.hasTimeFault()); // time fault shouldn't clear yet

    // set a good timestamp to clear the time fault
    TestParamsStruct goodTime = testArray.at(2);
    deviceTime.setLocalTimestamp2000(goodTime.localTimestamp, goodTime.timezone, goodTime.DST);
    testLocalGetters_helper(goodTime, deviceTime);
    TEST_ASSERT_EQUAL(false, deviceTime.hasTimeFault());
  }

  // onboard timestamp has fault shouldn't raise a time fault if RTC is good
  {
    onboardTime.setTimestamp_uS(0);

    TestParamsStruct testTime = testArray.at(7);
    std::shared_ptr<MockWire> wire = std::make_shared<MockWire>(testTime.seconds, testTime.minutes, testTime.hours, testTime.dayOfWeek, testTime.date, testTime.month, testTime.years);
    auto configHal = std::make_unique<MockConfigHal>();
    auto mockConfigHal = makeConcreteConfigHal<MockConfigHal>();
    std::shared_ptr<ConfigManagerClass> configs = std::make_shared<ConfigManagerClass>(std::move(mockConfigHal));

    DeviceTimeWithRTCClass<MockWire> deviceTime(configs, wire);
    testLocalGetters_helper(testTime, deviceTime);
    TEST_ASSERT_EQUAL(false, deviceTime.hasTimeFault());

    // reset onboard time
    onboardTime.setTimestamp_uS(0);
    testLocalGetters_helper(testTime, deviceTime);
    TEST_ASSERT_EQUAL(false, deviceTime.hasTimeFault());
  }
}

void testErrorsCatching_withRTC(void){
  // test that the timzone and DST according to BLE-SIG's absolute banger of a read, GATT Specification Supplement
  // bad timestamps and time faults already been tested in the other tests
  TestParamsStruct goodTime = testArray.at(0);
  auto testClass = deviceTimeWireFactory(goodTime);
  RTCConfigsStruct rtcConfigs = globalConfigs->getRTCConfigs();
  TEST_ASSERT_EQUAL(0, rtcConfigs.DST);
  TEST_ASSERT_EQUAL(0, rtcConfigs.timezone);

  // bad timezone
  {
    // negative out-of-bounds
    TestParamsStruct badTimezone1 = testArray.at(6);
    badTimezone1.testName = "bad timezone 1";
    badTimezone1.timezone = -49*15*60;
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badTimezone1.localTimestamp, badTimezone1.timezone, badTimezone1.DST));
    TEST_ASSERT_NOT_EQUAL(badTimezone1.localTimestamp, testClass.getLocalTimestampSeconds());
    TEST_ASSERT_NOT_EQUAL(badTimezone1.timezone, globalConfigs->getRTCConfigs().timezone);

    // positive out-of-bounds
    TestParamsStruct badTimezone2 = testArray.at(6);
    badTimezone2.testName = "bad timezone 2";
    badTimezone2.timezone = 57*15*60;
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badTimezone2.localTimestamp, badTimezone2.timezone, badTimezone2.DST));
    TEST_ASSERT_NOT_EQUAL(badTimezone2.localTimestamp, testClass.getLocalTimestampSeconds());
    TEST_ASSERT_NOT_EQUAL(badTimezone2.timezone, globalConfigs->getRTCConfigs().timezone);

    // not a multiple of 15 minutes
    TestParamsStruct badTimezone3 = testArray.at(6);
    badTimezone3.testName = "bad timezone 3";
    badTimezone3.timezone = 10*15*60 + 7;
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badTimezone3.localTimestamp, badTimezone3.timezone, badTimezone3.DST));
    TEST_ASSERT_NOT_EQUAL(badTimezone3.localTimestamp, testClass.getLocalTimestampSeconds());
    TEST_ASSERT_NOT_EQUAL(badTimezone3.timezone, globalConfigs->getRTCConfigs().timezone);
  }

  // bad DST
  {
    TestParamsStruct badDST1 = testArray.at(6);
    badDST1.testName = "bad DST 1";
    badDST1.DST = 1;
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badDST1.localTimestamp, badDST1.timezone, badDST1.DST));
    TEST_ASSERT_NOT_EQUAL(badDST1.localTimestamp, testClass.getLocalTimestampSeconds());
    TEST_ASSERT_NOT_EQUAL(badDST1.DST, globalConfigs->getRTCConfigs().DST);

    TestParamsStruct badDST2 = testArray.at(6);
    badDST2.testName = "bad DST 2";
    badDST2.DST = 59;
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badDST2.localTimestamp, badDST2.timezone, badDST2.DST));
    TEST_ASSERT_NOT_EQUAL(badDST2.localTimestamp, testClass.getLocalTimestampSeconds());
    TEST_ASSERT_NOT_EQUAL(badDST2.DST, globalConfigs->getRTCConfigs().DST);

    TestParamsStruct badDST3 = testArray.at(6);
    badDST3.testName = "bad DST 3";
    badDST3.DST = 122;
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badDST3.localTimestamp, badDST3.timezone, badDST3.DST));
    TEST_ASSERT_NOT_EQUAL(badDST3.localTimestamp, testClass.getLocalTimestampSeconds());
    TEST_ASSERT_NOT_EQUAL(badDST3.DST, globalConfigs->getRTCConfigs().DST);
  }

  // bad time
  {
    TestParamsStruct badTime1 = badTime;
    TEST_ASSERT(badTime1.localTimestamp * secondsToMicros < BUILD_TIMESTAMP);
    TEST_ASSERT_FALSE(testClass.setLocalTimestamp2000(badTime1.localTimestamp, badTime1.timezone, badTime1.DST));
    RTCConfigsStruct testConfigs = globalConfigs->getRTCConfigs();
    TEST_ASSERT_NOT_EQUAL(testConfigs.DST, badTime1.DST);
    TEST_ASSERT_NOT_EQUAL(testConfigs.timezone, badTime1.timezone);
  }
}

void RUN_UNITY_TESTS_withRTC(){
  UNITY_BEGIN();
  // tests with RTC
  RUN_TEST(getTimestampRounding_withRTC);
  RUN_TEST(setLocalTimestamp2000_withRTC);
  RUN_TEST(setLocalTimestamp1970_withRTC);
  RUN_TEST(setUTCTimestamp2000_withRTC);
  RUN_TEST(setUTCTimestamp1970_withRTC);
  RUN_TEST(testTimeFault_withRTC);
  RUN_TEST(testErrorsCatching_withRTC);
  RUN_TEST(testTimeBetweenSyncs_withRTC);
  UNITY_END();
};