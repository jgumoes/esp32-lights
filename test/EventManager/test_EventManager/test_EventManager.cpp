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
#include "mockModalLights.hpp"

#include "testEvents.h"
#include "../../nativeMocksAndHelpers/mockConfig.h"
#include <iostream>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

auto makeTestConfigManager(){
  ConfigsStruct configs;
  configs.defaultEventWindow = 60*60;
  auto mockConfigHal = std::make_unique<MockConfigHal>();
  mockConfigHal->setConfigs(configs);
  std::shared_ptr<ConfigManagerClass> configsManager = std::make_shared<ConfigManagerClass>(std::move(mockConfigHal));
  return configsManager;
};

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
  std::vector<EventDataPacket> emptyEvents = {};
  // initialising with no events shouldn't cause an issue
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  auto configs = makeTestConfigManager();
  EventManager testClass(modalLights, configs, mondayAtMidnight, emptyEvents);
  TEST_ASSERT_EQUAL(0, testClass.getNextEventID());
  TEST_ASSERT_EQUAL(0, modalLights->getSetModeCount());

  // checking with no events shouldn't cause an issue
  testClass.check(mondayAtMidnight + timeToSeconds(6, 0, 0));
  TEST_ASSERT_EQUAL(0, testClass.getNextEventID());
  TEST_ASSERT_EQUAL(0, modalLights->getSetModeCount());

  // adding a good event should work fine
  testClass.addEvent(mondayAtMidnight + testEvent1.timeOfDay +1, testEvent1);
  TEST_ASSERT_EQUAL(testEvent1.eventID, testClass.getNextEventID());
  TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getMostRecentMode());
  TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());
}

void EventManagerFindsNextEventOnConstruction(void){
  uint64_t morningTimestamp = 794281114; // Monday 1:38:34 3/3/25 epoch=2000
  uint64_t eveningTimestamp = 794252745; // Sunday 17:45:45 2/3/25
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  auto configs = makeTestConfigManager();
  // testEvent1, morning
  {
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    std::vector<EventDataPacket> testEvents = {testEvent1};
    for(int i = 0; i < 5; i++){
      modalLights->resetInstance();
      EventManager testClass1(modalLights, configs, morningTimestamp + secondsInDay*i, testEvents);
      TEST_ASSERT_EQUAL(expectedTimestamp + secondsInDay*i, testClass1.getNextEventTime());
      TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));
    }
    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    modalLights->resetInstance();
    EventManager testClass2(modalLights, configs, morningTimestamp + secondsInDay*5, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEventTime());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));

    modalLights->resetInstance();
    EventManager testClass3(modalLights, configs, morningTimestamp + secondsInDay*6, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass3.getNextEventTime());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));
  }
  // testEvent1, evening
  {
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    std::vector<EventDataPacket> testEvents = {testEvent1};
    for(int i = 0; i < 5; i++){
      modalLights->resetInstance();
      EventManager testClass1(modalLights, configs, eveningTimestamp + secondsInDay*i, testEvents);
      TEST_ASSERT_EQUAL(expectedTimestamp + secondsInDay*i, testClass1.getNextEventTime());
    }
    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    modalLights->resetInstance();

    EventManager testClass2(modalLights, configs, eveningTimestamp + secondsInDay*5, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEventTime());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));

    modalLights->resetInstance();
    EventManager testClass3(modalLights, configs, eveningTimestamp + secondsInDay*6, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass3.getNextEventTime());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));
  }
  // testEvent1, just after alarm
  {
    uint64_t justAfterAlarm = 794299557;   // Monday 06:45:57 3/3/25
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    std::vector<EventDataPacket> testEvents = {testEvent1};
    for(int i = 0; i < 4; i++){
      // mon-thu
      modalLights->resetInstance();
      EventManager testClass1(modalLights, configs, justAfterAlarm + secondsInDay*i, testEvents);
      TEST_ASSERT_EQUAL(expectedTimestamp + secondsInDay*(i+1), testClass1.getNextEventTime());
      TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testEvent1.modeID));
    }
    // friday
    modalLights->resetInstance();
    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    EventManager testClass2(modalLights, configs, justAfterAlarm + secondsInDay*4, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEventTime());
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testEvent1.modeID));

    // saturday
    modalLights->resetInstance();
    // uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    EventManager testClass3(modalLights, configs, justAfterAlarm + secondsInDay*5, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEventTime());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));

    //sunday
    modalLights->resetInstance();
    EventManager testClass4(modalLights, configs, justAfterAlarm + secondsInDay*6, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass3.getNextEventTime());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));
  }
}

void EventManagerSelectsCorrectBackgroundMode(void){
  // shove in some background modes only, test accross different times of day on different days
  std::vector<EventDataPacket> testEvents = {testEvent3, testEvent4, testEvent5};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  auto configs = makeTestConfigManager();

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
        EventManager testClass(modalLights, configs, testTimestamp, testEvents);
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
    EventManager testClass(modalLights, configs, mondayAtMidnight, testEvents);
    for(int i = 0; i < 7; i++){
      for(const auto& [time, expectedMode] : testTimesAndExpectedModes){
        uint64_t testTimestamp = mondayAtMidnight + time + i*secondsInDay;
        testClass.check(testTimestamp);
        uint8_t setModeCount = modalLights->getSetModeCount();
        TEST_ASSERT_EQUAL(expectedMode, modalLights->getBackgroundMode());
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
    EventManager testClass(modalLights, configs, mondayAtMidnight, testEvents);
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
    modalLights->resetInstance();
    EventManager testClass(modalLights, configs, mondayAtMidnight + timeToSeconds(9, 0, 0), testEvents);
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getMostRecentMode());

    // rebuild trigger times
    modalLights->resetInstance();
    testClass.rebuildTriggerTimes(mondayAtMidnight + timeToSeconds(8, 59, 0));
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    // TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());
  }

  // eventWindow is ignored for background modes
  {
    modalLights->resetInstance();
    EventManager testClass(modalLights, configs, mondayAtMidnight + timeToSeconds(8, 59, 0), testEvents);
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());

    uint64_t eventWindow = testEvent5.eventWindow;
    TEST_ASSERT(eventWindow > hardwareMinimumEventWindow);  // just in case it gets changed for some reason
    testClass.check(mondayAtMidnight + testEvent5.timeOfDay + eventWindow + 10);
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getMostRecentMode());
  }
}

void onlyMostRecentActiveModeIsTriggered(void){
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  auto configs = makeTestConfigManager();

  // on construction
  {
    modalLights->resetInstance();
    std::vector<EventDataPacket> testEvents = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent6};
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);
    // TEST_ASSERT_EQUAL(5, modalLights->getMostRecentMode());
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(3));
    TEST_ASSERT_EQUAL(5, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(5));
    TEST_ASSERT_EQUAL(2, modalLights->getSetModeCount());
  }

  // every day of the week
  {
    std::vector<EventDataPacket> testEvents = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent6, testEvent10};
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight;
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    for(int i = 0; i < 5; i++){
      testClass.check(testTimestamp + i*secondsInDay + timeToSeconds(6, 45, 0));
      TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());

      testClass.check(testTimestamp + i*secondsInDay + timeToSeconds(7, 0, 0));
      TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());

      testClass.check(testTimestamp + i*secondsInDay + timeToSeconds(22, 0, 0));
      TEST_ASSERT_EQUAL(testEvent10.modeID, modalLights->getActiveMode());
    }
  }
}

void activeAndBackgroundModesAreBothTriggered(void){
  // the active mode should be called first, then the background mode
  std::vector<EventDataPacket> testEvents = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent6, testEvent7};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  auto configs = makeTestConfigManager();

  uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
  // on construction
  {
    modalLights->resetInstance();
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(6));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(5));
    TEST_ASSERT_EQUAL(6, modalLights->getMostRecentMode()); // won't be active, but should be called second
  }

  // on time change
  {
    EventManager testClass(modalLights, configs, mondayAtMidnight + timeToSeconds(6, 45, 0), testEvents);
    modalLights->resetInstance();
    testClass.check(testTimestamp);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(5));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(6));
    TEST_ASSERT_EQUAL(2, modalLights->getSetModeCount());
  }

  // when background mode has trigger time before active mode
  {
    // on construction
    std::vector<EventDataPacket> testEvents2 = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent10};
    modalLights->resetInstance();
    EventManager testClass(modalLights, configs, testTimestamp, testEvents2);
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    testClass.check(mondayAtMidnight + timeToSeconds(22, 0, 1));
    TEST_ASSERT_EQUAL(testEvent10.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());
  }
}

void missedActiveEventsAreSkippedAfterTimeWindowClosses(void){
  std::vector<EventDataPacket> testEvents = {testEvent1, testEvent7};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  auto configs = makeTestConfigManager();

  uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 0, 0);
  EventManager testClass(modalLights, configs, testTimestamp, testEvents);
  modalLights->resetInstance();
  testClass.check(mondayAtMidnight + timeToSeconds(12, 0, 0));
  TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
  TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());

  // modalLights->resetInstance();
  testClass.check(mondayAtMidnight + timeToSeconds(12, 0, 0) + secondsInDay);
  TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
}

void addEventChecksTheNewEvent(void){
  std::vector<EventDataPacket> testEvents = {testEvent7};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  auto configs = makeTestConfigManager();

  uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 46, 0);
  EventManager testClass(modalLights, configs, testTimestamp, testEvents);

  // active event should trigger when added
  modalLights->resetInstance();
  eventError_t error = testClass.addEvent(testTimestamp, testEvent1);
  TEST_ASSERT_EQUAL(EventManagerErrors::success, error);
  TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(2));
  TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
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
  EventDataPacket badEventUUID = testEvent2;
  badEventUUID.eventID = 0;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_uuid, testClass.addEvent(testTimestamp2, badEventUUID));
  EventDataPacket badModeUUID = testEvent2;
  badModeUUID.modeID = 0;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_uuid, testClass.addEvent(testTimestamp2, badModeUUID));


  // bad time should return error
  EventDataPacket badTimeOfDay = testEvent2;
  badTimeOfDay.timeOfDay = secondsInDay + 1;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_time, testClass.addEvent(testTimestamp2, badTimeOfDay));
  EventDataPacket badDaysOfWeek = testEvent2;
  badDaysOfWeek.daysOfWeek = 0;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_time, testClass.addEvent(testTimestamp2, badDaysOfWeek));
  EventDataPacket badEventWindow = testEvent2;
  badEventWindow.eventWindow = secondsInDay + 1;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_time, testClass.addEvent(testTimestamp2, badEventWindow));
}

void eventWindow0ShouldUseSystemDefault(void){
  // start configs with eventWindow = 1 hour
  auto configs = makeTestConfigManager();
  EventManagerConfigsStruct configStruct;
  configStruct.defaultEventWindow = 60*60;
  configs->setEventManagerConfigs(configStruct);
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();

  EventDataPacket testDefaultActiveEvent = testEvent1;
  testDefaultActiveEvent.eventWindow = 0;
  std::vector<EventDataPacket> testEvents = {testDefaultActiveEvent};

  const uint64_t testTimestamp = mondayAtMidnight + testDefaultActiveEvent.timeOfDay - 1;
  // constructor should use default event window
  {
    // construct with time = triggerTime + eventWindow/2
    modalLights->resetInstance();
    EventManager eventManager(modalLights, configs, testTimestamp + (configStruct.defaultEventWindow/2), testEvents);
    TEST_ASSERT_EQUAL(testDefaultActiveEvent.modeID, modalLights->getMostRecentMode());
    
    
    // construct with eventWindow = 0 and time = triggerTime - 1 second
    // skip forward 1 minute
    modalLights->resetInstance();
    EventManager eventManager2(modalLights, configs, testTimestamp, testEvents);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));
    eventManager2.check(testTimestamp + (configStruct.defaultEventWindow/2));
    TEST_ASSERT_EQUAL(testDefaultActiveEvent.modeID, modalLights->getMostRecentMode());
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));
  }

  // addEvent should use default event window
  {
    // change default eventWindow to 30 minutes
    configStruct.defaultEventWindow = 30*60;
    configs->setEventManagerConfigs(configStruct);

    std::vector<EventDataPacket> testEvents2 = {testEvent4};
    
    // should trigger during eventWindow
    modalLights->resetInstance();
    EventManager eventManager(modalLights, configs, testTimestamp, testEvents2);
    // add the event before it's trigger window
    eventManager.addEvent(mondayAtMidnight + testDefaultActiveEvent.timeOfDay - 1, testDefaultActiveEvent);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));
    eventManager.check(mondayAtMidnight + testDefaultActiveEvent.timeOfDay + configStruct.defaultEventWindow -10);
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));
    TEST_ASSERT_EQUAL(testDefaultActiveEvent.modeID, modalLights->getMostRecentMode());


    // shouldn't trigger after the window
    modalLights->resetInstance();
    EventManager eventManager2(modalLights, configs, testTimestamp, testEvents2);
    // time of alarm + defaultEventWindow (30 mins) - 10 seconds
    const uint64_t checkingTimestamp = mondayAtMidnight + testDefaultActiveEvent.timeOfDay + configStruct.defaultEventWindow + 10;
    // add the event after it's trigger window
    eventManager2.addEvent(checkingTimestamp, testDefaultActiveEvent);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));

    // if defaultEventWindow changes such that a missed event should be triggered,
    // rebuildTriggerTimes should trigger the missed event
    modalLights->resetInstance();
    // change the defaultEventWindow to 1 hour
    configStruct.defaultEventWindow = 60*60;
    configs->setEventManagerConfigs(configStruct);
    // check the event can trigger
    modalLights->resetInstance();
    eventManager2.rebuildTriggerTimes(checkingTimestamp);
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));
    TEST_ASSERT_EQUAL(2, modalLights->getSetModeCount()); // active + background should be called
  }
}


void testRemoveEvent(void){
  std::vector<EventDataPacket> testEvents = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  auto configs = makeTestConfigManager();

  // remove a single background event
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(22, 0, 0);
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getMostRecentMode());

    testClass.removeEvent(testTimestamp, testEvent3.eventID);
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getMostRecentMode());
  }

  // remove a single active event at trigger time
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 44, 0);
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);

    testClass.removeEvent(mondayAtMidnight + timeToSeconds(6, 50, 0), testEvent1.eventID);
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
  }

  // remove a single active event during trigger
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 50, 0);
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());

    testClass.removeEvent(testTimestamp, testEvent1.eventID);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.eventID));
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    TEST_ASSERT_EQUAL(1, modalLights->getCancelCallCount(testEvent1.modeID));
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
  }

  // remove array of background events
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(22, 0, 0);
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getMostRecentMode());

    eventUUID removeEvents[2] = {testEvent3.eventID, testEvent5.eventID};
    testClass.removeEvents(testTimestamp, removeEvents, 2);
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
  }

  // remove array of active events at trigger time
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 44, 0);
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);

    eventUUID removeEvents[2] = {testEvent1.eventID, testEvent2.eventID};
    testClass.removeEvents(mondayAtMidnight + timeToSeconds(6, 50, 0), removeEvents, 2);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.eventID));
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    TEST_ASSERT_EQUAL(2, modalLights->getCancelCallCount(testEvent1.modeID)); // event1 and event2 share a modeID
  }

  // remove active events during trigger time
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 50, 0);
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);

    eventUUID removeEvents[2] = {testEvent1.eventID, testEvent2.eventID};
    testClass.removeEvents(mondayAtMidnight + timeToSeconds(6, 50, 0), removeEvents, 2);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.eventID));
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    TEST_ASSERT_GREATER_OR_EQUAL(1, modalLights->getCancelCallCount(testEvent1.modeID));
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
  }

  // removing an active event during trigger time doesn't trigger the previous active event even though its still within its time window
  {
    modalLights->resetInstance();
    // construct @7am with testEvent9 and testEvent1, cancel testEvent9 and check testEvent1 doesn't trigger
    std::vector<EventDataPacket> testEvents2 = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent9};
    uint64_t testTimeestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
    EventManager testClass(modalLights, configs, testTimeestamp, testEvents2);
    TEST_ASSERT_EQUAL(testEvent9.modeID, modalLights->getActiveMode());

    testClass.removeEvent(testTimeestamp, testEvent9.eventID);
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
  }
}

void testUpdateEvent(void){
  std::vector<EventDataPacket> testEvents = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  auto configs = makeTestConfigManager();

  // change active event to trigger at current time
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 35, 0);
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());

    EventDataPacket newEvent1 = testEvent1;
    newEvent1.timeOfDay = timeToSeconds(6, 30, 0);
    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.updateEvent(testTimestamp, newEvent1));
    TEST_ASSERT_EQUAL(newEvent1.modeID, modalLights->getActiveMode());
  }

  // change triggered active event to latter time
  {
    // change active event to trigger at current time
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 50, 0);
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());

    EventDataPacket newEvent1 = testEvent1;
    newEvent1.timeOfDay = timeToSeconds(7, 30, 0);
    testClass.updateEvent(testTimestamp, newEvent1);
    TEST_ASSERT_EQUAL(mondayAtMidnight + timeToSeconds(7, 30, 0), testClass.getNextEventTime());
    // i don't think I want to cancel the mode. the interface that updates the events should have a force-cancel function
    // TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    // TEST_ASSERT_EQUAL(1, modalLights->getCancelCallCount(newEvent1.modeID));
  }

  // change current background event to future
  {
    modalLights->resetInstance();
    uint64_t  testTimestamp = mondayAtMidnight + timeToSeconds(21, 0, 10);
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());

    EventDataPacket newEvent3 = testEvent3;
    newEvent3.timeOfDay = timeToSeconds(21, 30, 0);
    testClass.updateEvent(testTimestamp, newEvent3);
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(newEvent3.eventID, testClass.getNextEventID());
    TEST_ASSERT_EQUAL(mondayAtMidnight + timeToSeconds(21, 30, 0), testClass.getNextEventTime());
  }
  
  // change future background event to past
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(20, 32, 0);
    EventManager testClass(modalLights, configs, testTimestamp, testEvents);
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());

    EventDataPacket newEvent3 = testEvent3;
    newEvent3.timeOfDay = timeToSeconds(20, 30, 0);
    testClass.updateEvent(testTimestamp, newEvent3);
    TEST_ASSERT_EQUAL(newEvent3.modeID, modalLights->getBackgroundMode());
  }

  // test updating an array of events
  {
    modalLights->resetInstance();
    std::vector<EventDataPacket> testEvents2 = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent9};
    uint64_t testTimestamp1 = mondayAtMidnight + timeToSeconds(6, 35, 0);
    EventManager testClass(modalLights, configs, testTimestamp1, testEvents2);
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
    eventError_t actualErrors[numberOfUpdates];

    eventError_t expectedErrors[numberOfUpdates] = {
      EventManagerErrors::success,
      EventManagerErrors::success,
      EventManagerErrors::success,
      EventManagerErrors::bad_uuid,
      EventManagerErrors::success,
      EventManagerErrors::bad_uuid,
      EventManagerErrors:: bad_time
    };

    uint64_t testTimestamp2 = mondayAtMidnight + timeToSeconds(7, 0, 0);
    testClass.updateEvents(testTimestamp2, updateEvents, actualErrors, numberOfUpdates);
    TEST_ASSERT_EQUAL_INT8_ARRAY(expectedErrors, actualErrors, numberOfUpdates);
    TEST_ASSERT_EQUAL(newEvent9.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(newEvent5.modeID, modalLights->getBackgroundMode());

    uint64_t testTimestamp3 = mondayAtMidnight + timeToSeconds(7, 31, 0);
    testClass.check(testTimestamp3);
    TEST_ASSERT_EQUAL(newEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(newEvent5.modeID, modalLights->getBackgroundMode());

    uint64_t testTimestamp4 = mondayAtMidnight + timeToSeconds(20, 31, 0);
    testClass.check(testTimestamp4);
    TEST_ASSERT_EQUAL(newEvent3.modeID, modalLights->getBackgroundMode());
  }
}

void integrateDeviceTime(void){
  TEST_IGNORE();
}

void somethingSomethingDST(void){
  TEST_IGNORE();
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
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
  RUN_TEST(integrateDeviceTime);
  RUN_TEST(somethingSomethingDST);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif