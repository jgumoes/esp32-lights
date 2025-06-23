#include <unity.h>
#include <string>
#include <etl/vector.h>
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
#include "mockModalLights.hpp"
#include "DataStorageClass.h"

#include "testEvents.h"
#include "../../ConfigStorageClass/testObjects.hpp"
#include "../../nativeMocksAndHelpers/mockStorageHAL.hpp"
#include "../../DeviceTime/RTCMocksAndHelpers/setTimeTestArray.h"
#include "../../DeviceTime/RTCMocksAndHelpers/RTCMockWire.h"

namespace EventManagerTestHelpers
{
  struct TimeStruct{
    uint64_t localTimestamp_S = 0;  // initial local timestamp, 200 epoch
    int32_t timezone_S = 0;
    uint16_t DST_S = 0;
  };

  struct TestEventsStruct{
  private:
    eventsVector_t _events;
  public:
    void set(eventsVector_t newEvents){
      _events = newEvents;
    }

    const eventsVector_t get(){
      return _events;
    }

    TestEventsStruct(eventsVector_t initialEvents) : _events(initialEvents){}
  };
  
  struct TestObjects{
    std::shared_ptr<DeviceTimeClass> deviceTime;
    std::shared_ptr<ConfigManagerTestObjects::FakeStorageAdapter> storageAdapter;
    std::shared_ptr<ConfigStorageClass> configStorage;
    std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();

    std::shared_ptr<MockStorageHAL> mockStorageHAL; // TODO: delete
    std::shared_ptr<DataStorageClass> dataStorage;
    
    std::shared_ptr<EventManager> eventManager;

    /**
     * @brief Constructs test objects. sets the local timestamp, then resets modalLights before constructing eventManager
     * 
     * @param initialTime 
     * @param initialEvents 
     * @param initialModes 
     * @param initialConfigs 
     */
    TestObjects(TimeStruct initialTime, eventsVector_t initialEvents, modesVector_t initialModes, EventManagerConfigsStruct initialConfigs){
      using namespace ConfigManagerTestObjects;
      etl::map<ModuleID, GenericConfigStruct, 255> initialConfigMap = {
        {ModuleID::modalLights, makeGenericConfigStruct(
          initialConfigs
        )}
      };
      storageAdapter = std::make_shared<FakeStorageAdapter>(initialConfigMap, true, "event manager");
      configStorage = std::make_shared<ConfigStorageClass>(storageAdapter);

      deviceTime = std::make_shared<DeviceTimeClass>(configStorage);
      deviceTime->setLocalTimestamp2000(initialTime.localTimestamp_S, initialTime.timezone_S, initialTime.DST_S);

      mockStorageHAL = std::make_shared<MockStorageHAL>(initialModes, initialEvents);
      dataStorage = std::make_shared<DataStorageClass>(mockStorageHAL);
      dataStorage->loadIDs(); // TODO: delete
      modalLights->resetInstance();

      eventManager = std::make_shared<EventManager>(modalLights, configStorage, deviceTime, dataStorage);

      // the load order is extremely important
      configStorage->loadAllConfigs();
      // TODO: modalLights->loadAllModes();
      eventManager->loadAllEvents();
    }

    errorCode_t setConfigs(EventManagerConfigsStruct newConfigs){
      byte serializedConfigs[maxConfigSize];
      ConfigStructFncs::serialize(serializedConfigs, newConfigs);
      return configStorage->setConfig(serializedConfigs);
    }
  };

  /**
   * @brief contains an expected timestamp, and expected modeCallCount
   * 
   */
  struct ExpectedTimestampStruct{
    uint64_t timestamp_S = 0; // expected timestamp, 2000 epoch
    uint8_t modeCallCount = 0;        // expected mode call count
  };
} // namespace EventManagerTestHelpers

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void EventDataPacketShouldInitialiseEmpty(void){
  EventDataPacket emptyEvent;

  TEST_ASSERT_EQUAL(0, emptyEvent.daysOfWeek);
  TEST_ASSERT_EQUAL(0, emptyEvent.eventID);
  TEST_ASSERT_EQUAL(0, emptyEvent.eventWindow);
  TEST_ASSERT_EQUAL(0, emptyEvent.modeID);
  TEST_ASSERT_EQUAL(0, emptyEvent.timeOfDay);
  TEST_ASSERT_EQUAL(0, emptyEvent.isActive);
}

void InitialisingWithNoEvents(void){
  using namespace EventManagerTestHelpers;

  EventManagerConfigsStruct configStruct;
  eventsVector_t emptyEvents = {};
  TestObjects testObjects(
    TimeStruct{.localTimestamp_S = mondayAtMidnight},
    emptyEvents,
    {},
    configStruct
  );
  auto deviceTime = testObjects.deviceTime;
  // initialising with no events shouldn't cause an issue
  auto modalLights = testObjects.modalLights;
  auto testClass = testObjects.eventManager;
  
  TEST_ASSERT_EQUAL(0, testClass->getNextEvent().ID);
  TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());
  TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());

  // checking with no events shouldn't cause an issue
  deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(6, 0, 0), 0, 0);
  testClass->check();
  TEST_ASSERT_EQUAL(0, testClass->getNextEvent().ID);
  TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());
  TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());

  // adding a good event should work fine
  deviceTime->setLocalTimestamp2000(mondayAtMidnight + testEvent1.timeOfDay +1, 0, 0);
  testClass->addEvent(testEvent1);
  TEST_ASSERT_EQUAL(testEvent1.eventID, testClass->getNextEvent().ID);
  TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());
  TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
}

void EventManagerFindsNextEventOnConstruction(void){
  using namespace EventManagerTestHelpers;

  EventManagerConfigsStruct configStruct;
  uint64_t morningTimestamp = 794281114; // Monday 1:38:34 3/3/25 epoch=2000
  uint64_t eveningTimestamp = 794252745; // Sunday 17:45:45 2/3/25
  // testEvent1, morning before alarm
  {
    const std::string testMessage = "testEvent1, morning before alarm";
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    etl::map<int, ExpectedTimestampStruct, 7> expectedMap = {
      {0, {expectedTimestamp + secondsInDay*0, 0}},   // monday
      {1, {expectedTimestamp + secondsInDay*1, 0}},
      {2, {expectedTimestamp + secondsInDay*2, 0}},
      {3, {expectedTimestamp + secondsInDay*3, 0}},
      {4, {expectedTimestamp + secondsInDay*4, 0}},   // friday
      {5, {expectedTimestamp + 7 * secondsInDay, 0}},
      {6, {expectedTimestamp + 7 * secondsInDay, 0}},
    };
    eventsVector_t testEvents = {testEvent1};
    for(int i = 0; i < 7; i++){
      const std::string loopMessage = testMessage + "; day = " + std::to_string(i);
      TestObjects testObjects(
        TimeStruct{.localTimestamp_S = morningTimestamp + secondsInDay*i},
        testEvents,
        {},
        configStruct
      );
      auto modalLights = testObjects.modalLights;
      auto testClass = testObjects.eventManager;

      const ExpectedTimestampStruct expected = expectedMap.at(i);
      TEST_ASSERT_EQUAL_MESSAGE(expected.timestamp_S, testClass->getNextEvent().triggerTime, loopMessage.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expected.modeCallCount, modalLights->getModeCallCount(testEvent1.modeID), loopMessage.c_str());
    }
  }

  // testEvent1, evening
  {
    const std::string testName = "testEvent1, evening";
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    etl::map<int, ExpectedTimestampStruct, 7> expectedMap = {
      {0, {expectedTimestamp + secondsInDay*0, 0}},   // monday
      {1, {expectedTimestamp + secondsInDay*1, 0}},
      {2, {expectedTimestamp + secondsInDay*2, 0}},
      {3, {expectedTimestamp + secondsInDay*3, 0}},
      {4, {expectedTimestamp + secondsInDay*4, 0}},   // friday
      {5, {expectedTimestamp + 7 * secondsInDay, 0}},
      {6, {expectedTimestamp + 7 * secondsInDay, 0}},
    };
    eventsVector_t testEvents = {testEvent1};
    for(int i = 0; i < 5; i++){
      const std::string loopMessage = testName + "; day = " + std::to_string(i);
      TestObjects testObjects(
        TimeStruct{.localTimestamp_S = eveningTimestamp + secondsInDay*i},
        testEvents,
        {},
        configStruct
      );
      auto modalLights = testObjects.modalLights;
      auto testClass = testObjects.eventManager;
      
      const ExpectedTimestampStruct expected = expectedMap.at(i);
      TEST_ASSERT_EQUAL_MESSAGE(expected.timestamp_S, testClass->getNextEvent().triggerTime, loopMessage.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expected.modeCallCount, modalLights->getModeCallCount(testEvent1.modeID), loopMessage.c_str());
    }

    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
  }

  // testEvent1, just after alarm
  {
    const std::string testName = "testEvent1, just after alarm";
    uint64_t justAfterAlarm = 794299557;   // Monday 06:45:57 3/3/25
    const uint64_t firstExpTime = 794299500; // Monday 6:45
    etl::map<int, ExpectedTimestampStruct, 7> expectedMap = {
      {0, {firstExpTime + secondsInDay*1, 1}},    // monday
      {1, {firstExpTime + secondsInDay*2, 1}},
      {2, {firstExpTime + secondsInDay*3, 1}},
      {3, {firstExpTime + secondsInDay*4, 1}},
      {4, {firstExpTime + 7 * secondsInDay, 1}},  // friday
      {5, {firstExpTime + 7 * secondsInDay, 0}},
      {6, {firstExpTime + 7 * secondsInDay, 0}},
    };
    eventsVector_t testEvents = {testEvent1};
    for(int i = 0; i < 7; i++){
      const std::string loopMessage = testName + "; day = " + std::to_string(i);
      TestObjects testObjects(
        TimeStruct{.localTimestamp_S = justAfterAlarm + secondsInDay*i},
        testEvents,
        {},
        configStruct
      );
      auto modalLights = testObjects.modalLights;
      auto testClass = testObjects.eventManager;

      const ExpectedTimestampStruct expected = expectedMap.at(i);
      TEST_ASSERT_EQUAL_MESSAGE(expected.timestamp_S, testClass->getNextEvent().triggerTime, loopMessage.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expected.modeCallCount, modalLights->getModeCallCount(testEvent1.modeID), loopMessage.c_str());
    }
  }

  // TODO: add test for many different active and background modes
  
  TEST_FAIL_MESSAGE("important TODO: on construction, EventManager, must set all valid modes in order until the next modes are in the future. this will let ModalLights pick out only the modes that are valid");
}

void EventManagerSelectsCorrectBackgroundMode(void){
  using namespace EventManagerTestHelpers;

  OnboardTimestamp onboardTimestamp;
  EventManagerConfigsStruct configStruct;

  // shove in some background modes only, test accross different times of day on different days
  eventsVector_t testEvents = {testEvent3, testEvent4, testEvent5};

  etl::flat_map<uint32_t, modeUUID, 100> testTimesAndExpectedModes = {
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
      int t = 0;
      for(const auto& [time, expectedMode] : testTimesAndExpectedModes){
        const uint64_t testTimestamp = mondayAtMidnight + time + i*secondsInDay;
        std::string message = "day = " + std::to_string(i) +"; t = " + std::to_string(t) + "; testTimestamp = " + std::to_string(testTimestamp);
        
        TestObjects testObjects(
          TimeStruct{.localTimestamp_S = testTimestamp},
          testEvents,
          {},
          configStruct
        );
        auto modalLights = testObjects.modalLights;
        auto testClass = testObjects.eventManager;

        TEST_ASSERT_EQUAL_MESSAGE(expectedMode, modalLights->getBackgroundMode(), message.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(expectedMode, modalLights->getMostRecentMode(), message.c_str());

        // TODO: test events were triggered in order
        
        t++;
      }
    }
  }

  // test switching. First test also tests behaviour with repeated timestamp checks
  {
    uint8_t expectedSetCount = 0;
    modeUUID previousMode = 0;
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = mondayAtMidnight},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    
    for(int i = 0; i < 7; i++){
      int i_map = 0;
      for(const auto& [time, expectedMode] : testTimesAndExpectedModes){
        const uint64_t testTimestamp = mondayAtMidnight + time + i*secondsInDay;
        onboardTimestamp.setTimestamp_S(testTimestamp);
        testClass->check();
        
        // TODO: test events were triggered (where applicable)
        TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
        TEST_ASSERT_EQUAL(expectedMode, modalLights->getBackgroundMode());
        if(expectedMode != previousMode){
          expectedSetCount++; previousMode = expectedMode;
        }
        i_map++;
      }
    }
  }

  // eventWindow is ignored for background modes
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(8, 59, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());

    uint64_t eventWindow = testEvent5.eventWindow;
    TEST_ASSERT(eventWindow > hardwareMinimumEventWindow);  // just in case it gets changed for some reason

    onboardTimestamp.setTimestamp_S(mondayAtMidnight + testEvent5.timeOfDay + eventWindow + 10);
    testClass->check();
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getMostRecentMode());
  }

  TEST_FAIL_MESSAGE("important TODO: on construction, EventManager, must set all valid modes in order until the next modes are in the future. this will let ModalLights pick out only the modes that are valid");
}

void onlyMostRecentActiveModeIsTriggered(void){
  using namespace EventManagerTestHelpers;

  EventManagerConfigsStruct configStruct;
  // on construction
  {
    eventsVector_t testEvents = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent6};
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    // TODO: test active events get triggered in order
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    // TEST_ASSERT_EQUAL(2, modalLights->getSetModeCount());
  }

  // every day of the week
  {
    eventsVector_t testEvents = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent6, testEvent10};
    const uint64_t testTimestamp = mondayAtMidnight;
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    OnboardTimestamp onboardTimestamp;
    
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    // weekday
    for(int i = 0; i < 5; i++){
      std::string message = "day = " + std::to_string(i + 1);
      onboardTimestamp.setTimestamp_S(testTimestamp + i*secondsInDay + timeToSeconds(6, 45, 0));
      testClass->check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent1.modeID, modalLights->getActiveMode(), message.c_str());

      onboardTimestamp.setTimestamp_S(testTimestamp + i*secondsInDay + timeToSeconds(7, 0, 0));
      testClass->check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent6.modeID, modalLights->getActiveMode(), message.c_str());

      onboardTimestamp.setTimestamp_S(testTimestamp + i*secondsInDay + timeToSeconds(22, 0, 0));
      testClass->check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent10.modeID, modalLights->getActiveMode(), message.c_str());
    }

    // weekend
    for(int d = 5; d < 7; d++){
      std::string message = "day = " + std::to_string(d + 1);

      onboardTimestamp.setTimestamp_S(testTimestamp + d*secondsInDay + timeToSeconds(10, 0, 0));
      testClass->check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent2.modeID, modalLights->getActiveMode(), message.c_str());

      onboardTimestamp.setTimestamp_S(testTimestamp + d*secondsInDay + timeToSeconds(22, 0, 0));
      testClass->check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent10.modeID, modalLights->getActiveMode(), message.c_str());
    }
  }

  TEST_FAIL_MESSAGE("important TODO: on construction, EventManager, must set all valid modes in order until the next modes are in the future. this will let ModalLights pick out only the modes that are valid");
}

void activeAndBackgroundModesAreBothTriggered(void){
  using namespace EventManagerTestHelpers;

  // the active mode should be called first, then the background mode
  eventsVector_t testEvents = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent6, testEvent7};
  OnboardTimestamp onboardTimestamp;
  EventManagerConfigsStruct configStruct;

  const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
  // on construction
  {
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getMostRecentMode()); // won't be active, but should be called second
  }

  // on time change
  {
    const uint64_t initialTimestamp = mondayAtMidnight + timeToSeconds(6, 45, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = initialTimestamp},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    
    onboardTimestamp.setTimestamp_S(testTimestamp);
    testClass->check();
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getMostRecentMode()); // won't be active, but should be called second
  }

  // when background mode has trigger time before active mode
  {
    // on construction
    eventsVector_t testEvents2 = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent10};
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents2,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(22, 0, 1));
    testClass->check();
    TEST_ASSERT_EQUAL(testEvent10.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());
  }

  TEST_FAIL_MESSAGE("important TODO: on construction, EventManager, must set all valid modes in order until the next modes are in the future. this will let ModalLights pick out only the modes that are valid");
}

void missedActiveEventsAreSkippedAfterTimeWindowClosses(void){
  using namespace EventManagerTestHelpers;

  eventsVector_t testEvents = {testEvent1, testEvent7};
  OnboardTimestamp onboardTimestamp;
  EventManagerConfigsStruct configStruct;

  const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 0, 0);
  
  TestObjects testObjects(
    TimeStruct{.localTimestamp_S = testTimestamp},
    testEvents,
    {},
    configStruct
  );
  auto modalLights = testObjects.modalLights;
  auto testClass = testObjects.eventManager;
  
  modalLights->resetInstance();
  onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(12, 0, 0));
  testClass->check();
  TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
  TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

  onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(12, 0, 0) + secondsInDay);
  testClass->check();
  TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
}

void addEventChecksTheNewEvent(void){
  using namespace EventManagerTestHelpers;

  OnboardTimestamp onboardTimestamp;
  EventManagerConfigsStruct configStruct;

  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 46, 0);
    eventsVector_t testEvents = {testEvent7};
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
  
    // active event should trigger when added
    modalLights->resetInstance();
    errorCode_t error = testClass->addEvent(testEvent1);
    TEST_ASSERT_EQUAL(errorCode_t::success, error);
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(2));
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());
  
    // active event shouldn't trigger when added
    modalLights->resetInstance();
    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->addEvent(testEvent8));
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(7));
    // TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());
  
    // background event should trigger
    uint64_t testTimestamp2 = mondayAtMidnight + timeToSeconds(23, 59, 59);
    onboardTimestamp.setTimestamp_S(testTimestamp2);
    testClass->check();
    modalLights->resetInstance();
    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->addEvent(testEvent4));
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
  
    // background event shouldn't trigger
    modalLights->resetInstance();
    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->addEvent(testEvent3));
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent3.modeID));
  
    // bad uuid should return error
    TEST_ASSERT_EQUAL(errorCode_t::bad_uuid, testClass->addEvent(testEvent3));
    EventDataPacket badEventUUID = testEvent2;
    badEventUUID.eventID = 0;
    TEST_ASSERT_EQUAL(errorCode_t::bad_uuid, testClass->addEvent(badEventUUID));
    EventDataPacket badModeUUID = testEvent2;
    badModeUUID.modeID = 0;
    TEST_ASSERT_EQUAL(errorCode_t::bad_uuid, testClass->addEvent(badModeUUID));
  
    // bad time should return error
    EventDataPacket badTimeOfDay = testEvent2;
    badTimeOfDay.timeOfDay = secondsInDay + 1;
    TEST_ASSERT_EQUAL(errorCode_t::bad_time, testClass->addEvent(badTimeOfDay));
    EventDataPacket badDaysOfWeek = testEvent2;
    badDaysOfWeek.daysOfWeek = 0;
    TEST_ASSERT_EQUAL(errorCode_t::bad_time, testClass->addEvent(badDaysOfWeek));
    EventDataPacket badEventWindow = testEvent2;
    badEventWindow.eventWindow = secondsInDay + 1;
    TEST_ASSERT_EQUAL(errorCode_t::bad_time, testClass->addEvent(badEventWindow));
  }

  // start with an active event that's just triggered. add an active event that should have triggered after the existing active event. the new event should be active
  {
    eventsVector_t testEvents = {testEvent1, testEvent2, testEvent3, testEvent4};
    const uint64_t testTimestamp = mondayAtMidnight+timeToSeconds(7, 0, 1);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    testClass->addEvent(testEvent6);
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    testClass->addEvent(testEvent7);
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
  }

  // start with an active event that's just triggered. add an active event that's still within its event window, but before the current active event. the new active event should not have triggered
  {
    eventsVector_t testEvents = {testEvent2, testEvent3, testEvent6, testEvent7};
    const uint64_t testTimestamp = mondayAtMidnight+timeToSeconds(7, 0, 1);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    
    testClass->addEvent(testEvent1);
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

    testClass->addEvent(testEvent4);
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
  }

  TEST_IGNORE_MESSAGE("important tests need to be added");
}

void eventWindow0ShouldUseSystemDefault(void){
  using namespace EventManagerTestHelpers;

  OnboardTimestamp onboardTimestamp;

  // start configs with eventWindow = 1 hour
  
  EventManagerConfigsStruct configStruct{.defaultEventWindow_S = 60*60};

  EventDataPacket testDefaultActiveEvent = testEvent1;
  testDefaultActiveEvent.eventWindow = 0;
  eventsVector_t testEvents = {testDefaultActiveEvent};

  const uint64_t testTimestamp = mondayAtMidnight + testDefaultActiveEvent.timeOfDay - 1;
  // constructor should use default event window
  {
    // construct with time = triggerTime + eventWindow/2
    {
      const uint64_t initialTimestamp = testTimestamp + (configStruct.defaultEventWindow_S/2);
      TestObjects testObjects(
        TimeStruct{.localTimestamp_S = initialTimestamp},
        testEvents,
        {},
        configStruct
      );
      auto modalLights = testObjects.modalLights;
      auto testClass = testObjects.eventManager;
  
      TEST_ASSERT_EQUAL(testDefaultActiveEvent.modeID, modalLights->getActiveMode());
    }
    
    // construct with eventWindow = 0 and time = triggerTime - 1 second
    {
      TestObjects testObjects(
        TimeStruct{.localTimestamp_S = testTimestamp},
        testEvents,
        {},
        configStruct
      );
      auto modalLights = testObjects.modalLights;
      auto testClass = testObjects.eventManager;
      TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));
  
      onboardTimestamp.setTimestamp_S(testTimestamp + (configStruct.defaultEventWindow_S/2));
      testClass->check();
  
      TEST_ASSERT_EQUAL(testDefaultActiveEvent.modeID, modalLights->getActiveMode());
      TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));
    }
  }

  // change default eventWindow to 30 minutes
  configStruct.defaultEventWindow_S = 30*60;

  // addEvent should use default event window
  {

    eventsVector_t testEvents2 = {testEvent4};
    
    // should trigger during eventWindow
    {
      TestObjects testObjects(
        TimeStruct{.localTimestamp_S = testTimestamp},
        testEvents2,
        {},
        configStruct
      );
      auto modalLights = testObjects.modalLights;
      auto testClass = testObjects.eventManager;
      // add the event before it's trigger window
      testObjects.deviceTime->setLocalTimestamp2000(mondayAtMidnight + testDefaultActiveEvent.timeOfDay - 1, 0, 0);
      testClass->addEvent(testDefaultActiveEvent);
      TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));
  
      onboardTimestamp.setTimestamp_S(mondayAtMidnight + testDefaultActiveEvent.timeOfDay + configStruct.defaultEventWindow_S -10);
  
      testClass->check();
      TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));
      TEST_ASSERT_EQUAL(testDefaultActiveEvent.modeID, modalLights->getMostRecentMode());
    }


    // shouldn't trigger after the window
    {
      TestObjects testObjects(
        TimeStruct{.localTimestamp_S = testTimestamp},
        testEvents,
        {},
        configStruct
      );
      auto modalLights = testObjects.modalLights;
      auto testClass = testObjects.eventManager;
      
      // time of alarm + defaultEventWindow (30 mins) - 10 seconds
      const uint64_t checkingTimestamp = mondayAtMidnight + testDefaultActiveEvent.timeOfDay + configStruct.defaultEventWindow_S + 10;
      // add the event after it's trigger window
      testObjects.deviceTime->setLocalTimestamp2000(checkingTimestamp, 0, 0);
      testClass->addEvent(testDefaultActiveEvent);
      TEST_ASSERT_NOT_EQUAL(testDefaultActiveEvent.modeID, modalLights->getActiveMode());
      TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
  
      // if defaultEventWindow changes such that a missed event should be triggered, rebuildTriggerTimes should trigger the missed event
      modalLights->resetInstance();

      EventManagerConfigsStruct newConfigs = testClass->getConfigs();
      // change the defaultEventWindow to 1 hour
      newConfigs.defaultEventWindow_S = 60*60;
      TEST_ASSERT_SUCCESS(testObjects.setConfigs(newConfigs), "missed window configs");

      TEST_ASSERT_EQUAL(testDefaultActiveEvent.modeID, modalLights->getActiveMode());
      TEST_ASSERT_EQUAL(2, modalLights->getSetModeCount()); // active + background should be called
    }
  }
}

void testRemoveEvent(void){
  using namespace EventManagerTestHelpers;

  OnboardTimestamp onboardTimestamp;

  TestEventsStruct testEvents({testEvent1, testEvent2, testEvent3, testEvent4, testEvent5});
  EventManagerConfigsStruct configStruct;

  // remove a single background event
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(22, 0, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getMostRecentMode());
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());

    testClass->removeEvent(testEvent3.eventID);
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());
  }

  // remove a single active event at trigger time
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 44, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(6, 50, 0));
    testClass->removeEvent(testEvent1.eventID);
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
  }

  // remove a single active event during trigger
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 50, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());

    testClass->removeEvent(testEvent1.eventID);
    TEST_ASSERT_NOT_EQUAL(testEvent1.eventID, modalLights->getActiveMode());
    // TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    // TEST_ASSERT_EQUAL(1, modalLights->getCancelCallCount(testEvent1.modeID));
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
  }

  // remove array of background events
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(22, 0, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getMostRecentMode());

    eventUUID removeEvents[2] = {testEvent3.eventID, testEvent5.eventID};
    testClass->removeEvents(removeEvents, 2);
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
  }

  // remove array of active events at trigger time
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 44, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    eventUUID removeEvents[2] = {testEvent1.eventID, testEvent2.eventID};
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(6, 50, 0));
    modalLights->test_cancelActiveMode();  // cancel testEvent1
    testClass->removeEvents(removeEvents, 2);
    // TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.eventID));
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
  }

  // remove active events during trigger time
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 50, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    eventUUID removeEvents[2] = {testEvent1.eventID, testEvent2.eventID};
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(6, 50, 0));
    modalLights->test_cancelActiveMode();
    testClass->removeEvents(removeEvents, 2);
    // TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.eventID));
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
  }

  // removing an active event during trigger time doesn't trigger the previous active event even though its still within its time window
  {
    // construct @7am with testEvent9 and testEvent1, cancel testEvent9 and check testEvent1 doesn't trigger
    testEvents.set({testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent9});
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    
    TEST_ASSERT_EQUAL(testEvent9.modeID, modalLights->getActiveMode());

    testClass->removeEvent(testEvent9.eventID);
    // TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
  }

  // clearing all background events should call defaultConstBrightness
  {
    testEvents.set({testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent7});
    const uint64_t testTimestamp = mondayAtMidnight;
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->removeEvent(testEvent4.eventID));
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->removeEvent(testEvent3.eventID));
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->removeEvent(testEvent5.eventID));
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

    // remove the last event
    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->removeEvent(testEvent7.eventID));
    TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());
  }

  // all events can be cleared
  {
    testEvents.set({testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent7});
    const uint64_t testTimestamp = mondayAtMidnight;
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->removeEvent(testEvent4.eventID));
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->removeEvent(testEvent3.eventID));
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->removeEvent(testEvent5.eventID));
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

    // remove the last background event
    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->removeEvent(testEvent7.eventID));
    TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(testEvent1.eventID, testClass->getNextActiveEvent().ID);
    TEST_ASSERT_EQUAL(testEvent1.eventID, testClass->getNextEvent().ID);

    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->removeEvent(testEvent1.eventID));
    TEST_ASSERT_EQUAL(testEvent2.eventID, testClass->getNextEvent().ID);

    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->removeEvent(testEvent2.eventID));
    TEST_ASSERT_EQUAL(0, testClass->getNextEvent().ID);
    TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());
  }
}

void testUpdateEvent(void){
  using namespace EventManagerTestHelpers;

  TestEventsStruct testEvents({testEvent1, testEvent2, testEvent3, testEvent4, testEvent5});
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  OnboardTimestamp onboardTimestamp;
  EventManagerConfigsStruct configStruct;

  // change active event to trigger at current time
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 35, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());

    EventDataPacket newEvent1 = testEvent1;
    newEvent1.timeOfDay = timeToSeconds(6, 30, 0);
    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->updateEvent(newEvent1));
    TEST_ASSERT_EQUAL(newEvent1.modeID, modalLights->getActiveMode());
  }

  // change triggered active event to latter time
  {
    // change active event to trigger at current time
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 50, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());

    EventDataPacket newEvent1 = testEvent1;
    newEvent1.timeOfDay = timeToSeconds(7, 30, 0);
    testClass->updateEvent(newEvent1);
    TEST_ASSERT_EQUAL(mondayAtMidnight + timeToSeconds(7, 30, 0), testClass->getNextEvent().triggerTime);
  }

  // change current background event to future
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(21, 0, 10);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());

    EventDataPacket newEvent3 = testEvent3;
    newEvent3.timeOfDay = timeToSeconds(21, 30, 0);
    testClass->updateEvent(newEvent3);
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(newEvent3.eventID, testClass->getNextEvent().ID);
    TEST_ASSERT_EQUAL(mondayAtMidnight + timeToSeconds(21, 30, 0), testClass->getNextEvent().triggerTime);
  }
  
  // change future background event to past
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(20, 32, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());

    EventDataPacket newEvent3 = testEvent3;
    newEvent3.timeOfDay = timeToSeconds(20, 30, 0);
    testClass->updateEvent(newEvent3);
    TEST_ASSERT_EQUAL(newEvent3.modeID, modalLights->getBackgroundMode());
  }

  // test updating an array of events
  {
    modalLights->resetInstance();
    testEvents.set({testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent9});
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 35, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    // good events
    EventDataPacket newEvent1 = testEvent1;
    newEvent1.timeOfDay = timeToSeconds(7, 30, 0);
    EventDataPacket newEvent5 = testEvent5;
    newEvent5.timeOfDay = timeToSeconds(6, 30, 0);
    EventDataPacket newEvent3 = testEvent3;
    newEvent3.timeOfDay = timeToSeconds(20, 30, 0);
    EventDataPacket newEvent9 = testEvent9;
    newEvent9.timeOfDay = timeToSeconds(6, 30, 0);

    EventDataPacket badEvent1 = testEvent10;
    EventDataPacket badEvent2 = testEvent1;
    badEvent2.modeID = 0;
    EventDataPacket badEvent3 = testEvent2;
    badEvent3.timeOfDay = secondsInDay + 10;
    nEvents_t numberOfUpdates = 7;
    EventDataPacket updateEvents[numberOfUpdates] = {
      newEvent1,
      newEvent5,
      newEvent3,
      badEvent1,  // bad id (event doesn't exist)
      newEvent9,
      badEvent2,  // bad mode id
      badEvent3   // bad time
    };
    errorCode_t actualErrors[numberOfUpdates];

    errorCode_t expectedErrors[numberOfUpdates] = {
      errorCode_t::success,
      errorCode_t::success,
      errorCode_t::success,
      errorCode_t::bad_uuid,
      errorCode_t::success,
      errorCode_t::bad_uuid,
      errorCode_t:: bad_time
    };

    uint64_t testTimestamp2 = mondayAtMidnight + timeToSeconds(7, 0, 0);
    onboardTimestamp.setTimestamp_S(testTimestamp2);
    testClass->updateEvents(updateEvents, actualErrors, numberOfUpdates);
    TEST_ASSERT_EQUAL_INT8_ARRAY(expectedErrors, actualErrors, numberOfUpdates);
    TEST_ASSERT_EQUAL(newEvent9.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(newEvent5.modeID, modalLights->getBackgroundMode());

    uint64_t testTimestamp3 = mondayAtMidnight + timeToSeconds(7, 31, 0);
    onboardTimestamp.setTimestamp_S(testTimestamp3);
    testClass->check();
    TEST_ASSERT_EQUAL(newEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(newEvent5.modeID, modalLights->getBackgroundMode());

    uint64_t testTimestamp4 = mondayAtMidnight + timeToSeconds(20, 31, 0);
    onboardTimestamp.setTimestamp_S(testTimestamp4);
    testClass->check();
    TEST_ASSERT_EQUAL(newEvent3.modeID, modalLights->getBackgroundMode());
  }

  // test changing an active event to background
  {
    modalLights->resetInstance();
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 50, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    modalLights->resetInstance();
    EventDataPacket newEvent1 = testEvent1;
    newEvent1.isActive = false;
    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->updateEvent(newEvent1));
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(newEvent1.modeID, modalLights->getBackgroundMode());
  }

  // test changing a background mode to active
  {
    modalLights->resetInstance();
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(21, 0, 3);
    testEvents.set({testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent12});
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;
    
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());

    modalLights->resetInstance();
    EventDataPacket newEvent3 = testEvent3;
    newEvent3.isActive = true;
    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->updateEvent(newEvent3));
    TEST_ASSERT_EQUAL(newEvent3.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent12.modeID, modalLights->getBackgroundMode());
  }

  // test changing the only background mode to active
  {
    modalLights->resetInstance();
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(10, 0, 3);
    testEvents.set({testEvent1, testEvent2, testEvent12});
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto testClass = testObjects.eventManager;

    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent12.modeID, modalLights->getBackgroundMode());

    EventDataPacket newEvent12 = testEvent12;
    newEvent12.isActive = true;
    TEST_ASSERT_EQUAL(errorCode_t::success, testClass->updateEvent(newEvent12));
    TEST_ASSERT_EQUAL(newEvent12.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());
  }
}

void eventSkipping(void){
  using namespace EventManagerTestHelpers;

  TEST_IGNORE_MESSAGE("TODO");
}

void testConfigChanges(void){
  using namespace EventManagerTestHelpers;

  TEST_FAIL_MESSAGE("not yet implemented, but very important");
}

void testTimeUpdates(void){
  using namespace EventManagerTestHelpers;

  TestEventsStruct testEvents({testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent6, testEvent7, testEvent11});
  const EventManagerConfigsStruct configStruct{
    .defaultEventWindow_S = 10*60   // 10 minutes
  };
  OnboardTimestamp onboardTimestamp;
  onboardTimestamp.setTimestamp_S(mondayAtMidnight);
  
  // negative time changes < minimumEventWindow should rebuild trigger times without checking missed active events
  {
    const uint64_t testTimestamp = mondayAtMidnight;
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;

    // start at Monday 7am. testEvent7 should be background and testEvent6 should be active.
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(7, 0, 0));
    testClass->check();
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());

    // change time to 6:59:01. testEvent4 should be background and testEvent6 should be active
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(6, 59, 1), 0, 0);
    // eventManager should rebuild event times in the observer callback to avoid collisions with methods such as addEvent(). current timestamp is passed with updates, so other observers being held up shouldn't matter
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());

    // set time to 7am. testEvent7 should be background, and testEvent6 shouldn't retrigger
    modalLights->resetInstance(); // effectively cancels active mode
    testClass->check(); // events were already called, so this should do nothing
    TEST_ASSERT_EQUAL(0, modalLights->getSetModeCount());

    // const uint8_t expActiveCalls = modalLights->getModeCallCount(testEvent7.modeID);
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(7, 0, 0));
    testClass->check();
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
  }

  // negative time changes > smallTimeAdjustmentWindow_m should rebuild events and check missed active
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(9, 0, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;

    // set time to 9am. testEvent5 should be background and testEvent11 should be active.
    TEST_ASSERT_EQUAL(testEvent11.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());

    // change time to 7:00:01 am. testEvent4 should be background and testEvent1 should be active
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(7, 0, 1));
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

    // fast forward to 8:30am. testEvent7 should be still be background and testEvent11 should trigger again.
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(8, 30, 0));
    testClass->check();
    TEST_ASSERT_EQUAL(testEvent11.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
  } 

  // two small negative changes less than one big negative change shouldn't cause skipped events to trigger
  { 
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 59, 59);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;
    
    // start at 6:59:59am. testEvent4 should be background and testEvent1 should be active
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());

    // change time to 7am. testEvent7 should be background and testEvent6 should be active.
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(7, 0, 0), 0, 0);
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    
    // change time to 6:59:30, then 6:59:01. testEvent4 should be background and testEvent6 should be active
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(6, 59, 30), 0, 0);
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(6, 59, 1), 0, 0);
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());

    // set time to 7am testEvent7 should be background, and testEvent6 shouldn't retrigger
    modalLights->resetInstance();
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(7, 0, 0));
    testClass->check();
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
  }

  // small negative changes don't effect active events beyond the change time
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 59, 59);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;

    // set time to 6:59:59am. testEvent4 should be background and testEvent1 should be active
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());

    // change time to 6:59:00am, then set time to 7am. testEvent7 should be background and testEvent6 should be active
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(6, 59, 0), 0, 0);
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(7, 0, 0));
    testClass->check();
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
  }

  // positive time changes less than event window should be ignored
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;

    // set time to 7am. testEvent7 should be background and testEvent6 should be active.
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());

    // change time to 7:01:00. modalLights shouldn't have been called
    const uint8_t callCount = modalLights->getSetModeCount();
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(7, 1, 0), 0, 0);
    testClass->check();
    TEST_ASSERT_EQUAL(callCount, modalLights->getSetModeCount());
  }

  // positive time changes greater than event window should rebuild events checking for missed events
  {
    const uint64_t testTimestamp = mondayAtMidnight;
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;

    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(9, 0, 1), 0, 0);
    // testClass->check();
    TEST_ASSERT_TRUE(testEvent11.eventWindow > configStruct.defaultEventWindow_S); // test that the rebuilding is done properly
    TEST_ASSERT_EQUAL(testEvent11.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());

    // skip over the daytime modes
    modalLights->resetInstance();
    deviceTime->setUTCTimestamp2000(mondayAtMidnight + timeToSeconds(21, 0, 0), 0, oneHour);
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());
  }

  // a small positive time adjustment that triggers an active mode, followed by a small negative adjustment
  {
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 59, 59);
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;

    // start at 6:59:59am. testEvent4 should be background and testEvent1 should be active
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());

    // set time to 7am. testEvent7 should be background and testEvent6 should be active.
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(7, 0, 0), 0, 0);
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
  }

  // changing the time to an event trigger time should trigger those events
  {
    testEvents.set({testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent6, testEvent10});
    const uint64_t testTimestamp = mondayAtMidnight;
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;
    
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    for(int i = 0; i < 5; i++){
      std::string message = "day = " + std::to_string(i + 1);
      deviceTime->setLocalTimestamp2000(testTimestamp + i*secondsInDay + timeToSeconds(6, 45, 0), 0, 0);
      TEST_ASSERT_EQUAL_MESSAGE(testEvent1.modeID, modalLights->getActiveMode(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(testEvent4.modeID, modalLights->getBackgroundMode(), message.c_str());

      deviceTime->setLocalTimestamp2000(testTimestamp + i*secondsInDay + timeToSeconds(7, 0, 0), 0, 0);
      TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
      TEST_ASSERT_EQUAL_MESSAGE(testEvent4.modeID, modalLights->getBackgroundMode(), message.c_str());

      deviceTime->setLocalTimestamp2000(testTimestamp + i*secondsInDay + timeToSeconds(22, 0, 0), 0, 0);
      TEST_ASSERT_EQUAL(testEvent10.modeID, modalLights->getActiveMode());
      TEST_ASSERT_EQUAL_MESSAGE(testEvent3.modeID, modalLights->getBackgroundMode(), message.c_str());
    }

    // weekend
    for(int d = 5; d < 7; d++){
      std::string message = "day = " + std::to_string(d + 1);

      deviceTime->setLocalTimestamp2000(testTimestamp + d*secondsInDay + timeToSeconds(10, 0, 0));
      // testClass->check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent2.modeID, modalLights->getActiveMode(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(testEvent5.modeID, modalLights->getBackgroundMode(), message.c_str());

      deviceTime->setLocalTimestamp2000(testTimestamp + d*secondsInDay + timeToSeconds(22, 0, 0));
      testClass->check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent10.modeID, modalLights->getActiveMode(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(testEvent3.modeID, modalLights->getBackgroundMode(), message.c_str());
    }
  }
  

  {
    // on construction
    const uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
    testEvents.set({testEvent1, testEvent3, testEvent4, testEvent5, testEvent10});
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents.get(),
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;

    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(22, 0, 1), 0, 0);
    testClass->check();
    TEST_ASSERT_EQUAL(testEvent10.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());
  }

}

void testEventLimit(){
  // test that the number of stored events cannot exceed a pre-defined limit
  using namespace EventManagerTestHelpers;

  TEST_ASSERT_TRUE(MAX_NUMBER_OF_EVENTS > 10);
  TEST_ASSERT_TRUE(MAX_NUMBER_OF_EVENTS < 254);
  
  eventsVector_t testEvents = {};
  EventManagerConfigsStruct configStruct;
  
  // background events can't exceed limit
  {
    const uint64_t testTimestamp = mondayAtMidnight;
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;

    for(uint32_t i = 0; i < MAX_NUMBER_OF_EVENTS; i++){
      EventDataPacket packet{
        .eventID = static_cast<eventUUID>(i + 1),
        .modeID = static_cast<modeUUID>(i + 1),
        .timeOfDay = i,
        .daysOfWeek = 0b01111111,
        .eventWindow = 15*60,
        .isActive = false
      };
      TEST_ASSERT_EQUAL(errorCode_t::success, testClass->addEvent(packet));
    }

    EventDataPacket activePacket{
      .eventID = static_cast<eventUUID>(MAX_NUMBER_OF_EVENTS + 2),
      .modeID = static_cast<modeUUID>(MAX_NUMBER_OF_EVENTS + 2),
      .timeOfDay = MAX_NUMBER_OF_EVENTS,
      .daysOfWeek = 0b01111111,
      .eventWindow = 15*60,
      .isActive = true
    };
    TEST_ASSERT_EQUAL(errorCode_t::storage_full, testClass->addEvent(activePacket));

    EventDataPacket backgroundPacket{
      .eventID = static_cast<eventUUID>(MAX_NUMBER_OF_EVENTS + 1),
      .modeID = static_cast<modeUUID>(MAX_NUMBER_OF_EVENTS + 1),
      .timeOfDay = MAX_NUMBER_OF_EVENTS + 1,
      .daysOfWeek = 0b01111111,
      .eventWindow = 15*60,
      .isActive = false
    };
    TEST_ASSERT_EQUAL(errorCode_t::storage_full, testClass->addEvent(backgroundPacket));
  }

  // active events can't exceed limit
  {
    const uint64_t testTimestamp = mondayAtMidnight;
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;

    for(uint32_t i = 0; i < MAX_NUMBER_OF_EVENTS; i++){
      EventDataPacket packet{
        .eventID = static_cast<eventUUID>(i + 1),
        .modeID = static_cast<modeUUID>(i + 1),
        .timeOfDay = i,
        .daysOfWeek = 0b01111111,
        .eventWindow = 15*60,
        .isActive = true
      };
      TEST_ASSERT_EQUAL(errorCode_t::success, testClass->addEvent(packet));
    }

    EventDataPacket activePacket{
      .eventID = static_cast<eventUUID>(MAX_NUMBER_OF_EVENTS + 2),
      .modeID = static_cast<modeUUID>(MAX_NUMBER_OF_EVENTS + 2),
      .timeOfDay = MAX_NUMBER_OF_EVENTS,
      .daysOfWeek = 0b01111111,
      .eventWindow = 15*60,
      .isActive = true
    };
    TEST_ASSERT_EQUAL(errorCode_t::storage_full, testClass->addEvent(activePacket));

    EventDataPacket backgroundPacket{
      .eventID = static_cast<eventUUID>(MAX_NUMBER_OF_EVENTS + 1),
      .modeID = static_cast<modeUUID>(MAX_NUMBER_OF_EVENTS + 1),
      .timeOfDay = MAX_NUMBER_OF_EVENTS + 1,
      .daysOfWeek = 0b01111111,
      .eventWindow = 15*60,
      .isActive = false
    };
    TEST_ASSERT_EQUAL(errorCode_t::storage_full, testClass->addEvent(backgroundPacket));
  }

  // total events can't exceed limit
  {
    eventsVector_t testEvents = {};
    const uint64_t testTimestamp = mondayAtMidnight;
    TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;

    for(uint32_t i = 0; i < MAX_NUMBER_OF_EVENTS; i++){
      EventDataPacket packet{
        .eventID = static_cast<eventUUID>(i + 1),
        .modeID = static_cast<modeUUID>(i + 1),
        .timeOfDay = i,
        .daysOfWeek = 0b01111111,
        .eventWindow = 15*60,
        .isActive = (i%2 == 1)
      };
      TEST_ASSERT_EQUAL(errorCode_t::success, testClass->addEvent(packet));
    }

    EventDataPacket activePacket{
      .eventID = static_cast<eventUUID>(MAX_NUMBER_OF_EVENTS + 2),
      .modeID = static_cast<modeUUID>(MAX_NUMBER_OF_EVENTS + 2),
      .timeOfDay = MAX_NUMBER_OF_EVENTS,
      .daysOfWeek = 0b01111111,
      .eventWindow = 15*60,
      .isActive = true
    };
    TEST_ASSERT_EQUAL(errorCode_t::storage_full, testClass->addEvent(activePacket));

    EventDataPacket backgroundPacket{
      .eventID = static_cast<eventUUID>(MAX_NUMBER_OF_EVENTS + 1),
      .modeID = static_cast<modeUUID>(MAX_NUMBER_OF_EVENTS + 1),
      .timeOfDay = MAX_NUMBER_OF_EVENTS + 1,
      .daysOfWeek = 0b01111111,
      .eventWindow = 15*60,
      .isActive = false
    };
    TEST_ASSERT_EQUAL(errorCode_t::storage_full, testClass->addEvent(backgroundPacket));
  }
}

void testEventWindowLimits(){
  using namespace EventManagerTestHelpers;

  EventManagerConfigsStruct configStruct;

  eventsVector_t testEvents = getAllTestEvents();
  const uint64_t testTimestamp = mondayAtMidnight;
  TestObjects testObjects(
      TimeStruct{.localTimestamp_S = testTimestamp},
      testEvents,
      {},
      configStruct
    );
    auto modalLights = testObjects.modalLights;
    auto deviceTime = testObjects.deviceTime;
    auto testClass = testObjects.eventManager;
  
  // event window under hardwareMinimumEventWindow gets rejected
  {
    for(uint32_t i = 0; i < hardwareMinimumEventWindow; i++){
      EventManagerConfigsStruct testConfigs = testClass->getConfigs();
      testConfigs.defaultEventWindow_S = i;
      TEST_ASSERT_ERROR(errorCode_t::bad_time, testObjects.setConfigs(testConfigs), "");
    }

    EventManagerConfigsStruct testConfigs = testClass->getConfigs();
    testConfigs.defaultEventWindow_S = hardwareMinimumEventWindow;
    TEST_ASSERT_ERROR(errorCode_t::success, testObjects.setConfigs(testConfigs), "");
  }
}

void noEmbeddedUnfriendlyLibraries(){
  #ifdef __PRINT_DEBUG_H__
    TEST_ASSERT_MESSAGE(false, "did you forget to remove the print debugs?");
  #else
    TEST_ASSERT(true);
  #endif

  #ifdef _GLIBCXX_MAP
    TEST_ASSERT_MESSAGE(false, "std::map is included");
  #else
    TEST_ASSERT(true);
  #endif
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(noEmbeddedUnfriendlyLibraries);
  RUN_TEST(EventDataPacketShouldInitialiseEmpty);
  RUN_TEST(InitialisingWithNoEvents);
  RUN_TEST(EventManagerFindsNextEventOnConstruction);
  RUN_TEST(EventManagerSelectsCorrectBackgroundMode);
  RUN_TEST(onlyMostRecentActiveModeIsTriggered);
  RUN_TEST(activeAndBackgroundModesAreBothTriggered);
  RUN_TEST(missedActiveEventsAreSkippedAfterTimeWindowClosses);
  RUN_TEST(addEventChecksTheNewEvent);
  RUN_TEST(eventWindow0ShouldUseSystemDefault);
  RUN_TEST(testRemoveEvent);
  RUN_TEST(testUpdateEvent);
  RUN_TEST(eventSkipping);
  RUN_TEST(testTimeUpdates);
  RUN_TEST(testEventLimit);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif