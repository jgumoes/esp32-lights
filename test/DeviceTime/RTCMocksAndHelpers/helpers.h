#ifndef __NATIVE_HELPERS_H__
#define __NATIVE_HELPERS_H__

#include "DeviceTime.h"
#include "ProjectDefines.h"
#include "setTimeTestArray.h"

#define ONES(decimal) ((decimal) % 10)
#define TENS(decimal) ((decimal) / 10)

#define AS_BCD(decimal) (TENS(decimal) << 4) | ONES(decimal)
#define AS_DEC(bcd) ((((bcd) >> 4) * 10) + ((bcd) & 0b00001111))

#include <algorithm>
#define COPY_ARRAY(inputArray, destinationArray) std::copy(std::begin(inputArray), std::end(inputArray), std::begin(destinationArray))

uint64_t findTimeInDay(TestTimeParamsStruct testParams){
  uint64_t timestamp = testParams.seconds;
  timestamp += testParams.minutes * 60;
  timestamp += testParams.hours * 60 * 60;
  return timestamp * 1000000;
}

uint64_t findStartOfDay(TestTimeParamsStruct testParams){
  uint64_t timestamp = testParams.localTimestamp * 1000000;
  timestamp -= findTimeInDay(testParams);
  return timestamp;
}

uint64_t utcTimeFromTestParams(TestTimeParamsStruct testParams){
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

class TestObserver : public TimeObserver{
  private:
    int _calls = 0;
    TimeUpdateStruct _updates{
      .utcTimeChange_uS= 0,
      .localTimeChange_uS = 0
    };
  public:
    TestObserver(){};
    
    void notification(const TimeUpdateStruct& timeUpdates){
      _updates = timeUpdates;
      _calls++;
    };

    void resetParams(){
      _calls = 0;
      _updates.utcTimeChange_uS = 0;
      _updates.localTimeChange_uS = 0;
    }

    int getCallCount(){return _calls;}

    int getCallCountAndReset(){
      int calls = _calls;
      _calls = 0;
      return calls;
    }

    TimeUpdateStruct getUpdates(){return _updates;}
};

/**
 * @brief helper class to cache the previous times, and find the time changes between previous times and new times
 * 
 */
class TimeChangeFinder{
  private:
    uint64_t _utcTimes_uS[2] = {0, 0};
    uint64_t _localTimes_uS[2] = {0, 0};

    bool _index = true;
  public:
    TimeChangeFinder(){};
    TimeChangeFinder(uint64_t utcTime_uS, uint64_t localTime_uS){
      _utcTimes_uS[_index] = utcTime_uS;
      _localTimes_uS[_index] = localTime_uS;
    }

    TimeUpdateStruct setTimes_uS(uint64_t utcTime_uS, uint64_t localTime_uS){
      _index = !_index;
      _utcTimes_uS[_index] = utcTime_uS;
      _localTimes_uS[_index] = localTime_uS;
      return getTimeUpdates();
    }

    TimeUpdateStruct setTimes_S(uint64_t utcTime_S, uint64_t localTime_S){
      return setTimes_uS(utcTime_S*secondsToMicros, localTime_S*secondsToMicros);
    }

    TimeUpdateStruct setTimes(TestTimeParamsStruct testParams){
      uint64_t localtime_S = testParams.localTimestamp;
      uint64_t utcTime_S = utcTimeFromTestParams(testParams);
      return setTimes_S(utcTime_S, localtime_S);
    }

    TimeUpdateStruct getTimeUpdates(){
      int64_t utcTimeChange_uS = _utcTimes_uS[_index] - _utcTimes_uS[!_index];
      int64_t localTimeChange_uS = _localTimes_uS[_index] - _localTimes_uS[!_index];
      return TimeUpdateStruct{
        .utcTimeChange_uS = utcTimeChange_uS,
        .localTimeChange_uS = localTimeChange_uS,
        .currentLocalTime_uS = _localTimes_uS[_index]
      };
    }
};

/**
 * @brief tests that the actual TimeUpdateStruct matches the expected TimeUpdateStruct
 * 
 */
#define TEST_ASSERT_EQUAL_TimeUpdateStruct(expUpdates, actUpdates) {\
  TEST_ASSERT_EQUAL(expUpdates.utcTimeChange_uS, actUpdates.utcTimeChange_uS);\
  TEST_ASSERT_EQUAL(expUpdates.localTimeChange_uS, actUpdates.localTimeChange_uS);\
  TEST_ASSERT_EQUAL(expUpdates.currentLocalTime_uS, actUpdates.currentLocalTime_uS);\
}

/**
 * @brief tests that the actual TimeUpdateStruct matches the expected TimeUpdateStruct
 * 
 */
#define TEST_ASSERT_EQUAL_TimeUpdateStruct_MESSAGE(expUpdates, actUpdates, message) {\
  std::string utcMessage = "utc failed; " + std::string(message);\
  TEST_ASSERT_EQUAL_MESSAGE(expUpdates.utcTimeChange_uS, actUpdates.utcTimeChange_uS, utcMessage.c_str());\
  std::string localUpdateMessage = "local update failed; " + std::string(message);\
  TEST_ASSERT_EQUAL_MESSAGE(expUpdates.localTimeChange_uS, actUpdates.localTimeChange_uS, localUpdateMessage.c_str());\
  std::string localTimeMessage = "local time failed; " + std::string(message);\
  TEST_ASSERT_EQUAL_MESSAGE(expUpdates.currentLocalTime_uS, actUpdates.currentLocalTime_uS, localTimeMessage.c_str());\
}

/**
 * @brief tests that the observer has been called once, updates updateCheck with expected times from testParams, and the expected TimeUpdateStruct from updateCheck matches the TimeUpdateStruct notified to the observer
 * @param observer TestObserver instance
 * @param updateCheck TimeChangeFinder instance
 * @param testParams TestTimeParamsStruct
 */
#define TEST_TIME_UPDATE_OBSERVER(observer, updateCheck, testParams){\
  TimeUpdateStruct expUpdates = updateCheck.setTimes(testParams);\
  TimeUpdateStruct actUpdates = observer.getUpdates();\
  TEST_ASSERT_EQUAL(1, observer.getCallCountAndReset());\
  std::string localTimestampMatchMessage = "local timestamp doesn't match testParams";\
  TEST_ASSERT_EQUAL_MESSAGE(testParams.localTimestamp*secondsToMicros, actUpdates.currentLocalTime_uS, localTimestampMatchMessage.c_str());\
  TEST_ASSERT_EQUAL_TimeUpdateStruct(expUpdates, actUpdates);\
}

/**
 * @brief tests that the observer has been called once, updates updateCheck with expected times from testParams, and the expected TimeUpdateStruct from updateCheck matches the TimeUpdateStruct notified to the observer
 * @param observer TestObserver instance
 * @param updateCheck TimeChangeFinder instance
 * @param testParams TestTimeParamsStruct
 * @param message c string
 */
#define TEST_TIME_UPDATE_OBSERVER_MESSAGE(observer, updateCheck, testParams, message){\
  TimeUpdateStruct expUpdates = updateCheck.setTimes(testParams);\
  TimeUpdateStruct actUpdates = observer.getUpdates();\
  TEST_ASSERT_EQUAL_MESSAGE(1, observer.getCallCountAndReset(), message);\
  std::string localTimestampMatchMessage = "local timestamp doesn't match testParams; " + std::string(message);\
  TEST_ASSERT_EQUAL_MESSAGE(testParams.localTimestamp*secondsToMicros, actUpdates.currentLocalTime_uS, localTimestampMatchMessage.c_str());\
  TEST_ASSERT_EQUAL_TimeUpdateStruct_MESSAGE(expUpdates, actUpdates, message);\
}

/**
 * @brief tests that the observer class has not been notified of time updates, without resetting any values
 * @param observer TestObserver instance
 */
#define TEST_TIME_OBSERVER_NOT_NOTIFIED(observer){\
  TEST_ASSERT_EQUAL(0, observer.getCallCount());\
  TimeUpdateStruct actUpdates = observer.getUpdates();\
  TEST_ASSERT_EQUAL(0, actUpdates.utcTimeChange_uS);\
  TEST_ASSERT_EQUAL(0, actUpdates.localTimeChange_uS);\
}

#endif