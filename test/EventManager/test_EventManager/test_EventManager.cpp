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
// #include "DeviceTime.h"
#include "mockModalLights.hpp"
#include "DataStorageClass.h"

#include "testEvents.h"
#include "../../nativeMocksAndHelpers/mockConfig.h"
#include "../../nativeMocksAndHelpers/mockStorageHAL.hpp"
#include "../../DeviceTime/RTCMocksAndHelpers/setTimeTestArray.h"
#include "../../DeviceTime/RTCMocksAndHelpers/RTCMockWire.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

auto EventManagerFactory(
  std::shared_ptr<MockModalLights> modalLights,
  std::shared_ptr<ConfigManagerClass> configs,
  std::shared_ptr<DeviceTimeClass> deviceTime,
  std::vector<EventDataPacket> testEvents
){
  std::vector<ModeDataStruct> modeDataPackets = {};
  auto mockStorageHAL = std::make_shared<MockStorageHAL>(modeDataPackets, testEvents);
  std::vector<TestModeDataStruct> testModes = {mvpModes["warmConstBrightness"]};
  auto dataStorage = std::make_shared<DataStorageClass>(std::move(mockStorageHAL));
  return EventManager(modalLights, configs,deviceTime, dataStorage->getAllEvents());
}

auto makeTestConfigManager(EventManagerConfigsStruct initialConfigs){
  ConfigsStruct configs;
  configs.eventConfigs = initialConfigs;
  auto mockConfigHal = std::make_unique<MockConfigHal>();
  mockConfigHal->setConfigs(configs);
  std::shared_ptr<ConfigManagerClass> configsManager = std::make_shared<ConfigManagerClass>(std::move(mockConfigHal));
  return configsManager;
};

auto makeTestConfigManager(){
  EventManagerConfigsStruct defaultConfigs;
  return makeTestConfigManager(defaultConfigs);
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
  std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
  std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);

  std::vector<EventDataPacket> emptyEvents = {};
  // initialising with no events shouldn't cause an issue
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  
  deviceTime->setLocalTimestamp2000(mondayAtMidnight, 0, 0);
  EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, emptyEvents);
  TEST_ASSERT_EQUAL(0, testClass.getNextEvent().ID);
  TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());
  TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());

  // checking with no events shouldn't cause an issue
  deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(6, 0, 0), 0, 0);
  testClass.check();
  TEST_ASSERT_EQUAL(0, testClass.getNextEvent().ID);
  TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());
  TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());

  // adding a good event should work fine
  deviceTime->setLocalTimestamp2000(mondayAtMidnight + testEvent1.timeOfDay +1, 0, 0);
  testClass.addEvent(testEvent1);
  TEST_ASSERT_EQUAL(testEvent1.eventID, testClass.getNextEvent().ID);
  TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());
  TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
}

void EventManagerFindsNextEventOnConstruction(void){
  std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
  std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);

  uint64_t morningTimestamp = 794281114; // Monday 1:38:34 3/3/25 epoch=2000
  uint64_t eveningTimestamp = 794252745; // Sunday 17:45:45 2/3/25
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  // testEvent1, morning
  {
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    std::vector<EventDataPacket> testEvents = {testEvent1};
    for(int i = 0; i < 5; i++){
      modalLights->resetInstance();
      deviceTime->setLocalTimestamp2000(morningTimestamp + secondsInDay*i, 0, 0);
      EventManager testClass1 = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
      TEST_ASSERT_EQUAL(expectedTimestamp + secondsInDay*i, testClass1.getNextEvent().triggerTime);
      TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));
      deviceTime->remove_observer(testClass1);
    }
    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    modalLights->resetInstance();
    deviceTime->setLocalTimestamp2000(morningTimestamp + secondsInDay*5, 0, 0);
    EventManager testClass2 = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEvent().triggerTime);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));

    deviceTime->remove_observer(testClass2);
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(morningTimestamp + secondsInDay*6, 0, 0);
    EventManager testClass3 = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass3.getNextEvent().triggerTime);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));

    deviceTime->remove_observer(testClass3);
  }
  // testEvent1, evening
  {
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    std::vector<EventDataPacket> testEvents = {testEvent1};
    for(int i = 0; i < 5; i++){
      modalLights->resetInstance();
      
      deviceTime->setLocalTimestamp2000(eveningTimestamp + secondsInDay*i, 0, 0);
      EventManager testClass1 = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
      TEST_ASSERT_EQUAL(expectedTimestamp + secondsInDay*i, testClass1.getNextEvent().triggerTime);

      deviceTime->remove_observer(testClass1);
    }
    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    modalLights->resetInstance();

    
    deviceTime->setLocalTimestamp2000(eveningTimestamp + secondsInDay*5, 0, 0);
    EventManager testClass2 = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEvent().triggerTime);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));

    deviceTime->remove_observer(testClass2);
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(eveningTimestamp + secondsInDay*6, 0, 0);
    EventManager testClass3 = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass3.getNextEvent().triggerTime);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));
    deviceTime->remove_observer(testClass3);
  }
  // testEvent1, just after alarm
  {
    uint64_t justAfterAlarm = 794299557;   // Monday 06:45:57 3/3/25
    uint64_t expectedTimestamp = 794299500; // Monday 6:45
    std::vector<EventDataPacket> testEvents = {testEvent1};
    for(int i = 0; i < 4; i++){
      // mon-thu
      modalLights->resetInstance();
      
      deviceTime->setLocalTimestamp2000(justAfterAlarm + secondsInDay*i, 0, 0);
      EventManager testClass1 = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
      TEST_ASSERT_EQUAL(expectedTimestamp + secondsInDay*(i+1), testClass1.getNextEvent().triggerTime);
      TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testEvent1.modeID));

      deviceTime->remove_observer(testClass1);
    }
    // friday
    modalLights->resetInstance();
    uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    
    deviceTime->setLocalTimestamp2000(justAfterAlarm + secondsInDay*4, 0, 0);
    EventManager testClass2 = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEvent().triggerTime);
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testEvent1.modeID));

    // saturday
    deviceTime->remove_observer(testClass2);
    modalLights->resetInstance();
    // uint64_t secondExpectedTimestamp = expectedTimestamp + 7 * secondsInDay;
    
    deviceTime->setLocalTimestamp2000(justAfterAlarm + secondsInDay*5, 0, 0);
    EventManager testClass3 = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass2.getNextEvent().triggerTime);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));

    //sunday
    deviceTime->remove_observer(testClass3);
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(justAfterAlarm + secondsInDay*6, 0, 0);
    EventManager testClass4 = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(secondExpectedTimestamp, testClass3.getNextEvent().triggerTime);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.modeID));
    deviceTime->remove_observer(testClass4);
  }
}

void EventManagerSelectsCorrectBackgroundMode(void){
  std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
  std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);
  OnboardTimestamp onboardTimestamp;

  // shove in some background modes only, test accross different times of day on different days
  std::vector<EventDataPacket> testEvents = {testEvent3, testEvent4, testEvent5};
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
      int t = 0;
      for(const auto& [time, expectedMode] : testTimesAndExpectedModes){
        modalLights->resetInstance();
        uint64_t testTimestamp = mondayAtMidnight + time + i*secondsInDay;

        std::string message = "day = " + std::to_string(i) +"; t = " + std::to_string(t) + "; testTimestamp = " + std::to_string(testTimestamp);
        
        deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
        EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
        TEST_ASSERT_EQUAL_MESSAGE(expectedMode, modalLights->getBackgroundMode(), message.c_str());
        TEST_ASSERT_EQUAL(expectedMode, modalLights->getMostRecentMode());
        TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(expectedMode));
        TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());
        deviceTime->remove_observer(testClass);
        t++;
      }
    }
  }

  // test switching. First test also tests behaviour with repeated timestamp checks
  {
    uint8_t expectedSetCount = 0;
    modeUUID previousMode = 0;
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(mondayAtMidnight, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    for(int i = 0; i < 7; i++){
      int i_map = 0;
      for(const auto& [time, expectedMode] : testTimesAndExpectedModes){
        const uint64_t testTimestamp = mondayAtMidnight + time + i*secondsInDay;
        onboardTimestamp.setTimestamp_S(testTimestamp);
        testClass.check();
        uint8_t setModeCount = modalLights->getSetModeCount();
        TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
        TEST_ASSERT_EQUAL(expectedMode, modalLights->getBackgroundMode());
        if(expectedMode != previousMode){
          expectedSetCount++; previousMode = expectedMode;
        }
        TEST_ASSERT_EQUAL(expectedSetCount, modalLights->getSetModeCount());
        i_map++;
      }
    }
    deviceTime->remove_observer(testClass);
  }

  // eventWindow is ignored for background modes
  {
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(8, 59, 0), 0, 0);
    modalLights->resetInstance();
    
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());

    uint64_t eventWindow = testEvent5.eventWindow;
    TEST_ASSERT(eventWindow > hardwareMinimumEventWindow);  // just in case it gets changed for some reason
    // deviceTime->setLocalTimestamp2000(mondayAtMidnight + testEvent5.timeOfDay + eventWindow + 10, 0, 0);
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + testEvent5.timeOfDay + eventWindow + 10);
    testClass.check();
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getMostRecentMode());
    deviceTime->remove_observer(testClass);
  }
}

void onlyMostRecentActiveModeIsTriggered(void){
  std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
  std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);

  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  

  // on construction
  {
    modalLights->resetInstance();
    std::vector<EventDataPacket> testEvents = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent6};
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    // TEST_ASSERT_EQUAL(5, modalLights->getMostRecentMode());
    // TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
    // TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(3));
    // TEST_ASSERT_EQUAL(5, modalLights->getActiveMode());
    // TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(5));
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(2, modalLights->getSetModeCount());
    deviceTime->remove_observer(testClass);
  }

  // every day of the week
  {
    std::vector<EventDataPacket> testEvents = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent6, testEvent10};
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight;
    OnboardTimestamp onboardTimestamp;
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    // weekday
    for(int i = 0; i < 5; i++){
      std::string message = "day = " + std::to_string(i + 1);
      onboardTimestamp.setTimestamp_S(testTimestamp + i*secondsInDay + timeToSeconds(6, 45, 0));
      testClass.check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent1.modeID, modalLights->getActiveMode(), message.c_str());

      onboardTimestamp.setTimestamp_S(testTimestamp + i*secondsInDay + timeToSeconds(7, 0, 0));
      testClass.check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent6.modeID, modalLights->getActiveMode(), message.c_str());

      onboardTimestamp.setTimestamp_S(testTimestamp + i*secondsInDay + timeToSeconds(22, 0, 0));
      testClass.check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent10.modeID, modalLights->getActiveMode(), message.c_str());
    }

    // weekend
    for(int d = 5; d < 7; d++){
      std::string message = "day = " + std::to_string(d + 1);

      onboardTimestamp.setTimestamp_S(testTimestamp + d*secondsInDay + timeToSeconds(10, 0, 0));
      testClass.check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent2.modeID, modalLights->getActiveMode(), message.c_str());

      onboardTimestamp.setTimestamp_S(testTimestamp + d*secondsInDay + timeToSeconds(22, 0, 0));
      testClass.check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent10.modeID, modalLights->getActiveMode(), message.c_str());
    }
    deviceTime->remove_observer(testClass);
  }
}

void activeAndBackgroundModesAreBothTriggered(void){
  std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
  std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);
  // the active mode should be called first, then the background mode
  std::vector<EventDataPacket> testEvents = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent6, testEvent7};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  OnboardTimestamp onboardTimestamp;

  uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
  // on construction
  {
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(6));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(5));
    TEST_ASSERT_EQUAL(6, modalLights->getMostRecentMode()); // won't be active, but should be called second
    deviceTime->remove_observer(testClass);
  }

  // on time change
  {
    
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(6, 45, 0), 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    modalLights->resetInstance();
    // deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    onboardTimestamp.setTimestamp_S(testTimestamp);
    testClass.check();
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(5));
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(6));
    TEST_ASSERT_EQUAL(2, modalLights->getSetModeCount());
    deviceTime->remove_observer(testClass);
  }

  // when background mode has trigger time before active mode
  {
    // on construction
    std::vector<EventDataPacket> testEvents2 = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent10};
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents2);
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(22, 0, 1));
    testClass.check();
    TEST_ASSERT_EQUAL(testEvent10.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());
    deviceTime->remove_observer(testClass);
  }
}

void missedActiveEventsAreSkippedAfterTimeWindowClosses(void){
  std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
  std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);

  std::vector<EventDataPacket> testEvents = {testEvent1, testEvent7};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  OnboardTimestamp onboardTimestamp;

  uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 0, 0);
  
  deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
  EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
  modalLights->resetInstance();
  onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(12, 0, 0));
  testClass.check();
  TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
  TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

  onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(12, 0, 0) + secondsInDay);
  testClass.check();
  TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(2));
  deviceTime->remove_observer(testClass);
}

void addEventChecksTheNewEvent(void){
  std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
  std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);

  std::vector<EventDataPacket> testEvents = {testEvent7};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  

  uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 46, 0);
  
  OnboardTimestamp onboardTimestamp;
  deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
  EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);

  // active event should trigger when added
  modalLights->resetInstance();
  eventError_t error = testClass.addEvent(testEvent1);
  TEST_ASSERT_EQUAL(EventManagerErrors::success, error);
  TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(2));
  TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
  TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());

  // active event shouldn't trigger when added
  modalLights->resetInstance();
  TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.addEvent(testEvent8));
  TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(7));
  // TEST_ASSERT_EQUAL(1, modalLights->getSetModeCount());

  // background event should trigger
  uint64_t testTimestamp2 = mondayAtMidnight + timeToSeconds(23, 59, 59);
  onboardTimestamp.setTimestamp_S(testTimestamp2);
  testClass.check();
  modalLights->resetInstance();
  TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.addEvent(testEvent4));
  TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

  // background event shouldn't trigger
  modalLights->resetInstance();
  TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.addEvent(testEvent3));
  TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent3.modeID));

  // bad uuid should return error
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_uuid, testClass.addEvent(testEvent3));
  EventDataPacket badEventUUID = testEvent2;
  badEventUUID.eventID = 0;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_uuid, testClass.addEvent(badEventUUID));
  EventDataPacket badModeUUID = testEvent2;
  badModeUUID.modeID = 0;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_uuid, testClass.addEvent(badModeUUID));

  // bad time should return error
  EventDataPacket badTimeOfDay = testEvent2;
  badTimeOfDay.timeOfDay = secondsInDay + 1;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_time, testClass.addEvent(badTimeOfDay));
  EventDataPacket badDaysOfWeek = testEvent2;
  badDaysOfWeek.daysOfWeek = 0;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_time, testClass.addEvent(badDaysOfWeek));
  EventDataPacket badEventWindow = testEvent2;
  badEventWindow.eventWindow = secondsInDay + 1;
  TEST_ASSERT_EQUAL(EventManagerErrors::bad_time, testClass.addEvent(badEventWindow));

  deviceTime->remove_observer(testClass);

  // start with an active event that's just triggered. add an active event that should have triggered after the existing active event. the new event should be active
  {
    modalLights->resetInstance();
    std::vector<EventDataPacket> testEvents2 = {testEvent1, testEvent2, testEvent3, testEvent4};
    deviceTime->setLocalTimestamp2000(mondayAtMidnight+timeToSeconds(7, 0, 1));
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents2);

    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    eventManager.addEvent(testEvent6);
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    eventManager.addEvent(testEvent7);
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

    deviceTime->remove_observer(eventManager);
  }

  // start with an active event that's just triggered. add an active event that's still within its event window, but before the current active event. the new active event should not have triggered
  {
    modalLights->resetInstance();
    std::vector<EventDataPacket> testEvents3 = {testEvent2, testEvent3, testEvent6, testEvent7};
    deviceTime->setLocalTimestamp2000(mondayAtMidnight+timeToSeconds(7, 0, 1));
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents3);

    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    
    eventManager.addEvent(testEvent1);
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

    eventManager.addEvent(testEvent4);
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

    deviceTime->remove_observer(eventManager);
  }

  TEST_IGNORE_MESSAGE("important tests need to be added");
}

void eventWindow0ShouldUseSystemDefault(void){
  std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
  std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);
  OnboardTimestamp onboardTimestamp;

  // start configs with eventWindow = 1 hour
  
  EventManagerConfigsStruct configStruct;
  configStruct.defaultEventWindow_S = 60*60;
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
    
    deviceTime->setLocalTimestamp2000(testTimestamp + (configStruct.defaultEventWindow_S/2), 0, 0);
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(testDefaultActiveEvent.modeID, modalLights->getActiveMode());
    
    
    // construct with eventWindow = 0 and time = triggerTime - 1 second
    // skip forward 1 minute
    deviceTime->remove_observer(eventManager);
    modalLights->resetInstance();
    // 
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager eventManager2 = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));

    onboardTimestamp.setTimestamp_S(testTimestamp + (configStruct.defaultEventWindow_S/2));
    eventManager2.check();

    TEST_ASSERT_EQUAL(testDefaultActiveEvent.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));
    deviceTime->remove_observer(eventManager2);
  }

  // addEvent should use default event window
  {
    // change default eventWindow to 30 minutes
    configStruct.defaultEventWindow_S = 30*60;
    configs->setEventManagerConfigs(configStruct);

    std::vector<EventDataPacket> testEvents2 = {testEvent4};
    
    // should trigger during eventWindow
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents2);
    // add the event before it's trigger window
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + testDefaultActiveEvent.timeOfDay - 1, 0, 0);
    eventManager.addEvent(testDefaultActiveEvent);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));

    onboardTimestamp.setTimestamp_S(mondayAtMidnight + testDefaultActiveEvent.timeOfDay + configStruct.defaultEventWindow_S -10);

    eventManager.check();
    TEST_ASSERT_EQUAL(1, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));
    TEST_ASSERT_EQUAL(testDefaultActiveEvent.modeID, modalLights->getMostRecentMode());


    // shouldn't trigger after the window
    deviceTime->remove_observer(eventManager);
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager eventManager2 = EventManagerFactory(modalLights, configs, deviceTime, testEvents2);
    // time of alarm + defaultEventWindow (30 mins) - 10 seconds
    const uint64_t checkingTimestamp = mondayAtMidnight + testDefaultActiveEvent.timeOfDay + configStruct.defaultEventWindow_S + 10;
    // add the event after it's trigger window
    deviceTime->setLocalTimestamp2000(checkingTimestamp, 0, 0);
    eventManager2.addEvent(testDefaultActiveEvent);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testDefaultActiveEvent.modeID));

    // if defaultEventWindow changes such that a missed event should be triggered, rebuildTriggerTimes should trigger the missed event
    modalLights->resetInstance();
    // change the defaultEventWindow to 1 hour
    EventManagerConfigsStruct newConfigs = eventManager2.getConfigs();
    newConfigs.defaultEventWindow_S = 60*60;
    TEST_ASSERT_EQUAL(EventManagerErrors::success, eventManager2.setConfigs(newConfigs));
    // check the event can trigger
    modalLights->resetInstance();
    eventManager2.rebuildTriggerTimes();
    TEST_ASSERT_EQUAL(testDefaultActiveEvent.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(2, modalLights->getSetModeCount()); // active + background should be called
    deviceTime->remove_observer(eventManager2);
  }
}

void testRemoveEvent(void){
  std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
  std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);
  OnboardTimestamp onboardTimestamp;

  std::vector<EventDataPacket> testEvents = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  

  // remove a single background event
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(22, 0, 0);
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getMostRecentMode());
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());

    testClass.removeEvent(testEvent3.eventID);
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());
    deviceTime->remove_observer(testClass);
  }

  // remove a single active event at trigger time
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 44, 0);
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);

    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(6, 50, 0));
    testClass.removeEvent(testEvent1.eventID);
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    deviceTime->remove_observer(testClass);
  }

  // remove a single active event during trigger
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 50, 0);
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());

    testClass.removeEvent(testEvent1.eventID);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.eventID));
    // TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    // TEST_ASSERT_EQUAL(1, modalLights->getCancelCallCount(testEvent1.modeID));
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    deviceTime->remove_observer(testClass);
  }

  // remove array of background events
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(22, 0, 0);
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getMostRecentMode());

    eventUUID removeEvents[2] = {testEvent3.eventID, testEvent5.eventID};
    testClass.removeEvents(removeEvents, 2);
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    deviceTime->remove_observer(testClass);
  }

  // remove array of active events at trigger time
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 44, 0);
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);

    eventUUID removeEvents[2] = {testEvent1.eventID, testEvent2.eventID};
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(6, 50, 0));
    testClass.removeEvents(removeEvents, 2);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.eventID));
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    // TEST_ASSERT_EQUAL(2, modalLights->getCancelCallCount(testEvent1.modeID)); // event1 and event2 share a modeID
    deviceTime->remove_observer(testClass);
  }

  // remove active events during trigger time
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 50, 0);
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);

    eventUUID removeEvents[2] = {testEvent1.eventID, testEvent2.eventID};
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(6, 50, 0));
    testClass.removeEvents(removeEvents, 2);
    TEST_ASSERT_EQUAL(0, modalLights->getModeCallCount(testEvent1.eventID));
    // TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getMostRecentMode());
    // TEST_ASSERT_GREATER_OR_EQUAL(1, modalLights->getCancelCallCount(testEvent1.modeID));
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    deviceTime->remove_observer(testClass);
  }

  // removing an active event during trigger time doesn't trigger the previous active event even though its still within its time window
  {
    modalLights->resetInstance();
    // construct @7am with testEvent9 and testEvent1, cancel testEvent9 and check testEvent1 doesn't trigger
    std::vector<EventDataPacket> testEvents2 = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent9};
    uint64_t testTimeestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
    
    deviceTime->setLocalTimestamp2000(testTimeestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents2);
    TEST_ASSERT_EQUAL(testEvent9.modeID, modalLights->getActiveMode());

    testClass.removeEvent(testEvent9.eventID);
    // TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    deviceTime->remove_observer(testClass);
  }

  // clearing all background events should call defaultConstBrightness
  {
    std::vector<EventDataPacket> testEvents2 = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent7};
    uint64_t testTimestamp = mondayAtMidnight;
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents2);
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.removeEvent(testEvent4.eventID));
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.removeEvent(testEvent3.eventID));
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.removeEvent(testEvent5.eventID));
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

    // remove the last event
    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.removeEvent(testEvent7.eventID));
    TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());

    deviceTime->remove_observer(testClass);
  }

  // all events can be cleared
  {
    std::vector<EventDataPacket> testEvents2 = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent7};
    uint64_t testTimestamp = mondayAtMidnight;
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents2);
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.removeEvent(testEvent4.eventID));
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.removeEvent(testEvent3.eventID));
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.removeEvent(testEvent5.eventID));
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

    // remove the last background event
    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.removeEvent(testEvent7.eventID));
    TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());

    TEST_ASSERT_EQUAL(testEvent1.eventID, testClass.getNextActiveEvent().ID);
    TEST_ASSERT_EQUAL(testEvent1.eventID, testClass.getNextEvent().ID);

    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.removeEvent(testEvent1.eventID));
    TEST_ASSERT_EQUAL(testEvent2.eventID, testClass.getNextEvent().ID);

    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.removeEvent(testEvent2.eventID));
    TEST_ASSERT_EQUAL(0, testClass.getNextEvent().ID);
    TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());

    deviceTime->remove_observer(testClass);
  }
}

void testUpdateEvent(void){
  std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
  std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);
  std::vector<EventDataPacket> testEvents = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5};
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  OnboardTimestamp onboardTimestamp;
  

  // change active event to trigger at current time
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 35, 0);
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());

    EventDataPacket newEvent1 = testEvent1;
    newEvent1.timeOfDay = timeToSeconds(6, 30, 0);
    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.updateEvent(newEvent1));
    TEST_ASSERT_EQUAL(newEvent1.modeID, modalLights->getActiveMode());
    deviceTime->remove_observer(testClass);
  }

  // change triggered active event to latter time
  {
    // change active event to trigger at current time
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 50, 0);
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());

    EventDataPacket newEvent1 = testEvent1;
    newEvent1.timeOfDay = timeToSeconds(7, 30, 0);
    testClass.updateEvent(newEvent1);
    TEST_ASSERT_EQUAL(mondayAtMidnight + timeToSeconds(7, 30, 0), testClass.getNextEvent().triggerTime);
    // i don't think I want to cancel the mode. the interface that updates the events should have a force-cancel function
    // TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    // TEST_ASSERT_EQUAL(1, modalLights->getCancelCallCount(newEvent1.modeID));
    deviceTime->remove_observer(testClass);
  }

  // change current background event to future
  {
    modalLights->resetInstance();
    uint64_t  testTimestamp = mondayAtMidnight + timeToSeconds(21, 0, 10);
    std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);

    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());

    EventDataPacket newEvent3 = testEvent3;
    newEvent3.timeOfDay = timeToSeconds(21, 30, 0);
    testClass.updateEvent(newEvent3);
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(newEvent3.eventID, testClass.getNextEvent().ID);
    TEST_ASSERT_EQUAL(mondayAtMidnight + timeToSeconds(21, 30, 0), testClass.getNextEvent().triggerTime);
    deviceTime->remove_observer(testClass);
  }
  
  // change future background event to past
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(20, 32, 0);
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());

    EventDataPacket newEvent3 = testEvent3;
    newEvent3.timeOfDay = timeToSeconds(20, 30, 0);
    testClass.updateEvent(newEvent3);
    TEST_ASSERT_EQUAL(newEvent3.modeID, modalLights->getBackgroundMode());
    
    deviceTime->remove_observer(testClass);
  }

  // test updating an array of events
  {
    modalLights->resetInstance();
    std::vector<EventDataPacket> testEvents2 = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent9};
    uint64_t testTimestamp1 = mondayAtMidnight + timeToSeconds(6, 35, 0);
    
    deviceTime->setLocalTimestamp2000(testTimestamp1, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents2);
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
    // deviceTime->setLocalTimestamp2000(testTimestamp2, 0, 0);
    onboardTimestamp.setTimestamp_S(testTimestamp2);
    testClass.updateEvents(updateEvents, actualErrors, numberOfUpdates);
    TEST_ASSERT_EQUAL_INT8_ARRAY(expectedErrors, actualErrors, numberOfUpdates);
    TEST_ASSERT_EQUAL(newEvent9.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(newEvent5.modeID, modalLights->getBackgroundMode());

    uint64_t testTimestamp3 = mondayAtMidnight + timeToSeconds(7, 31, 0);
    // deviceTime->setLocalTimestamp2000(testTimestamp3, 0, 0);
    onboardTimestamp.setTimestamp_S(testTimestamp3);
    testClass.check();
    TEST_ASSERT_EQUAL(newEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(newEvent5.modeID, modalLights->getBackgroundMode());

    uint64_t testTimestamp4 = mondayAtMidnight + timeToSeconds(20, 31, 0);
    // deviceTime->setLocalTimestamp2000(testTimestamp4, 0, 0);
    onboardTimestamp.setTimestamp_S(testTimestamp4);
    testClass.check();
    TEST_ASSERT_EQUAL(newEvent3.modeID, modalLights->getBackgroundMode());
    deviceTime->remove_observer(testClass);
  }

  // test changing an active event to background
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(6, 50, 0);
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    modalLights->resetInstance();
    EventDataPacket newEvent1 = testEvent1;
    newEvent1.isActive = false;
    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.updateEvent(newEvent1));
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(newEvent1.modeID, modalLights->getBackgroundMode());

    deviceTime->remove_observer(testClass);
  }

  // test changing a background mode to active
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(21, 0, 3);
    std::vector<EventDataPacket> testEvents2 = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent12};
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents2);
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());

    modalLights->resetInstance();
    EventDataPacket newEvent3 = testEvent3;
    newEvent3.isActive = true;
    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.updateEvent(newEvent3));
    TEST_ASSERT_EQUAL(newEvent3.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent12.modeID, modalLights->getBackgroundMode());

    deviceTime->remove_observer(testClass);
  }

  // test changing the only background mode to active
  {
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(10, 0, 3);
    std::vector<EventDataPacket> testEvents2 = {testEvent1, testEvent2, testEvent12};
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents2);
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent12.modeID, modalLights->getBackgroundMode());

    EventDataPacket newEvent12 = testEvent12;
    newEvent12.isActive = true;
    TEST_ASSERT_EQUAL(EventManagerErrors::success, testClass.updateEvent(newEvent12));
    TEST_ASSERT_EQUAL(newEvent12.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(1, modalLights->getBackgroundMode());

    deviceTime->remove_observer(testClass);
  }
}

void eventSkipping(void){
  TEST_IGNORE();
}

void testConfigChanges(void){
  // changes to EventManager configs should be done through EventManager
  TEST_FAIL_MESSAGE("not yet implemented, but very important");
}

void testTimeUpdates(void){
  const std::vector<EventDataPacket> testEvents = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent6, testEvent7, testEvent11};
  const EventManagerConfigsStruct testConfigs{
    .defaultEventWindow_S = 10*60   // 10 minutes
  };
  std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager(testConfigs);
  OnboardTimestamp onboardTimestamp;
  onboardTimestamp.setTimestamp_S(mondayAtMidnight);
  std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);
  deviceTime->setLocalTimestamp2000(mondayAtMidnight, 0, 0);

  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  
  // negative time changes < minimumEventWindow should rebuild trigger times without checking missed active events
  {
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents);

    // start at Monday 7am. testEvent7 should be background and testEvent6 should be active.
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(7, 0, 0));
    eventManager.check();
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());

    // change time to 6:59:01. testEvent4 should be background and testEvent6 should be active
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(6, 59, 1), 0, 0);
    // eventManager should rebuild event times in the observer callback to avoid collisions with methods such as addEvent(). current timestamp is passed with updates, so other observers being held up shouldn't matter
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());

    // set time to 7am. testEvent7 should be background, and testEvent6 shouldn't retrigger
    modalLights->resetInstance(); // effectively cancels active mode
    eventManager.check(); // events were already called, so this should do nothing
    TEST_ASSERT_EQUAL(0, modalLights->getSetModeCount());

    // const uint8_t expActiveCalls = modalLights->getModeCallCount(testEvent7.modeID);
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(7, 0, 0));
    eventManager.check();
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

    deviceTime->remove_observer(eventManager);
  }

  // negative time changes > smallTimeAdjustmentWindow_m should rebuild events and check missed active
  {
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(9, 0, 0), 0, 0);
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents);

    // set time to 9am. testEvent5 should be background and testEvent11 should be active.
    TEST_ASSERT_EQUAL(testEvent11.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());

    // change time to 7:00:01 am. testEvent4 should be background and testEvent1 should be active
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(7, 0, 1));
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

    // fast forward to 8:30am. testEvent7 should be still be background and testEvent11 should trigger again.
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(8, 30, 0));
    eventManager.check();
    TEST_ASSERT_EQUAL(testEvent11.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());

    deviceTime->remove_observer(eventManager);
  } 

  // two small negative changes less than one big negative change shouldn't cause skipped events to trigger
  {
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(6, 59, 59), 0, 0);
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    
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
    eventManager.check();
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());

    deviceTime->remove_observer(eventManager);
  }

  // small negative changes don't effect active events beyond the change time
  {
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(6, 59, 59), 0, 0);
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents);

    // set time to 6:59:59am. testEvent4 should be background and testEvent1 should be active
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());

    // change time to 6:59:00am, then set time to 7am. testEvent7 should be background and testEvent6 should be active
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(6, 59, 0), 0, 0);
    onboardTimestamp.setTimestamp_S(mondayAtMidnight + timeToSeconds(7, 0, 0));
    eventManager.check();
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());

    deviceTime->remove_observer(eventManager);
  }

  // positive time changes less than event window should be ignored
  {
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(7, 0, 0), 0, 0);
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents);

    // set time to 7am. testEvent7 should be background and testEvent6 should be active.
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());

    // change time to 7:01:00. modalLights shouldn't have been called
    const uint8_t callCount = modalLights->getSetModeCount();
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(7, 1, 0), 0, 0);
    eventManager.check();
    TEST_ASSERT_EQUAL(callCount, modalLights->getSetModeCount());

    deviceTime->remove_observer(eventManager);
  }

  // positive time changes greater than event window should rebuild events checking for missed events
  {
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(mondayAtMidnight, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(9, 0, 1), 0, 0);
    // testClass.check();
    TEST_ASSERT_TRUE(testEvent11.eventWindow > testConfigs.defaultEventWindow_S); // test that the rebuilding is done properly
    TEST_ASSERT_EQUAL(testEvent11.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent5.modeID, modalLights->getBackgroundMode());

    // skip over the daytime modes
    modalLights->resetInstance();
    deviceTime->setUTCTimestamp2000(mondayAtMidnight + timeToSeconds(21, 0, 0), 0, oneHour);
    TEST_ASSERT_EQUAL(0, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());

    deviceTime->remove_observer(testClass);
  }

  // a small positive time adjustment that triggers an active mode, followed by a small negative adjustment
  {
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(6, 59, 59), 0, 0);
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents);

    // start at 6:59:59am. testEvent4 should be background and testEvent1 should be active
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());

    // set time to 7am. testEvent7 should be background and testEvent6 should be active.
    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(7, 0, 0), 0, 0);
    TEST_ASSERT_EQUAL(testEvent7.modeID, modalLights->getBackgroundMode());
    TEST_ASSERT_EQUAL(testEvent6.modeID, modalLights->getActiveMode());

    deviceTime->remove_observer(eventManager);
  }

  // changing the time to an event trigger time should trigger those events
  {
    std::vector<EventDataPacket> testEvents = {testEvent1,
      testEvent2, testEvent3, testEvent4, testEvent5, testEvent6, testEvent10};
    modalLights->resetInstance();
    uint64_t testTimestamp = mondayAtMidnight;
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
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
      // testClass.check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent2.modeID, modalLights->getActiveMode(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(testEvent5.modeID, modalLights->getBackgroundMode(), message.c_str());

      deviceTime->setLocalTimestamp2000(testTimestamp + d*secondsInDay + timeToSeconds(22, 0, 0));
      testClass.check();
      TEST_ASSERT_EQUAL_MESSAGE(testEvent10.modeID, modalLights->getActiveMode(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(testEvent3.modeID, modalLights->getBackgroundMode(), message.c_str());
    }
    deviceTime->remove_observer(testClass);
  }
  

  {
    // on construction
    uint64_t testTimestamp = mondayAtMidnight + timeToSeconds(7, 0, 0);
    std::vector<EventDataPacket> testEvents2 = {testEvent1, testEvent3, testEvent4, testEvent5, testEvent10};
    modalLights->resetInstance();
    
    deviceTime->setLocalTimestamp2000(testTimestamp, 0, 0);
    EventManager testClass = EventManagerFactory(modalLights, configs, deviceTime, testEvents2);
    TEST_ASSERT_EQUAL(testEvent1.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent4.modeID, modalLights->getBackgroundMode());

    deviceTime->setLocalTimestamp2000(mondayAtMidnight + timeToSeconds(22, 0, 1), 0, 0);
    testClass.check();
    TEST_ASSERT_EQUAL(testEvent10.modeID, modalLights->getActiveMode());
    TEST_ASSERT_EQUAL(testEvent3.modeID, modalLights->getBackgroundMode());
    deviceTime->remove_observer(testClass);
  }

  // TODO: small negative change followed by a small negative change should act like a big negative change if changes add to a big change

  // TODO: a large positive then large negative time adjustment that adds to a small adjustment should act like large time adjustments
}

void testEventLimit(){
  // test that the number of stored events cannot exceed a pre-defined limit

  TEST_ASSERT_TRUE(MAX_NUMBER_OF_EVENTS > 10);
  TEST_ASSERT_TRUE(MAX_NUMBER_OF_EVENTS < 254);
  // background events can't exceed limit
  {
    std::vector<EventDataPacket> testEvents = {};
    std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
    std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);
    deviceTime->setLocalTimestamp2000(mondayAtMidnight);
    std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents);

    for(uint32_t i = 0; i < MAX_NUMBER_OF_EVENTS; i++){
      EventDataPacket packet{
        .eventID = static_cast<eventUUID>(i + 1),
        .modeID = static_cast<modeUUID>(i + 1),
        .timeOfDay = i,
        .daysOfWeek = 0b01111111,
        .eventWindow = 15*60,
        .isActive = false
      };
      TEST_ASSERT_EQUAL(EventManagerErrors::success, eventManager.addEvent(packet));
    }

    EventDataPacket activePacket{
      .eventID = static_cast<eventUUID>(MAX_NUMBER_OF_EVENTS + 2),
      .modeID = static_cast<modeUUID>(MAX_NUMBER_OF_EVENTS + 2),
      .timeOfDay = MAX_NUMBER_OF_EVENTS,
      .daysOfWeek = 0b01111111,
      .eventWindow = 15*60,
      .isActive = true
    };
    TEST_ASSERT_EQUAL(EventManagerErrors::storage_full, eventManager.addEvent(activePacket));

    EventDataPacket backgroundPacket{
      .eventID = static_cast<eventUUID>(MAX_NUMBER_OF_EVENTS + 1),
      .modeID = static_cast<modeUUID>(MAX_NUMBER_OF_EVENTS + 1),
      .timeOfDay = MAX_NUMBER_OF_EVENTS + 1,
      .daysOfWeek = 0b01111111,
      .eventWindow = 15*60,
      .isActive = false
    };
    TEST_ASSERT_EQUAL(EventManagerErrors::storage_full, eventManager.addEvent(backgroundPacket));
  }

  // active events can't exceed limit
  {
    std::vector<EventDataPacket> testEvents = {};
    std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
    std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);
    deviceTime->setLocalTimestamp2000(mondayAtMidnight);
    std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents);

    for(uint32_t i = 0; i < MAX_NUMBER_OF_EVENTS; i++){
      EventDataPacket packet{
        .eventID = static_cast<eventUUID>(i + 1),
        .modeID = static_cast<modeUUID>(i + 1),
        .timeOfDay = i,
        .daysOfWeek = 0b01111111,
        .eventWindow = 15*60,
        .isActive = true
      };
      TEST_ASSERT_EQUAL(EventManagerErrors::success, eventManager.addEvent(packet));
    }

    EventDataPacket activePacket{
      .eventID = static_cast<eventUUID>(MAX_NUMBER_OF_EVENTS + 2),
      .modeID = static_cast<modeUUID>(MAX_NUMBER_OF_EVENTS + 2),
      .timeOfDay = MAX_NUMBER_OF_EVENTS,
      .daysOfWeek = 0b01111111,
      .eventWindow = 15*60,
      .isActive = true
    };
    TEST_ASSERT_EQUAL(EventManagerErrors::storage_full, eventManager.addEvent(activePacket));

    EventDataPacket backgroundPacket{
      .eventID = static_cast<eventUUID>(MAX_NUMBER_OF_EVENTS + 1),
      .modeID = static_cast<modeUUID>(MAX_NUMBER_OF_EVENTS + 1),
      .timeOfDay = MAX_NUMBER_OF_EVENTS + 1,
      .daysOfWeek = 0b01111111,
      .eventWindow = 15*60,
      .isActive = false
    };
    TEST_ASSERT_EQUAL(EventManagerErrors::storage_full, eventManager.addEvent(backgroundPacket));
  }

  // total events can't exceed limit
  {
    std::vector<EventDataPacket> testEvents = {};
    std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
    std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);
    deviceTime->setLocalTimestamp2000(mondayAtMidnight);
    std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
    EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents);

    for(uint32_t i = 0; i < MAX_NUMBER_OF_EVENTS; i++){
      EventDataPacket packet{
        .eventID = static_cast<eventUUID>(i + 1),
        .modeID = static_cast<modeUUID>(i + 1),
        .timeOfDay = i,
        .daysOfWeek = 0b01111111,
        .eventWindow = 15*60,
        .isActive = (i%2 == 1)
      };
      TEST_ASSERT_EQUAL(EventManagerErrors::success, eventManager.addEvent(packet));
    }

    EventDataPacket activePacket{
      .eventID = static_cast<eventUUID>(MAX_NUMBER_OF_EVENTS + 2),
      .modeID = static_cast<modeUUID>(MAX_NUMBER_OF_EVENTS + 2),
      .timeOfDay = MAX_NUMBER_OF_EVENTS,
      .daysOfWeek = 0b01111111,
      .eventWindow = 15*60,
      .isActive = true
    };
    TEST_ASSERT_EQUAL(EventManagerErrors::storage_full, eventManager.addEvent(activePacket));

    EventDataPacket backgroundPacket{
      .eventID = static_cast<eventUUID>(MAX_NUMBER_OF_EVENTS + 1),
      .modeID = static_cast<modeUUID>(MAX_NUMBER_OF_EVENTS + 1),
      .timeOfDay = MAX_NUMBER_OF_EVENTS + 1,
      .daysOfWeek = 0b01111111,
      .eventWindow = 15*60,
      .isActive = false
    };
    TEST_ASSERT_EQUAL(EventManagerErrors::storage_full, eventManager.addEvent(backgroundPacket));
  }
}

void testEventWindowLimits(){
  std::vector<EventDataPacket> testEvents = getAllTestEvents();
  std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
  std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);
  deviceTime->setLocalTimestamp2000(mondayAtMidnight);
  std::shared_ptr<MockModalLights> modalLights = std::make_shared<MockModalLights>();
  EventManager eventManager = EventManagerFactory(modalLights, configs, deviceTime, testEvents);
  
  // event window under hardwareMinimumEventWindow gets rejected
  {
    for(uint32_t i = 0; i < hardwareMinimumEventWindow; i++){
      EventManagerConfigsStruct testConfigs = eventManager.getConfigs();
      testConfigs.defaultEventWindow_S = i;
      TEST_ASSERT_EQUAL(EventManagerErrors::bad_time, eventManager.setConfigs(testConfigs));
    }

    EventManagerConfigsStruct testConfigs = eventManager.getConfigs();
    testConfigs.defaultEventWindow_S = hardwareMinimumEventWindow;
    TEST_ASSERT_EQUAL(EventManagerErrors::success, eventManager.setConfigs(testConfigs));
  }
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