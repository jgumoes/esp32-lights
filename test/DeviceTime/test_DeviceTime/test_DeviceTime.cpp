#include <unity.h>
#include "../RTCMocksAndHelpers/RTCMockWire.h"
#include "../RTCMocksAndHelpers/setTimeTestArray.h"
#include "../../nativeMocksAndHelpers/mockConfig.h"

#include "DeviceTime.h"
#include "onboardTimestamp.h"

#include <iostream>

uint64_t findTimeInDay(TestParamsStruct& testParams){
  uint64_t timestamp = testParams.seconds;
  timestamp += testParams.minutes * 60;
  timestamp += testParams.hours * 60 * 60;
  return timestamp * 1000000;
}

uint64_t findStartOfDay(TestParamsStruct& testParams){
  uint64_t timestamp = testParams.localTimestamp * 1000000;
  timestamp -= findTimeInDay(testParams);
  return timestamp;
}

DeviceTimeClass<MockWire> deviceTimeFactory(TestParamsStruct initTimeParams = testArray.at(0)){
  std::shared_ptr<MockWire> wire = std::make_shared<MockWire>(initTimeParams.seconds, initTimeParams.minutes, initTimeParams.hours, initTimeParams.dayOfWeek, initTimeParams.date, initTimeParams.month, initTimeParams.years);
  auto configHal = std::make_unique<MockConfigHal>();
  auto mockConfigHal = makeConcreteConfigHal<MockConfigHal>();
  std::shared_ptr<ConfigManagerClass> configs = std::make_shared<ConfigManagerClass>(std::move(mockConfigHal));

  std::unique_ptr<OnboardTimestamp> onboardTimestamp = std::make_unique<OnboardTimestamp>();
  DeviceTimeClass<MockWire> deviceTime(configs, wire);
  return deviceTime;
}

template <typename WireDependancy>
void testLocalGetters_helper(TestParamsStruct& testParams, DeviceTimeClass<WireDependancy>& deviceTime){
  TEST_ASSERT_EQUAL_MESSAGE(testParams.localTimestamp, deviceTime.getSeconds(), testParams.testName.c_str());
  TEST_ASSERT_EQUAL_MESSAGE(testParams.dayOfWeek, deviceTime.getDay(), testParams.testName.c_str());
  TEST_ASSERT_EQUAL_MESSAGE(testParams.month, deviceTime.getMonth(), testParams.testName.c_str());
  TEST_ASSERT_EQUAL_MESSAGE(testParams.years, deviceTime.getYear(), testParams.testName.c_str());
  TEST_ASSERT_EQUAL_MESSAGE(testParams.date, deviceTime.getDate(), testParams.testName.c_str());
  TEST_ASSERT_EQUAL_MESSAGE(findStartOfDay(testParams), deviceTime.getStartOfDay(), testParams.testName.c_str());
  TEST_ASSERT_EQUAL_MESSAGE(findTimeInDay(testParams), deviceTime.getTimeInDay(), testParams.testName.c_str());
}

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

// i.e. test getMillis() and getMicros()
void getTimestampRounding(void){
  // setup
  OnboardTimestamp testingTimer;

  TestParamsStruct time1 = testArray.at(1); // "during_british_winter_time_2023"
  DeviceTimeClass<MockWire> deviceTime = deviceTimeFactory(time1);

  // gets correct time when initiated
  {
    uint64_t actualSeconds = deviceTime.getSeconds();
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp, actualSeconds, time1.testName.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp * 1000, deviceTime.getMillis(), time1.testName.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp * 1000000, deviceTime.getMicros(), time1.testName.c_str());
  }

  // rounds timestamp properly
  {
    uint64_t timestamp2 = time1.localTimestamp * 1000000 + 513296;
    testingTimer.setTimestamp_uS(timestamp2);
    TEST_ASSERT_EQUAL_MESSAGE(timestamp2, deviceTime.getMicros(), time1.testName.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp + 1, deviceTime.getSeconds(), time1.testName.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp * 1000 + 513, deviceTime.getMillis(), time1.testName.c_str());

    uint64_t timestamp3 = time1.localTimestamp * 1000000 + 13896;
    testingTimer.setTimestamp_uS(timestamp3);
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp, deviceTime.getSeconds(), time1.testName.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(time1.localTimestamp * 1000 + 14, deviceTime.getMillis(), time1.testName.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(timestamp3, deviceTime.getMicros(), time1.testName.c_str());
  }
}

