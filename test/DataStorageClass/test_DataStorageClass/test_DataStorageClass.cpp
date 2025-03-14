#include <unity.h>
#include <map>

#include "DataStorageClass.h"
#include "../../nativeMocksAndHelpers/mockStorageHAL.hpp"
#include "../../EventManager/test_EventManager/testEvents.h"

void setUp(void){}
void tearDown(void){}

std::shared_ptr<DataStorageClass> DataStorageFactory(
    std::vector<ModeDataPacket> modeDataPackets,
    std::vector<EventDataPacket> eventDataPackets
  ){
  auto mockStorageHAL = std::make_shared<MockStorageHAL>(modeDataPackets, eventDataPackets);
  auto mockDataClass = std::make_shared<DataStorageClass>(std::move(mockStorageHAL));
  return mockDataClass;
}

#define ASSERT_EQUAL_EVENT_STRUCTS(expectedEvent, actualEvent)\
{\
  TEST_ASSERT_EQUAL(expectedEvent.daysOfWeek, actualEvent.daysOfWeek);\
  TEST_ASSERT_EQUAL(expectedEvent.eventID, actualEvent.eventID);\
  TEST_ASSERT_EQUAL(expectedEvent.eventWindow, actualEvent.eventWindow);\
  TEST_ASSERT_EQUAL(expectedEvent.isActive, actualEvent.isActive);\
  TEST_ASSERT_EQUAL(expectedEvent.modeID, actualEvent.modeID);\
  TEST_ASSERT_EQUAL(expectedEvent.timeOfDay, actualEvent.timeOfDay);\
}

void testIterableEventCollection(void){
  // yes I know i'm testing the implementation, but its a templated class so the debugger ignores it
  // TODO: add Modes
  // DataPreloadChunkSize does not have to be 5 for the project, but since I'm testing 8 events
  // using 5 means 1 full chunk and 1 partial chunk
  TEST_ASSERT_EQUAL(5, DataPreloadChunkSize);
  
  std::vector<ModeDataPacket> storedModes = {};
  std::vector<EventDataPacket> storedEvents = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent6, testEvent7, testEvent8};
  auto mockStorageHAL = std::make_shared<MockStorageHAL>(storedModes, storedEvents);

  IterableCollection<nEvents_t, EventDataPacket> testCollection(mockStorageHAL, 8);

  TEST_ASSERT_EQUAL(8, testCollection.getNumberStored());

  // test first chunk loads correctly
  TEST_ASSERT_EQUAL(0, mockStorageHAL->eventChunkNumber);
  // mockStorageHAL->fillEventChunkCallCount = 0;
  for(int i = 0; i < 5; i ++){
    ASSERT_EQUAL_EVENT_STRUCTS(storedEvents.at(i), mockStorageHAL->eventBuffer[i]);
  }
  TEST_ASSERT_EQUAL(1, mockStorageHAL->fillEventChunkCallCount);

  // test arbitrary access of chunks
  for(int i = 0; i < 5; i ++){
    ASSERT_EQUAL_EVENT_STRUCTS(storedEvents.at(i), testCollection.getObjectAt(i));
  }
  TEST_ASSERT_EQUAL(1, mockStorageHAL->fillEventChunkCallCount);

  // test second chunk loads correctly
  testCollection.getObjectAt(5);
  TEST_ASSERT_EQUAL(1, mockStorageHAL->eventChunkNumber);
  for(int i = 5; i < 8; i ++){
    ASSERT_EQUAL_EVENT_STRUCTS(storedEvents.at(i), mockStorageHAL->eventBuffer[i - 5]);
  }
  TEST_ASSERT_EQUAL(2, mockStorageHAL->fillEventChunkCallCount);

  // test arbitrary access of chunks
  for(int i = 5; i < 7; i ++){
    ASSERT_EQUAL_EVENT_STRUCTS(storedEvents.at(i), testCollection.getObjectAt(i));
  }
  TEST_ASSERT_EQUAL(2, mockStorageHAL->fillEventChunkCallCount);

  // test out-of-bounds access
  EventDataPacket emptyEvent;
  ASSERT_EQUAL_EVENT_STRUCTS(emptyEvent, testCollection.getObjectAt(10));
}

void testIterableModeCollection(void){
  TEST_ASSERT(false);
}

void testEventGetters(void){
  std::vector<ModeDataPacket> storedModes = {};
  std::vector<EventDataPacket> storedEvents = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent6, testEvent7, testEvent8};

  std::shared_ptr<DataStorageClass> testClass = DataStorageFactory(storedModes, storedEvents);

  // test arbitrary access
  for(int i = 0; i < 8; i++){
    EventDataPacket expectedEvent = storedEvents.at(i);
    EventDataPacket actualEvent = testClass->getEvent(expectedEvent.eventID);
    ASSERT_EQUAL_EVENT_STRUCTS(expectedEvent, actualEvent);
  }

  // test the iterator
  EventStorageIterator testIterator = testClass->getAllEvents();
  TEST_ASSERT_EQUAL(true, testIterator.hasMore());

  int count = 0;
  while(testIterator.hasMore()){
    EventDataPacket expectedEvent = storedEvents.at(count);
    EventDataPacket actualEvent = testIterator.getNext();
    ASSERT_EQUAL_EVENT_STRUCTS(expectedEvent, actualEvent);
    count++;
  };

  // test behaviour when iteration is complete
  TEST_ASSERT_EQUAL(storedEvents.size(), count);
  TEST_ASSERT_EQUAL(false, testIterator.hasMore());

  EventDataPacket emptyEvent;
  ASSERT_EQUAL_EVENT_STRUCTS(emptyEvent, testIterator.getNext());
  // TODO: test iterator throws out-of-bounds error

  // test iterator can be reset
  testIterator.reset();
  TEST_ASSERT_EQUAL(true, testIterator.hasMore());

  EventDataPacket actualEvent = storedEvents.at(0);
  EventDataPacket expectedEvent = testIterator.getNext();
  ASSERT_EQUAL_EVENT_STRUCTS(expectedEvent, actualEvent);
}

void testModeGetters(void){
  TEST_IGNORE();
}

void testCRUDOperations(void){
  TEST_IGNORE();
};

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(testIterableEventCollection);
  RUN_TEST(testEventGetters);
  RUN_TEST(testModeGetters);
  RUN_TEST(testCRUDOperations);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif