#pragma once

#include "deviceTimeTestIncludes.h"

DeviceTimeNoRTCClass deviceTimeFactory(TestParamsStruct initTimeParams = testArray.at(0)){
  OnboardTimestamp onboardTime;
  RTCConfigsStruct configsStruct = {initTimeParams.timezone, initTimeParams.DST};
  globalConfigs->setRTCConfigs(configsStruct);
  DeviceTimeNoRTCClass deviceTime(globalConfigs);
  onboardTime.setTimestamp_S(initTimeParams.localTimestamp - initTimeParams.DST - initTimeParams.timezone);
  return deviceTime;
}

// i.e. test getLocalTimestampMillis() and getLocalTimestampMicros()
void getTimestampRounding_noRTC(void){
  // setup
  OnboardTimestamp testingTimer;

  TestParamsStruct time1 = testArray.at(0); // "during_british_winter_time_2023"
  DeviceTimeNoRTCClass deviceTime = deviceTimeFactory(time1);

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

void setLocalTimestamp2000_noRTC(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeNoRTCClass deviceTime = deviceTimeFactory();
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

void setLocalTimestamp1970_noRTC(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeNoRTCClass deviceTime = deviceTimeFactory();
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

void setUTCTimestamp2000_noRTC(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeNoRTCClass deviceTime = deviceTimeFactory();

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

void setUTCTimestamp1970_noRTC(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeNoRTCClass deviceTime = deviceTimeFactory();

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

void testTimeFault_noRTC(){
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

  // DeviceTimeNoRTC has a time fault on construction
  RTCConfigsStruct configsStruct = {0, 0, secondsInDay};
  globalConfigs->setRTCConfigs(configsStruct);
  DeviceTimeNoRTCClass testClass(globalConfigs);
  TEST_ASSERT_EQUAL(true, testClass.hasTimeFault());

  // setting a good time clears the fault
  TestParamsStruct goodTime = testArray.at(12);
  testClass.setLocalTimestamp2000(goodTime.localTimestamp, goodTime.timezone, goodTime.DST);
  TEST_ASSERT_EQUAL(false, testClass.hasTimeFault());

  // timeFault is raised after max time between syncs
  onboardTime.setTimestamp_S(goodTime.localTimestamp + secondsInDay);
  TEST_ASSERT_EQUAL(goodTime.localTimestamp + secondsInDay, testClass.getLocalTimestampSeconds());
  TEST_ASSERT_EQUAL(true, testClass.hasTimeFault());

  // clear the time fault
  testClass.setLocalTimestamp2000(goodTime.localTimestamp + secondsInDay, goodTime.timezone, goodTime.DST);
  TEST_ASSERT_EQUAL(false, testClass.hasTimeFault());

  // set onboard timestamp to a bad time
  onboardTime.setTimestamp_S(faultTimeParams.localTimestamp);
  TEST_ASSERT_EQUAL(buildTimestampS, testClass.getLocalTimestampSeconds());
  TEST_ASSERT_EQUAL(true, testClass.hasTimeFault());
}

void testErrorsCatching_noRTC(void){
  // test that the timzone and DST according to BLE-SIG's absolute banger of a read, GATT Specification Supplement
  // bad timestamps and time faults already been tested in the other tests
  TestParamsStruct goodTime = testArray.at(0);
  DeviceTimeNoRTCClass testClass = deviceTimeFactory(goodTime);

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

void RUN_UNITY_TESTS_noRTC(){
  UNITY_BEGIN();
  // tests without RTC
  RUN_TEST(getTimestampRounding_noRTC);
  RUN_TEST(setLocalTimestamp2000_noRTC);
  RUN_TEST(setLocalTimestamp1970_noRTC);
  RUN_TEST(setUTCTimestamp2000_noRTC);
  RUN_TEST(setUTCTimestamp1970_noRTC);
  RUN_TEST(testTimeFault_noRTC);
  RUN_TEST(testErrorsCatching_noRTC);
  UNITY_END();
};