void setLocalTimestamp2000(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeClass<MockWire> deviceTime = deviceTimeFactory();

  for(int i = 0; i < testArray.size(); i++)
  {
    TestParamsStruct testParams = testArray.at(i);
    deviceTime.setLocalTimestamp2000(testParams.localTimestamp, testParams.timezone, testParams.DST);

    // local getters
    testLocalGetters_helper<MockWire>(testParams, deviceTime);
  }
}

void setLocalTimestamp1970(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeClass<MockWire> deviceTime = deviceTimeFactory();

  for(int i = 0; i < testArray.size(); i++)
  {
    TestParamsStruct testParams = testArray.at(i);
    uint64_t testTimestamp = testParams.localTimestamp + SECONDS_BETWEEN_1970_2000;
    deviceTime.setLocalTimestamp1970(testTimestamp, testParams.timezone, testParams.DST);

    // local getters
    testLocalGetters_helper<MockWire>(testParams, deviceTime);
  }
}

void setUTCTimestamp2000(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeClass<MockWire> deviceTime = deviceTimeFactory();

  for(int i = 0; i < testArray.size(); i++)
  {
    TestParamsStruct testParams = testArray.at(i);
    uint64_t testTimestamp = testParams.localTimestamp - testParams.DST - testParams.timezone;
    deviceTime.setUTCTimestamp2000(testTimestamp, testParams.timezone, testParams.DST);

    // local getters
    testLocalGetters_helper<MockWire>(testParams, deviceTime);
    TEST_ASSERT_EQUAL(testTimestamp, deviceTime.getUTCTimestamp());
  }
}

void setUTCTimestamp1970(void){
  // setup
  OnboardTimestamp testingTimer;
  DeviceTimeClass<MockWire> deviceTime = deviceTimeFactory();

  for(int i = 0; i < testArray.size(); i++)
  {
    TestParamsStruct testParams = testArray.at(i);
    uint64_t testTimestamp = testParams.localTimestamp - testParams.DST - testParams.timezone + SECONDS_BETWEEN_1970_2000;
    deviceTime.setUTCTimestamp1970(testTimestamp, testParams.timezone, testParams.DST);

    // local getters
    testLocalGetters_helper<MockWire>(testParams, deviceTime);
    TEST_ASSERT_EQUAL(testTimestamp - SECONDS_BETWEEN_1970_2000, deviceTime.getUTCTimestamp());
  }
}

void testTimeBetweenSyncs(void){
  TestParamsStruct initTimeParams = {"mar_1_2024", 762630313, 13, 45, 17, 5, 1, 3, 24, 0, 0};
  uint64_t secondsInADay = 60*60*24;

  std::shared_ptr<MockWire> wire = std::make_shared<MockWire>(initTimeParams.seconds, initTimeParams.minutes, initTimeParams.hours, initTimeParams.dayOfWeek, initTimeParams.date, initTimeParams.month, initTimeParams.years);
  auto configHal = std::make_unique<MockConfigHal>();
  auto mockConfigHal = makeConcreteConfigHal<MockConfigHal>();
  std::shared_ptr<ConfigManagerClass> configs = std::make_shared<ConfigManagerClass>(std::move(mockConfigHal));

  std::unique_ptr<OnboardTimestamp> onboardTimestamp = std::make_unique<OnboardTimestamp>();
  DeviceTimeClass<MockWire> deviceTime(configs, wire, secondsInADay);

  // check test is properly initialise
  TEST_ASSERT_EQUAL(initTimeParams.localTimestamp, deviceTime.getSeconds());

  // increase time in RTCInterface by a day plus a seconds
  TestParamsStruct finalTimeParams = {"mar_2_2024", 762716714, 14, 45, 17, 6, 2, 3, 24, 0, 0};
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
  
  // increment onboardTimer by a day minus a second
  uint64_t timestamp1 = initTimeParams.localTimestamp + secondsInADay - 1;
  onboardTimestamp->setTimestamp_S(timestamp1);
  TEST_ASSERT_EQUAL(timestamp1, deviceTime.getSeconds());

  // change onboardTimer to initial timestamp + secondsInADay
  uint64_t timestamp2 = initTimeParams.localTimestamp + secondsInADay;
  TEST_ASSERT_NOT_EQUAL(timestamp2, deviceTime.getSeconds());
  TEST_ASSERT_EQUAL(finalTimeParams.localTimestamp, deviceTime.getSeconds());
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(getTimestampRounding);
  RUN_TEST(setLocalTimestamp2000);
  RUN_TEST(setLocalTimestamp1970);
  RUN_TEST(setUTCTimestamp2000);
  RUN_TEST(setUTCTimestamp1970);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif