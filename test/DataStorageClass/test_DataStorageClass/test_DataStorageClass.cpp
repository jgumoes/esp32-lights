#include <unity.h>

#include "DataStorageClass.h"
#include "../../nativeMocksAndHelpers/mockStorageHAL.hpp"
#include "../../EventManager/test_EventManager/testEvents.h"

void setUp(void){}
void tearDown(void){}

std::shared_ptr<DataStorageClass> DataStorageFactory(
    std::vector<ModeDataStruct> testModes,
    std::vector<EventDataPacket> eventDataPackets
){
  // auto mockStorageHAL = std::make_shared<MockStorageHAL>(modeDataPackets, eventDataPackets);
  // auto testModes = makeModeDataStructArray(testModeDataStructs, channel);
  auto mockStorageHAL = std::make_shared<MockStorageHAL>(testModes, eventDataPackets);
  auto dataStorageClass = std::make_shared<DataStorageClass>(std::move(mockStorageHAL));
  dataStorageClass->loadIDs();
  return dataStorageClass;
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
  
  std::vector<ModeDataStruct> storedModes = {};
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
  // TODO: i don't think i even want this functionality
  TEST_ASSERT(false);
}

void testEventGetters(void){
  std::vector<ModeDataStruct> storedModes = {};
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
  TestChannels channel = TestChannels::RGB; // TODO: iterate over all TestChannels
  // test that ID=1 always returns default constant brightness, even when no data is given
  {
    std::vector<ModeDataStruct> storedModes = {};
    std::vector<EventDataPacket> storedEvents = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent6, testEvent7, testEvent8};

    const ModeDataStruct defaultConst = convertTestModeStruct(defaultConstantBrightness, channel);
    
    std::shared_ptr<DataStorageClass> testClass = DataStorageFactory(storedModes, storedEvents);
    uint8_t buffer[modePacketSize];
    TEST_ASSERT_TRUE(testClass->getMode(1, buffer));
    TEST_ASSERT_EQUAL(defaultConst.ID, buffer[0]);
    TEST_ASSERT_EQUAL(ModeTypes::constantBrightness, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(defaultConst.endColourRatios, &buffer[2], nChannels);
    TEST_ASSERT_EQUAL(defaultConst.minBrightness, buffer[getModeDataSize(ModeTypes::constantBrightness) - 1]);
    
    TEST_ASSERT_FALSE(testClass->getMode(2, buffer));
  }

  // std::vector<TestModeDataStruct> testModeStructs = {warmConstBrightness, purpleConstBrightness};
  std::vector<TestModeDataStruct> testModeStructs = getAllTestingModes();
  auto testModes = makeModeDataStructArray(testModeStructs, channel);
  std::vector<EventDataPacket> storedEvents = {testEvent1, testEvent2, testEvent3, testEvent4, testEvent5, testEvent6, testEvent7, testEvent8};
  auto testClass = DataStorageFactory(testModes, storedEvents);
  
  // test getting arbitrary modes
  {
    uint8_t expectedBuffer[modePacketSize];
    uint8_t testBuffer[modePacketSize];

    // test default constant brightness first
    serializeModeDataStruct(convertTestModeStruct(defaultConstantBrightness, channel), expectedBuffer);
    TEST_ASSERT_TRUE(testClass->getMode(1, testBuffer));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
      expectedBuffer, testBuffer, getModeDataSize(ModeTypes::constantBrightness)
    );
    TEST_ASSERT_EQUAL(0, testBuffer[getModeDataSize(ModeTypes::constantBrightness) - 1]);

    // test all the other modes
    for(auto& mode : testModes){
      serializeModeDataStruct(mode, expectedBuffer);

      TEST_ASSERT_TRUE(testClass->getMode(mode.ID, testBuffer));
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedBuffer, testBuffer, getModeDataSize(mode.type));
      TEST_ASSERT_EQUAL(mode.minBrightness, testBuffer[getModeDataSize(ModeTypes::constantBrightness) - 1]);
    }
  }

  // test the mutating the returned data doesn't mutate the actual data
  // i.e. buffer should be copied into, not have its reference changed
  {
    // test default constant brightness first
    {
      uint8_t expectedBuffer[modePacketSize];
      serializeModeDataStruct(convertTestModeStruct(defaultConstantBrightness, channel), expectedBuffer);
      
      uint8_t changedBuffer[modePacketSize];
      TEST_ASSERT_TRUE(testClass->getMode(1, changedBuffer));
      for(uint8_t i = 2; i < modePacketSize; i++){changedBuffer[i] = i;}
      
      uint8_t testBuffer2[modePacketSize];
      TEST_ASSERT_TRUE(testClass->getMode(1, testBuffer2));
      TEST_ASSERT_EQUAL_UINT8_ARRAY(
        expectedBuffer, testBuffer2, getModeDataSize(ModeTypes::constantBrightness)
      );
    }

    // test all the other modes
    for(auto& mode : testModes){
      uint8_t expectedBuffer[modePacketSize];
      serializeModeDataStruct(mode, expectedBuffer);
      
      uint8_t changedBuffer[modePacketSize];
      TEST_ASSERT_TRUE(testClass->getMode(1, changedBuffer));
      for(uint8_t i = 2; i < modePacketSize; i++){changedBuffer[i] = i;}
      
      uint8_t testBuffer2[modePacketSize];
      TEST_ASSERT_TRUE(testClass->getMode(mode.ID, testBuffer2));
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedBuffer, testBuffer2, getModeDataSize(mode.type));
    }
  }
}

void testCRUDOperations(void){
  // TODO: create and update operations should immediately read from storage and verify CRC (this should detect corruptions)

  // TODO: update operations should be inplace, unless there's a bad section
  // note: is there a way to identify and keep track of corruptions?

  // TODO: delete operations should also zero data

  // TODO: create operations should start looking for space from the end of the stored values, but loop around if end-of-storage is reached

  // TODO: if space can't be found, create operations should report back to server (also when an update hits corruptions and can't find space)
  // note: the server and app should know if there is space available, and not make requests that would overflow storage
  // i.e. lack of space should only occur due to fragmentation or corrupt bytes
  TEST_IGNORE_MESSAGE("not yet implemented (post MVP)");
};

void testStorageValidation(void){
  // test the CRC checks for the storage

  // TODO: test the CRC for individual packets

  // TODO: test behaviour for CRC errors (i.e. packets don't match stored CRC)
  // should make a network request that deletes packet from storage

  // TODO: test the overall check
  // either another CRC that doesn't care about order performed on valid mode ids or CRCs, or a list of valid mode ids and a list of invalid mode ids
  TEST_IGNORE_MESSAGE("not yet implemented (post MVP)");
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
  RUN_TEST(testIterableEventCollection);
  RUN_TEST(testEventGetters);
  RUN_TEST(testModeGetters);
  RUN_TEST(testCRUDOperations);
  RUN_TEST(testStorageValidation);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif