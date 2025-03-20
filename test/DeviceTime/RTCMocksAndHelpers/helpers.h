#ifndef __NATIVE_HELPERS_H__
#define __NATIVE_HELPERS_H__

#include "setTimeTestArray.h"

#define ONES(decimal) ((decimal) % 10)
#define TENS(decimal) ((decimal) / 10)

#define AS_BCD(decimal) (TENS(decimal) << 4) | ONES(decimal)
#define AS_DEC(bcd) ((((bcd) >> 4) * 10) + ((bcd) & 0b00001111))

// #include<iostream>

#include <algorithm>
#define COPY_ARRAY(inputArray, destinationArray) std::copy(std::begin(inputArray), std::end(inputArray), std::begin(destinationArray))

#ifdef _GLIBCXX_IOSTREAM
#define PRINT_AS_INT(title, value) std::cout << title << ": " << static_cast<int>(value) << std::endl

#define PRINT_ARRAY(name, array, length) std::cout << name << ":" << std::endl; \
  for(int i = 0; i < length; i++){\
    std::cout << '\t';\
    PRINT_AS_INT(i, array[i]);\
  }
#endif

#define EXPECT_CLOSE_TO(testValue, expectedValue) EXPECT_TRUE(testValue >= (expectedValue - 1)); EXPECT_TRUE(testValue <= (expectedValue + 1))

uint64_t findTimeInDay(TestParamsStruct testParams){
  uint64_t timestamp = testParams.seconds;
  timestamp += testParams.minutes * 60;
  timestamp += testParams.hours * 60 * 60;
  return timestamp * 1000000;
}

uint64_t findStartOfDay(TestParamsStruct testParams){
  uint64_t timestamp = testParams.localTimestamp * 1000000;
  timestamp -= findTimeInDay(testParams);
  return timestamp;
}

uint64_t utcTimeFromTestParams(TestParamsStruct testParams){
  int32_t offset = testParams.DST + testParams.timezone;
  return testParams.localTimestamp - offset;
}

#define testLocalGetters_helper(testParams, deviceTime){  \
  TEST_ASSERT_EQUAL_MESSAGE(testParams.localTimestamp, deviceTime.getLocalTimestampSeconds(), ("testParams: " + testParams.testName + "; fail: getLocalTimestampSeconds").c_str());\
  TEST_ASSERT_EQUAL_MESSAGE(utcTimeFromTestParams(testParams), deviceTime.getUTCTimestampSeconds(), ("testParams: " + testParams.testName + "; fail: getUTCTimestampSeconds").c_str());\
  TEST_ASSERT_EQUAL_MESSAGE(testParams.dayOfWeek, deviceTime.getDay(), ("testParams: " + testParams.testName + "; fail: getDay").c_str());\
  TEST_ASSERT_EQUAL_MESSAGE(testParams.month, deviceTime.getMonth(), ("testParams: " + testParams.testName + "; fail: getMonth").c_str());\
  TEST_ASSERT_EQUAL_MESSAGE(testParams.years, deviceTime.getYear(), ("testParams: " + testParams.testName + "; fail: getYear").c_str());\
  TEST_ASSERT_EQUAL_MESSAGE(testParams.date, deviceTime.getDate(), ("testParams: " + testParams.testName + "; fail: getDate").c_str());\
  TEST_ASSERT_EQUAL_MESSAGE(findStartOfDay(testParams), deviceTime.getStartOfDay(), ("testParams: " + testParams.testName + "; fail: getStartOfDay").c_str());\
  TEST_ASSERT_EQUAL_MESSAGE(findTimeInDay(testParams), deviceTime.getTimeInDay(), ("testParams: " + testParams.testName + "; fail: getTimeInDay").c_str());\
}

#endif