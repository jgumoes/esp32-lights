#include <unity.h>
#include <string>
/*
javascript function for finding timestamps:

let convert2000To1970 = (timestamp) => {
  let epoch = new Date(0);
  epoch.setYear(2000);
  epoch.setUTCHours(0);
  return new Date(epoch.getTime() + timestamp * 1000);
}

*/
#include "EventManager.h"
#include "DeviceTime.h"
#include <ModalLights.h>

#include "testEvents.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

class MockModalLights : public ModalLightsInterface{
  private:
    std::map<modeUUID, uint8_t> _calledModes;
    // std::shared_ptr<ModeStorageClassInterface> _storage;
    modeUUID _activeMode = 0;
    uint8_t _setModeCount = 0;
  public:
    MockModalLights(){};
    void setModeByUUID(modeUUID modeID) override {
      _setModeCount++;
      if(_calledModes.count(modeID) == 0){
        _calledModes[modeID] = 1;
      }
      else{
        _calledModes[modeID] += 1;
      }
      _activeMode = modeID;
    }

    uint8_t getModeCallCount(modeUUID modeID){
      return _calledModes[modeID];
    }
    uint8_t getSetModeCount(){return _setModeCount;}

    modeUUID getMostRecentMode(){
      modeUUID currentActiveMode = _activeMode;
      return currentActiveMode;
    }
    void resetInstance(){
      _calledModes.clear();
      _activeMode = 0;
      _setModeCount = 0;
    }
};

void EventManagerFindsNextEventOnConstruction(void){
  uint64_t morningTimestamp = 794281114; // Monday 1:38:34 3/3/25 epoch=2000
  uint64_t eveningTimestamp = 794252745; // Sunday 17:45:45 2/3/25
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  
  // testEvent1, morning
  {
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    std::vector<TimeEventDataStruct> testEvents = {testEvent1};
    for(int i = 0; i < 5; i++){
      modalLights->resetInstance();
      EventManager testClass1(modalLights, morningTimestamp + secondsInDay*i, testEvents);
      TEST_ASSERT_EQUAL(expectedTimestamp + secondsInDay*i, testClass1.getNextEventTime());
      TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));
    }
    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    modalLights->resetInstance();
    EventManager testClass2(modalLights, morningTimestamp + secondsInDay*5, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEventTime());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));

    modalLights->resetInstance();
    EventManager testClass3(modalLights, morningTimestamp + secondsInDay*6, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass3.getNextEventTime());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));
  }
  // testEvent1, evening
  {
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    std::vector<TimeEventDataStruct> testEvents = {testEvent1};
    for(int i = 0; i < 5; i++){
      modalLights->resetInstance();
      EventManager testClass1(modalLights, eveningTimestamp + secondsInDay*i, testEvents);
      TEST_ASSERT_EQUAL(expectedTimestamp + secondsInDay*i, testClass1.getNextEventTime());
    }
    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    modalLights->resetInstance();

    EventManager testClass2(modalLights, eveningTimestamp + secondsInDay*5, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEventTime());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));

    modalLights->resetInstance();
    EventManager testClass3(modalLights, eveningTimestamp + secondsInDay*6, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass3.getNextEventTime());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));
  }
  // testEvent1, just after alarm
  {
    uint64_t justAfterAlarm = 794299557;   // Monday 06:45:57 3/3/25
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    std::vector<TimeEventDataStruct> testEvents = {testEvent1};
    for(int i = 0; i < 4; i++){
      // mon-thu
      modalLights->resetInstance();
      EventManager testClass1(modalLights, justAfterAlarm + secondsInDay*i, testEvents);
      TEST_ASSERT_EQUAL(expectedTimestamp + secondsInDay*(i+1), testClass1.getNextEventTime());
      TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testEvent1.modeID));
    }
    // friday
    modalLights->resetInstance();
    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    EventManager testClass2(modalLights, justAfterAlarm + secondsInDay*4, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEventTime());
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testEvent1.modeID));

    // saturday
    modalLights->resetInstance();
    // uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    EventManager testClass3(modalLights, justAfterAlarm + secondsInDay*5, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEventTime());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));

    //sunday
    modalLights->resetInstance();
    EventManager testClass4(modalLights, justAfterAlarm + secondsInDay*6, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass3.getNextEventTime());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));
  }
}

void EventManagerSelectsCorrectBackgroundMode(void){
  // shove in some background modes only, test accross different times of day on different days
  std::vector<TimeEventDataStruct> testEvents = {testEvent3, testEvent4, testEvent5};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();

  std::map<uint32_t, modeUUID> testTimesAndExpectedModes = {
    {timeToSeconds(0, 0, 0), 3},
    {timeToSeconds(8, 59, 0), 3},
    {timeToSeconds(9, 0, 0), 1},
    {timeToSeconds(12, 0, 0), 1},
    {timeToSeconds(20, 59, 59), 1},
    {timeToSeconds(21, 0, 0), 4},
    {timeToSeconds(22, 0, 0), 4},
    {timeToSeconds(23, 10, 59), 3}
  };

  // test the constructor
  {
    for(int i = 0; i < 7; i++){
      for(const auto& [time, expectedMode] : testTimesAndExpectedModes){
        modalLights->resetInstance();
        uint64_t testTimestamp = mondayAtMidnight + time + i*secondsInDay;
        EventManager testClass(modalLights, testTimestamp, testEvents);
        TEST_ASSERT_EQUAL(expectedMode, modalLights->getMostRecentMode());
        TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(expectedMode));
        TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());
      }
    }
  }

  // test switching. First test also tests behaviour with repeated timestamp checks
  {
    uint8_t expectedSetCount = 0;
    modeUUID previousMode = 0;
    modalLights->resetInstance();
    EventManager testClass(modalLights, mondayAtMidnight, testEvents);
    for(int i = 0; i < 7; i++){
      for(const auto& [time, expectedMode] : testTimesAndExpectedModes){
        uint64_t testTimestamp = mondayAtMidnight + time + i*secondsInDay;
        testClass.check(testTimestamp);
        modeUUID currentActiveMode = modalLights->getMostRecentMode();
        uint8_t setModeCount = modalLights->getSetModeCount();
        TEST_ASSERT_EQUAL(expectedMode, currentActiveMode);
        // TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(expectedMode));
        if(expectedMode != previousMode){
          expectedSetCount++; previousMode = expectedMode;
        }
        TEST_ASSERT_EQUAL(expectedSetCount, modalLights->getSetModeCount());
      }
    }
  }

  // handles forward time-skips well i.e. adjustment after onboard clock looses time
  {
    modalLights->resetInstance();
    EventManager testClass(modalLights, mondayAtMidnight, testEvents);
    testClass.check(mondayAtMidnight + timeToSeconds(8, 59, 0));
    TEST_ASSERT_EQUAL(3, modalLights->getMostRecentMode());
    TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());
    // skip over the daytime mode
    testClass.check(mondayAtMidnight + timeToSeconds(22, 0, 0));
    TEST_ASSERT_EQUAL(4, modalLights->getMostRecentMode());

    // it would be nice to minimise read operations in ModalLights, but
    // doing so in EventManager is probably more expensive than it is worth.
    // Besides, time skips should only be a couple of minutes at most
    // TEST_ASSERT_EQUAL(2, modalLights->getSetModeCount());
  }

  // handles reverse time-skips well i.e. adjustment after onboard clock gains time
  {

  }
}

void onlyMostRecentActiveModeIsTriggered(void){
  // shove in background modes and two missed active modes with overlapping windows
  std::vector<TimeEventDataStruct> testEvents = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent6};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();

  uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
  // on construction
  {
    modalLights->resetInstance();
    EventManager testClass(modalLights, testTimestamp, testEvents);
    // TEST_ASSERT_EQUAL(5, modalLights->getMostRecentMode());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(3));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(5));
    TEST_ASSERT_EQUAL(2, modalLights->getSetModeCount());
  }

  // // on time skip
  // {
  //   modalLights->resetInstance();
  //   EventManager testClass(modalLights, mondayAtMidnight, testEvents);
  //   TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());
  //   testClass.check(testTimestamp);
  //   TEST_ASSERT_EQUAL(5, modalLights->getMostRecentMode());
  //   TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
  // }
}

void activeAndBackgroundModesWithSameTimeAreBothTriggered(void){
  // the active mode should be called first, then the background mode
  std::vector<TimeEventDataStruct> testEvents = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent6, testEvent7};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();

  uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
  // on construction
  {
    modalLights->resetInstance();
    EventManager testClass(modalLights, testTimestamp, testEvents);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(6));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(5));
    TEST_ASSERT_EQUAL(6, modalLights->getMostRecentMode()); // won't be active, but should be called second
  }

  // on time change
  {
    EventManager testClass(modalLights, mondayAtMidnight + timeToSeconds(6, 45, 0), testEvents);
    modalLights->resetInstance();
    testClass.check(testTimestamp);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(5));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(6));
    TEST_ASSERT_EQUAL(2, modalLights->getSetModeCount());
  }
}

void missedActiveEventsAreSkippedAfterTimeWindowClosses(void){
  std::vector<TimeEventDataStruct> testEvents = {testEvent1, testEvent7};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();

  uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 0, 0);
  EventManager testClass(modalLights, testTimestamp, testEvents);
  modalLights->resetInstance();
  testClass.check(mondayAtMidnight + timeToSeconds(12, 0, 0));
  TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
  TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());

  // modalLights->resetInstance();
  testClass.check(mondayAtMidnight + timeToSeconds(12, 0, 0) + secondsInDay);
  TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
}

void addEventChecksTheNewEvent(void){
  std::vector<TimeEventDataStruct> testEvents = {testEvent7};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();

  uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 46, 0);
  EventManager testClass(modalLights, testTimestamp, testEvents);

  // active event should trigger whn added
  modalLights->resetInstance();
  eventError_t error = testClass.addEvent(testTimestamp, testEvent1);
  TEST_ASSERT_EQUAL(EventManagerErrors::success, error);
  TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(2));
  TEST_ASSERT_EQUAL(2, modalLights->getMostRecentMode());
  TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());

  // active event shouldn't trigger when added
  modalLights->resetInstance();
  TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.addEvent(testTimestamp, testEvent8));
  TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(7));
  // TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());

  // background event should trigger
  uint64_t testTimestamp2 = mondayAtMidnight + timeToSeconds(23, 59, 59);
  testClass.check(testTimestamp2);
  modalLights->resetInstance();
  TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.addEvent(testTimestamp2, testEvent4));
  TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testEvent4.modeID));

  // background event shouldn't trigger
  modalLights->resetInstance();
  TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.addEvent(testTimestamp2, testEvent3));
  TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent3.modeID));

  // bad uuid should return error
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_uuid, testClass.addEvent(testTimestamp2, testEvent3));
  TimeEventDataStruct badEventUUID = testEvent2;
  badEventUUID.eventID = 0;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_uuid, testClass.addEvent(testTimestamp2, badEventUUID));
  TimeEventDataStruct badModeUUID = testEvent2;
  badModeUUID.modeID = 0;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_uuid, testClass.addEvent(testTimestamp2, badModeUUID));


  // bad time should return error
  TimeEventDataStruct badTimeOfDay = testEvent2;
  badTimeOfDay.timeOfDay = secondsInDay + 1;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_time, testClass.addEvent(testTimestamp2, badTimeOfDay));
  TimeEventDataStruct badDaysOfWeek = testEvent2;
  badDaysOfWeek.daysOfWeek = 0;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_time, testClass.addEvent(testTimestamp2, badDaysOfWeek));
  TimeEventDataStruct badEventWindow = testEvent2;
  badEventWindow.eventWindow = secondsInDay + 1;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_time, testClass.addEvent(testTimestamp2, badEventWindow));
}

void eventWindow0ShouldUseSystemDefault(void){
  TEST_ASSERT(false);
}
void somethingSomethingDST(void){
  TEST_ASSERT(false);
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(EventManagerFindsNextEventOnConstruction);
  RUN_TEST(EventManagerSelectsCorrectBackgroundMode);
  RUN_TEST(onlyMostRecentActiveModeIsTriggered);
  RUN_TEST(activeAndBackgroundModesWithSameTimeAreBothTriggered);
  RUN_TEST(missedActiveEventsAreSkippedAfterTimeWindowClosses);
  RUN_TEST(addEventChecksTheNewEvent);
  RUN_TEST(eventWindow0ShouldUseSystemDefault);
  RUN_TEST(somethingSomethingDST);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif