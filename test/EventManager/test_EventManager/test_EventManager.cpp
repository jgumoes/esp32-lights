#include <unity.h>
#include <string>
/*
javascript function for finding timestamps:
let timestamper = ()
*/
#include "EventManager.h"
#include "DeviceTime.h"

#include "testEvents.h"

const uint32_t secondsInDay = 60*60*24;

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void EventManagerFindsNextEvent(void){
  uint64_t morningTimestamp = 794281114; // Monday 1:38:34 3/3/25 epoch=2000
  uint64_t eveningTimestamp = 794252745; // Sunday 17:45:45 2/3/25
  
  // testEvent1, morning
  {
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    std::vector<TimeEventDataStruct> testEvents = {testEvent1};
    for(int i = 0; i < 5; i++){
      EventManager testClass1 = EventManager(morningTimestamp + secondsInDay*i, testEvents);
      TEST_ASSERT_EQUAL(expectedTimestamp + secondsInDay*i, testClass1.getNextEventTime());
    }
    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    EventManager testClass2 = EventManager(morningTimestamp + secondsInDay*5, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEventTime());
    EventManager testClass3 = EventManager(morningTimestamp + secondsInDay*6, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass3.getNextEventTime());
  }
  // testEvent1, evening
  {
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    std::vector<TimeEventDataStruct> testEvents = {testEvent1};
    for(int i = 0; i < 5; i++){
      EventManager testClass1 = EventManager(eveningTimestamp + secondsInDay*i, testEvents);
      TEST_ASSERT_EQUAL(expectedTimestamp + secondsInDay*i, testClass1.getNextEventTime());
    }
    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    EventManager testClass2 = EventManager(eveningTimestamp + secondsInDay*5, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEventTime());
    EventManager testClass3 = EventManager(eveningTimestamp + secondsInDay*6, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass3.getNextEventTime());
  }
  // testEvent1, just after alarm
  {
    uint64_t justAfterAlarm = 794299557;   // Monday 06:45:57 3/3/25
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    std::vector<TimeEventDataStruct> testEvents = {testEvent1};
    for(int i = 0; i < 5; i++){
      EventManager testClass1 = EventManager(justAfterAlarm + secondsInDay*i, testEvents);
      TEST_ASSERT_EQUAL(expectedTimestamp + secondsInDay*i, testClass1.getNextEventTime());
    }
    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    EventManager testClass2 = EventManager(justAfterAlarm + secondsInDay*5, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEventTime());
    EventManager testClass3 = EventManager(justAfterAlarm + secondsInDay*6, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass3.getNextEventTime());
  }
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(EventManagerFindsNextEvent);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif