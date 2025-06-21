#include <unity.h>
#include <ModalLights.h>
#include "test_constBrightness.h"

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void testRoundingDivide(){
  for(uint16_t top = 0; top <= 255; top++){
    for(uint16_t bottom = 1; bottom <= 255; bottom++){
      uint8_t expected = round((float)(top)/ (float)(bottom));
      uint8_t actual = interpDivide(top, bottom);
      std::string message = "top = ";
      message += std::to_string(top);
      message += "; bottom = ";
      message += std::to_string(bottom);
      TEST_ASSERT_EQUAL_MESSAGE(expected, actual, message.c_str());
    }
  }
}

void SerializeAndDeserializeModeData(){
  // TODO: iterate over all channel types
  // test constant brightness
  {
    const uint8_t modeDataSize = sizeof(ModeDataStruct::ID) + sizeof(ModeDataStruct::type) + sizeof(ModeDataStruct::endColourRatios) + sizeof(ModeDataStruct::minBrightness);
    TEST_ASSERT_EQUAL(modeDataSize, getModeDataSize(ModeTypes::constantBrightness));
    
    ModeDataStruct tempMode = ModeDataStruct{
      .ID = 5,
      .type = ModeTypes::constantBrightness,
      .maxBrightness = 69,
      .minBrightness = 20
    };
    const duty_t testColours[8] = {255, 163, 247, 209, 69, 42, 0, 8};
    memcpy(tempMode.endColourRatios, testColours, nChannels);
    const ModeDataStruct testMode = tempMode;

    uint8_t expectedBuffer[modePacketSize];
    {
      expectedBuffer[0] = testMode.ID;
      expectedBuffer[1] = static_cast<uint8_t>(testMode.type);
      int i = 2;
      for(int c = 0; c<nChannels; c++){
        expectedBuffer[i] = testColours[c];
        i++;
      }
      expectedBuffer[i] = testMode.minBrightness;
      i++;
      for(i; i < modePacketSize; i++){
        expectedBuffer[i] = 0;
      }
    }

    // test the serialization
    uint8_t buffer[modePacketSize];
    for(int i = 0; i < modePacketSize; i++){
      buffer[i] = 0;
    }
    serializeModeDataStruct(testMode, buffer);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedBuffer, buffer, modePacketSize);

    // test the deserialization
    ModeDataStruct actualStruct;
    deserializeModeData(buffer, &actualStruct);
    TEST_ASSERT_EQUAL(testMode.ID, actualStruct.ID);
    TEST_ASSERT_EQUAL(testMode.type, actualStruct.type);
    TEST_ASSERT_EQUAL(testMode.minBrightness, actualStruct.minBrightness);
  }

  // TODO: test the other modes

  TEST_IGNORE_MESSAGE("need to do the other modes; constant brightness mode passed");
}

void testConfigGuards(){
  const TestChannels channel = TestChannels::RGB; // iterating over all channels is unnessesery here
  const std::string testName = "test bad configs";
  
  const uint8_t halfSoftChangeWindow_S = 1;
  const ModalConfigsStruct goodConfigs = {
    .minOnBrightness = 50,
    .softChangeWindow = halfSoftChangeWindow_S*2,
    .defaultOnBrightness = 0
  };

  // bad configs should use this struct for the good values, to make sure they don't get written
  const ModalConfigsStruct differentGoodConfigs = {
    .minOnBrightness = 20,
    .softChangeWindow = halfSoftChangeWindow_S*4,
    .defaultOnBrightness = 50
  };

  const uint64_t testStartTime = mondayAtMidnight;
  const TestObjectsStruct testObjects = modalLightsFactoryAllModes(channel, testStartTime, goodConfigs);

  const std::shared_ptr<ModalLightsController> testClass = testObjects.modalLights;

  const ModalConfigsStruct badConfigVals = {
    .minOnBrightness = 0,
    .softChangeWindow = 16
  };
  testClass->updateLights();
  TEST_ASSERT_EQUAL(goodConfigs.minOnBrightness, testClass->getBrightnessLevel());
  TEST_ASSERT_EQUAL(goodConfigs.minOnBrightness, testClass->getSetBrightness());
  TEST_ASSERT_EQUAL(true, testClass->getState());

  // test that minOnBrightness = 0 gets rejected
  {
    ModalConfigsStruct badConfigs{
      .minOnBrightness = 0,
      .softChangeWindow = differentGoodConfigs.softChangeWindow,
      .defaultOnBrightness = differentGoodConfigs.defaultOnBrightness
    };
    TEST_ASSERT_ERROR(errorCode_t::badValue, testObjects.setConfigs(badConfigs), testName.c_str());

    // testClass->setState(false);
    testClass->adjustBrightness(255, false);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    testClass->setState(true);
    TEST_ASSERT_EQUAL(goodConfigs.minOnBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(goodConfigs.minOnBrightness, testClass->getSetBrightness());
  }

  // test that soft change window over 15 gets rejected
  {
    ModalConfigsStruct badConfigs{
      .minOnBrightness = differentGoodConfigs.minOnBrightness,
      .softChangeWindow = 16,
      .defaultOnBrightness = differentGoodConfigs.defaultOnBrightness
    };
    TEST_ASSERT_ERROR(errorCode_t::badValue, testObjects.setConfigs(badConfigs), testName.c_str());

    // soft change should match goood configs
    testClass->setBrightnessLevel(255);
    TEST_ASSERT_EQUAL(goodConfigs.minOnBrightness, testClass->getBrightnessLevel());
    
    // half of good soft change
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    const duty_t expB = round((goodConfigs.minOnBrightness + 255)/2.);
    TEST_ASSERT_EQUAL(expB, testClass->getBrightnessLevel());

    // end of good soft change
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());

    // no changes near the end of the bad soft change
    incrementTimeAndUpdate_S(badConfigs.softChangeWindow - goodConfigs.softChangeWindow - 1, testObjects);
    TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());
  }

  // configs should still match good configs
  const ModalConfigsStruct actualConfigs = testClass->getConfigs();
  TEST_ASSERT_EQUAL(goodConfigs.minOnBrightness, actualConfigs.minOnBrightness);
  TEST_ASSERT_EQUAL(goodConfigs.softChangeWindow, actualConfigs.softChangeWindow);
  TEST_ASSERT_EQUAL(goodConfigs.defaultOnBrightness, actualConfigs.defaultOnBrightness);
}

void testSetModeIgnoring(){
  /*
  this is the behaviour expected by EventManager

  setting a mode that is current should be ignored, unless trigger time is important. this includes all background modes (except if changing is trigger-time dependant)

  setting a mode that is current, with the same trigger time, should always be ignored

  if the modeID doesn't exist, it gets discarded and isn't set to next
  */
 const uint8_t halfWindow_S = 1;
  const ModalConfigsStruct initialConfigs{
    .softChangeWindow = 2*halfWindow_S
  };
  TestObjectsStruct testObjects = modalLightsFactoryAllModes(
    TestChannels::RGB,
    mondayAtMidnight,
    initialConfigs
  );

  auto modalLights = testObjects.modalLights;
  modalLights->updateLights();
  auto mockStorageHAL = testObjects.mockStorageHAL;
  // test that getModeCount isn't incremented with repeat calls
  {
    uint64_t currentTime = incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_CURRENT_MODES(1, 0, modalLights);
    
    // repeat mode 1
    mockStorageHAL->getModeCount = 0;
    modalLights->setModeByUUID(1, currentTime, false);
    modalLights->updateLights();
    currentTime = incrementTimeAndUpdate_S(1, testObjects);

    TEST_ASSERT_EQUAL(0, mockStorageHAL->getModeCount);

    // set a different background mode
    modeUUID backgroundID = testModesMap["purpleConstBrightness"].ID;
    modalLights->setModeByUUID(backgroundID, currentTime, false);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(backgroundID, 0, modalLights);
    TEST_ASSERT_EQUAL(1, mockStorageHAL->getModeCount);

    // repeat the same background mode
    mockStorageHAL->getModeCount = 0;
    currentTime = incrementTimeAndUpdate_S(1, testObjects);
    modalLights->setModeByUUID(backgroundID, currentTime, false);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(backgroundID, 0, modalLights);
    TEST_ASSERT_EQUAL(0, mockStorageHAL->getModeCount);

    // set an active mode
    modeUUID activeMode1 = testModesMap["warmConstBrightness"].ID;
    currentTime = incrementTimeAndUpdate_S(1, testObjects);
    modalLights->setModeByUUID(activeMode1, currentTime, true);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(backgroundID, activeMode1, modalLights);
    TEST_ASSERT_EQUAL(1, mockStorageHAL->getModeCount);

    // repeat the active mode
    mockStorageHAL->getModeCount = 0;
    currentTime = incrementTimeAndUpdate_S(1, testObjects);
    modalLights->setModeByUUID(activeMode1, currentTime, true);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(backgroundID, activeMode1, modalLights);
    TEST_ASSERT_EQUAL(0, mockStorageHAL->getModeCount);

    // set the background mode to active
    currentTime = incrementTimeAndUpdate_S(1, testObjects);
    modalLights->setModeByUUID(backgroundID, currentTime, true);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(backgroundID, backgroundID, modalLights);
    TEST_ASSERT_EQUAL(0, mockStorageHAL->getModeCount);
  }

  const modeUUID badID = 105;
  for(const auto& pair : testModesMap){
    TEST_ASSERT_NOT_EQUAL_MESSAGE(badID, pair.second.ID, "need to choose a different badID because this one actually exists");
  }

  const duty_t minB = initialConfigs.minOnBrightness;

  // set a background mode that doesn't exist. nothing should happen
  {
    // prepare modal lights for a changeover
    uint64_t currentTime = incrementTimeAndUpdate_S(60, testObjects);
    modalLights->cancelActiveMode();
    modalLights->setModeByUUID(1, currentTime, false);
    modalLights->setBrightnessLevel(255);
    currentTime = incrementTimeAndUpdate_S(60, testObjects);

    TEST_ASSERT_CURRENT_MODES(1, 0, modalLights);
    TEST_ASSERT_EQUAL(255, modalLights->getBrightnessLevel());
    TEST_ASSERT_EQUAL(255, modalLights->getSetBrightness());
    TEST_ASSERT_EACH_EQUAL_UINT8(255, currentChannelValues, nChannels);

    // start the changeover
    const TestModeDataStruct goodMode = testModesMap["warmConstBrightness"];
    modalLights->setModeByUUID(goodMode.ID, currentTime, false);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(goodMode.ID, 0, modalLights);
    currentTime = incrementTimeAndUpdate_S(halfWindow_S, testObjects);

    // set bad ID
    modalLights->setModeByUUID(badID, currentTime, false);
    modalLights->updateLights();
    currentTime = incrementTimeAndUpdate_S(halfWindow_S, testObjects);

    // good mode should have completed its changeover
    TEST_ASSERT_CURRENT_MODES(goodMode.ID, 0, modalLights);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(goodMode.endColourRatios.RGB, currentChannelValues, nChannels);

    // start a new changeover
    modalLights->setModeByUUID(1, currentTime, false);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(1, 0, modalLights);
    currentTime = incrementTimeAndUpdate_S(halfWindow_S, testObjects);

    // set null ID
    modalLights->setModeByUUID(0, currentTime, false);
    modalLights->updateLights();
    currentTime = incrementTimeAndUpdate_S(halfWindow_S, testObjects);

    // changeover should have finished
    TEST_ASSERT_CURRENT_MODES(1, 0, modalLights);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(defaultConstantBrightness.endColourRatios.RGB, currentChannelValues, nChannels);
  }

  // set an active mode that doesn't exist. nothing should happen
  {
    // prepare modal lights for a changeover
    uint64_t currentTime = incrementTimeAndUpdate_S(60, testObjects);
    modalLights->cancelActiveMode();
    modalLights->setModeByUUID(1, currentTime, false);
    modalLights->setBrightnessLevel(255);
    currentTime = incrementTimeAndUpdate_S(60, testObjects);

    TEST_ASSERT_CURRENT_MODES(1, 0, modalLights);
    TEST_ASSERT_EQUAL(255, modalLights->getBrightnessLevel());
    TEST_ASSERT_EQUAL(255, modalLights->getSetBrightness());
    TEST_ASSERT_EACH_EQUAL_UINT8(255, currentChannelValues, nChannels);

    // start the changeover
    const TestModeDataStruct goodMode = testModesMap["warmConstBrightness"];
    modalLights->setModeByUUID(goodMode.ID, currentTime, false);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(goodMode.ID, 0, modalLights);
    currentTime = incrementTimeAndUpdate_S(halfWindow_S, testObjects);

    // set bad ID
    modalLights->setModeByUUID(badID, currentTime, true);
    modalLights->updateLights();
    currentTime = incrementTimeAndUpdate_S(halfWindow_S, testObjects);

    // good mode should have completed its changeover
    TEST_ASSERT_CURRENT_MODES(goodMode.ID, 0, modalLights);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(goodMode.endColourRatios.RGB, currentChannelValues, nChannels);

    // start a new changeover
    modalLights->setModeByUUID(1, currentTime, true);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(goodMode.ID, 1, modalLights);
    currentTime = incrementTimeAndUpdate_S(halfWindow_S, testObjects);

    // set null ID
    modalLights->setModeByUUID(0, currentTime, true);
    modalLights->updateLights();
    currentTime = incrementTimeAndUpdate_S(halfWindow_S, testObjects);

    // changeover should have finished
    TEST_ASSERT_CURRENT_MODES(goodMode.ID, 1, modalLights);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(defaultConstantBrightness.endColourRatios.RGB, currentChannelValues, nChannels);
  }
  
  // set a background mode that does exist, then a background mode that doesn't. the good mode should be loaded
  {
    // prepare modal lights for a changeover
    uint64_t currentTime = incrementTimeAndUpdate_S(60, testObjects);
    modalLights->cancelActiveMode();
    modalLights->setModeByUUID(1, currentTime, false);
    modalLights->setBrightnessLevel(255);
    currentTime = incrementTimeAndUpdate_S(60, testObjects);

    TEST_ASSERT_CURRENT_MODES(1, 0, modalLights);
    TEST_ASSERT_EQUAL(255, modalLights->getBrightnessLevel());
    TEST_ASSERT_EQUAL(255, modalLights->getSetBrightness());
    TEST_ASSERT_EACH_EQUAL_UINT8(255, currentChannelValues, nChannels);

    // null ID
    const TestModeDataStruct goodMode = testModesMap["warmConstBrightness"];
    modalLights->setModeByUUID(goodMode.ID, currentTime, false);
    modalLights->setModeByUUID(0, currentTime, false);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(goodMode.ID, 0, modalLights);
    currentTime = incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(goodMode.endColourRatios.RGB, currentChannelValues, nChannels);

    // bad ID
    modalLights->setModeByUUID(1, currentTime, false);
    modalLights->setModeByUUID(badID, currentTime, false);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(1, 0, modalLights);
    currentTime = incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(defaultConstantBrightness.endColourRatios.RGB, currentChannelValues, nChannels);
  }

  // set an active mode that does exist, then an active mode that doesn't. the good mode should be loaded
  {
    // prepare modal lights for a changeover
    uint64_t currentTime = incrementTimeAndUpdate_S(60, testObjects);
    modalLights->cancelActiveMode();
    modalLights->setModeByUUID(1, currentTime, false);
    modalLights->setBrightnessLevel(255);
    currentTime = incrementTimeAndUpdate_S(60, testObjects);

    TEST_ASSERT_CURRENT_MODES(1, 0, modalLights);
    TEST_ASSERT_EQUAL(255, modalLights->getBrightnessLevel());
    TEST_ASSERT_EQUAL(255, modalLights->getSetBrightness());
    TEST_ASSERT_EACH_EQUAL_UINT8(255, currentChannelValues, nChannels);

    // null ID
    const TestModeDataStruct goodMode = testModesMap["warmConstBrightness"];
    modalLights->setModeByUUID(goodMode.ID, currentTime, true);
    modalLights->setModeByUUID(0, currentTime, true);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(1, goodMode.ID, modalLights);
    currentTime = incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(goodMode.endColourRatios.RGB, currentChannelValues, nChannels);

    // bad ID
    modalLights->setModeByUUID(1, currentTime, true);
    modalLights->setModeByUUID(badID, currentTime, true);
    modalLights->updateLights();
    TEST_ASSERT_CURRENT_MODES(1, 1, modalLights);
    currentTime = incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(defaultConstantBrightness.endColourRatios.RGB, currentChannelValues, nChannels);
  }

  std::string ignoreMessage = "very important TODO, EventManager expects this behaviour; ";
  // TODO: set a background mode, then set the same mode as active. active behaviour should be observed, but the mode shouldn't be loaded from storage (need to test for every mode type)

  // TODO: test that corrupted modes aren't loaded

  std::string constantBrightnessIgnoreMessage = ignoreMessage + "need to test for constantBrightness";
  TEST_IGNORE_MESSAGE(constantBrightnessIgnoreMessage.c_str());
  std::string sunriseIgnoreMessage = ignoreMessage + "need to test for sunrise";
  TEST_IGNORE_MESSAGE(sunriseIgnoreMessage.c_str());
  std::string sunsetIgnoreMessage = ignoreMessage + "need to test for sunset";
  TEST_IGNORE_MESSAGE(sunsetIgnoreMessage.c_str());
  std::string pulseIgnoreMessage = ignoreMessage + "need to test for pulse";
  TEST_IGNORE_MESSAGE(pulseIgnoreMessage.c_str());
  std::string chirpIgnoreMessage = ignoreMessage + "need to test for chirp";
  TEST_IGNORE_MESSAGE(chirpIgnoreMessage.c_str());
  std::string changingIgnoreMessage = ignoreMessage + "need to test for changing";
  TEST_IGNORE_MESSAGE(changingIgnoreMessage.c_str());

  std::string corruptedIgnoreMessage = ignoreMessage + "need to test for corrupted modes";
  TEST_IGNORE_MESSAGE(corruptedIgnoreMessage.c_str());
  
  TEST_FAIL_MESSAGE("very important TODO, EventManager expects this behaviour");
}

void test_convertToDataPackets_helper(){
  // make sure the test helper function actually works properly!
  TEST_IGNORE_MESSAGE("TODO");
}

void testInitialisation(){
  /*
  at time of initialisation, lights should be off, but turn on next time update is called. in other words and from the user perspective, when the device boots the brightness should be minOnBrightness, set to whatever mode EventManager thinks should be current (or defaultConstantBrightness if none)

  To help reduce heap fragmentation, the stored ID maps should be built after all the classes are constructed. by deferring the loadMode() call until the first update (or setModeByID() call), it should ensure that event manager is constructed before the ModeIDs map is built

  TODO: when modes are functional, it probably won't matter
  the device shouldn't be booting that often, so colour changeovers should be pretty rare. if they aren't, there's bigger problems to deal with
  */

  // construction without any modes should default to constant brightness on update
  {
    // TODO: repeat this test with a single channel
    // mangle currentChannelValues because they should be zeroed on construction
    for(int i = 0; i < nChannels; i++){
      currentChannelValues[i] = 69;
    }
    ModalConfigsStruct testConfigs;
    TestObjectsStruct testObjects = modalLightsFactoryAllModes(
      TestChannels::RGB,
      mondayAtMidnight,
      testConfigs
    );
    auto modalLights = testObjects.modalLights;
    // constant brightness should the current mode
    const CurrentModeStruct initialModes = modalLights->getCurrentModes();
    TEST_ASSERT_EQUAL(0, initialModes.activeMode);
    TEST_ASSERT_EQUAL(0, initialModes.backgroundMode);

    // light values should be 0 before update is called
    TEST_ASSERT_EACH_EQUAL_UINT8(0, currentChannelValues, nChannels);

    // light values should be minOnBrightness after update is called
    modalLights->updateLights();
    TEST_ASSERT_EACH_EQUAL_UINT8(testConfigs.minOnBrightness, currentChannelValues, nChannels);
  }

  // when a mode is set after construction but before update is first called, light should start at minOnBrightness with no colour changeover (test with a constant brightness mode)
  {
    // TODO: repeat this test with a single channel
    // mangle currentChannelValues because they should be zeroed on construction
    for(int i = 0; i < nChannels; i++){
      currentChannelValues[i] = 69;
    }
    ModalConfigsStruct testConfigs{
      .minOnBrightness = 50
    };
    TestObjectsStruct testObjects = modalLightsFactoryAllModes(
      TestChannels::RGB,
      mondayAtMidnight,
      testConfigs
    );
    auto modalLights = testObjects.modalLights;
    // constant brightness should the current mode
    const CurrentModeStruct initialModes = modalLights->getCurrentModes();
    TEST_ASSERT_EQUAL(0, initialModes.activeMode);
    TEST_ASSERT_EQUAL(0, initialModes.backgroundMode);

    auto testMode = testModesMap.at("purpleConstBrightness");
    modalLights->setModeByUUID(testMode.ID, mondayAtMidnight, false);

    // light values should be 0 before update is called
    TEST_ASSERT_EACH_EQUAL_UINT8(0, currentChannelValues, nChannels);

    duty_t expChannelVals[nChannels];
    fillChannelBrightness(expChannelVals, testMode.endColourRatios.RGB, testConfigs.minOnBrightness);
    // light values should be minOnBrightness after update is called
    modalLights->updateLights();
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expChannelVals, currentChannelValues, nChannels);
  }

  // if state is set to off before update is called, lights still turn on (this will probably happen because the device is rebooting and the lights are off, and someones trying to turn it on)
  {
    // mangle currentChannelValues because they should be zeroed on construction
    for(int i = 0; i < nChannels; i++){
      currentChannelValues[i] = 69;
    }
    ModalConfigsStruct testConfigs;
    TestObjectsStruct testObjects = modalLightsFactoryAllModes(
      TestChannels::RGB,
      mondayAtMidnight,
      testConfigs
    );
    auto modalLights = testObjects.modalLights;
    // constant brightness should the current mode
    const CurrentModeStruct initialModes = modalLights->getCurrentModes();
    TEST_ASSERT_EQUAL(0, initialModes.activeMode);
    TEST_ASSERT_EQUAL(0, initialModes.backgroundMode);

    // light values should be 0 before update is called
    TEST_ASSERT_EACH_EQUAL_UINT8(0, currentChannelValues, nChannels);

    // light values should be minOnBrightness after update is called
    modalLights->setState(false);
    TEST_ASSERT_EACH_EQUAL_UINT8(testConfigs.minOnBrightness, currentChannelValues, nChannels);
  }

  // set state to true before update
  {
    // mangle currentChannelValues because they should be zeroed on construction
    for(int i = 0; i < nChannels; i++){
      currentChannelValues[i] = 69;
    }
    ModalConfigsStruct testConfigs;
    TestObjectsStruct testObjects = modalLightsFactoryAllModes(
      TestChannels::RGB,
      mondayAtMidnight,
      testConfigs
    );
    auto modalLights = testObjects.modalLights;
    // constant brightness should the current mode
    const CurrentModeStruct initialModes = modalLights->getCurrentModes();
    TEST_ASSERT_EQUAL(0, initialModes.activeMode);
    TEST_ASSERT_EQUAL(0, initialModes.backgroundMode);

    // light values should be 0 before update is called
    TEST_ASSERT_EACH_EQUAL_UINT8(0, currentChannelValues, nChannels);

    // light values should be minOnBrightness after update is called
    modalLights->setState(true);
    TEST_ASSERT_EACH_EQUAL_UINT8(testConfigs.minOnBrightness, currentChannelValues, nChannels);
  }

  // adjusting brightness up before first update call should work like normal
  {
    // mangle currentChannelValues because they should be zeroed on construction
    for(int i = 0; i < nChannels; i++){
      currentChannelValues[i] = 69;
    }
    ModalConfigsStruct testConfigs;
    TestObjectsStruct testObjects = modalLightsFactoryAllModes(
      TestChannels::RGB,
      mondayAtMidnight,
      testConfigs
    );
    auto modalLights = testObjects.modalLights;
    // constant brightness should the current mode
    const CurrentModeStruct initialModes = modalLights->getCurrentModes();
    TEST_ASSERT_EQUAL(0, initialModes.activeMode);
    TEST_ASSERT_EQUAL(0, initialModes.backgroundMode);

    // light values should be 0 before update is called
    TEST_ASSERT_EACH_EQUAL_UINT8(0, currentChannelValues, nChannels);

    const duty_t adj = 100;
    // light values should be minOnBrightness after update is called
    modalLights->adjustBrightness(adj, true);
    TEST_ASSERT_EACH_EQUAL_UINT8(adj + testConfigs.minOnBrightness, currentChannelValues, nChannels);
  }
  
  // TODO: when a mode is set after construction but before update is first called, light should start at minOnBrightness with no colour changeover (test with changing mode)
  TEST_IGNORE_MESSAGE("needs more tests");
}

void validateModePacketTest(){
  // test that the mode packet is valid

  // for max ratios, at least one channel should have a brightness of 255

  // for min ratios, at least one channel should have a brightness of 255 or all channels should be 0 to start with the previous mode's colours
  TEST_IGNORE_MESSAGE("not implemented");
}

void testDeleteMode(){
  // test behaviour when current mode is deleted

  // TODO: mode cannot be deleted if it is used in an event

  TEST_IGNORE_MESSAGE("TODO");
}

void testUpdateMode(){
  // test behaviour when current mode is updated
  TEST_IGNORE_MESSAGE("TODO");
}

void testModeSwitching(){
  // when an active and background mode are set at the same time, when update is called the active mode is loaded first
  // TODO: setting a mode that's already set shouldn't reset the mode
  // TODO: load an active mode, loading a background mode then cancelling the active without calling update should switch to the pending background mode
  // TODO: trying to load a non-existant mode should do nothing

  // TODO: (needs refactoring) when event manager blasts a load of modes but some of them are invalid, the invalid ones should be ignored within setModeByUUID(), i.e. if the last mode is invalid the penultimate mode is loaded instead
  // TODO: corrupt mode packets shouldn't be loaded, and should trigger a network error message
  TEST_IGNORE_MESSAGE("TODO");
}

#define TEST_interpolateValue(expectedVals, interpClass, currentTimestamp, stringMessage) {\
  std::string _brightnessMessage_T_iV = stringMessage + "; b";\
  TEST_ASSERT_EQUAL_MESSAGE(expectedVals[0], interpClass.brightness.interpolateValue(currentTimestamp, 0), _brightnessMessage_T_iV.c_str());\
  for(int c = 0; c < numberOfColours; c++){\
    std::string _colourMessage_T_iV = message + "; c = " + std::to_string(c);\
    TEST_ASSERT_EQUAL_MESSAGE(expectedVals[c+1], interpClass.colours.interpolateValue(currentTimestamp, c), _colourMessage_T_iV.c_str());\
  }\
}

void testInterpolationClass(){
  const uint8_t numberOfColours = 3;
  ModeInterpolationClass<numberOfColours> interp;


  // increasing window interpolation
  {
    const duty_t initialVals[numberOfColours+1] = {0, 0, 0, 0};
    const duty_t targetVals[numberOfColours+1] = {255, 255, 255, 255};
    const uint64_t dT_uS = 60*secondsToMicros / 10;
    const uint64_t window_uS = dT_uS * 10;
    const uint64_t timestamp_uS = mondayAtMidnight * secondsToMicros;
    interp.newInterp_window(timestamp_uS, window_uS, initialVals, targetVals);

    // up to window
    for(int k = 0; k < 10; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t expectedVals[numberOfColours+1];
      interpolateArrays(expectedVals, initialVals, targetVals, k/10., numberOfColours+1);
      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = timestamp_uS + (k*dT_uS);
      TEST_interpolateValue(expectedVals, interp, currentTimestamp, message);

      interp.findNextValues(actualVals, currentTimestamp);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedVals, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(0, interp.isDone(), message.c_str());
    }

    // at and after window
    for(int k = 1; k <= 10; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = timestamp_uS + (k*window_uS);
      interp.findNextValues(actualVals, currentTimestamp);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(targetVals, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(IsDoneBitFlags::both, interp.isDone(), message.c_str());
      TEST_interpolateValue(targetVals, interp, currentTimestamp, message);
    }
  }

  // decreasing window interpolation
  {
    const duty_t initialVals[numberOfColours+1] = {255, 255, 255, 255};
    const duty_t targetVals[numberOfColours+1] = {0, 0, 0, 0};
    const uint64_t dT_uS = 60*secondsToMicros / 10;
    const uint64_t window_uS = dT_uS * 10;
    const uint64_t timestamp_uS = mondayAtMidnight * secondsToMicros;
    interp.newInterp_window(timestamp_uS, window_uS, initialVals, targetVals);

    // up to window
    for(int k = 0; k < 10; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t expectedVals[numberOfColours+1];
      interpolateArrays(expectedVals, initialVals, targetVals, k/10., numberOfColours+1);

      const uint64_t currentTimestamp = timestamp_uS + (k*dT_uS);
      duty_t actualVals[numberOfColours+1];
      interp.findNextValues(actualVals, currentTimestamp);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedVals, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(0, interp.isDone(), message.c_str());
      TEST_interpolateValue(expectedVals, interp, currentTimestamp, message);
    }

    // at and after window
    for(int k = 1; k <= 10; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = timestamp_uS + (k*window_uS);
      interp.findNextValues(actualVals, currentTimestamp);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(targetVals, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(IsDoneBitFlags::both, interp.isDone(), message.c_str());
      TEST_interpolateValue(targetVals, interp, currentTimestamp, message);
    }
  }

  // different directions window interpolation
  {
    const duty_t initialVals1[numberOfColours+1] = {100, 255, 0, 255};
    const duty_t targetVals1[numberOfColours+1] = {255, 0, 255, 255};
    const uint64_t dT_uS = 60*secondsToMicros / 10;
    const uint64_t window_uS = dT_uS * 10;
    const uint64_t timestamp_uS = mondayAtMidnight * secondsToMicros;
    interp.newInterp_window(timestamp_uS, window_uS, initialVals1, targetVals1);

    // up to window
    for(int k = 0; k < 10; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t expectedVals[numberOfColours+1];
      interpolateArrays(expectedVals, initialVals1, targetVals1, k/10., numberOfColours+1);
      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = timestamp_uS + (k*dT_uS);
      interp.findNextValues(actualVals, currentTimestamp);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedVals, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(0, interp.isDone(), message.c_str());
      TEST_interpolateValue(expectedVals, interp, currentTimestamp, message);
    }

    // at and after window
    for(int k = 1; k <= 10; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = timestamp_uS + (k*window_uS);
      interp.findNextValues(actualVals, currentTimestamp);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(targetVals1, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(IsDoneBitFlags::both, interp.isDone(), message.c_str());
      TEST_interpolateValue(targetVals1, interp, currentTimestamp, message);
    }
  }

  // colour only interpolations
  {
    const duty_t initialVals[numberOfColours+1] = {99, 255, 4, 20};
    const duty_t targetVals[numberOfColours+1] = {99, 100, 255, 180};
    const uint64_t dT_uS = 60*secondsToMicros / 10;
    const uint64_t window_uS = dT_uS*10;
    const uint64_t timestamp_uS = mondayAtMidnight * secondsToMicros;
    interp.newInterp_window(timestamp_uS, window_uS, initialVals, targetVals);


    // up to window
    for(int k = 0; k < 10; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t expectedVals[numberOfColours+1];
      interpolateArrays(expectedVals, initialVals, targetVals, k/10., numberOfColours+1);

      const uint64_t currentTimestamp = timestamp_uS + (k*dT_uS);
      duty_t actualVals[numberOfColours+1];
      interp.findNextValues(actualVals, currentTimestamp);
      
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedVals, actualVals, numberOfColours+1, message.c_str());

      TEST_ASSERT_EQUAL_MESSAGE(IsDoneBitFlags::brightness, interp.isDone(), message.c_str());
      TEST_interpolateValue(expectedVals, interp, currentTimestamp, message);
    }

    // at and after window
    for(int k = 10; k < 20; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = timestamp_uS + (k*window_uS);
      interp.findNextValues(actualVals, currentTimestamp);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(targetVals, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(IsDoneBitFlags::both, interp.isDone(), message.c_str());
      TEST_interpolateValue(targetVals, interp, currentTimestamp, message);
    }
  }

  // changing window interpolation
  {
    const duty_t initialVals1[numberOfColours+1] = {255, 255, 255, 0};
    const duty_t targetVals1[numberOfColours+1] = {100, 255, 0, 255};
    const uint64_t dT_uS = 60*secondsToMicros / 10;
    const uint64_t window1_uS = dT_uS * 10;
    const uint64_t initialTime_uS = mondayAtMidnight * secondsToMicros;
    interp.newInterp_window(initialTime_uS, window1_uS, initialVals1, targetVals1);

    // up to and at half window
    for(int k = 0; k <= 5; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t expectedVals[numberOfColours+1];
      interpolateArrays(expectedVals, initialVals1, targetVals1, k/10., numberOfColours+1);
      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = initialTime_uS + (k*dT_uS);
      interp.findNextValues(actualVals, currentTimestamp);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedVals, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(0, interp.isDone(), message.c_str());
      TEST_interpolateValue(expectedVals, interp, currentTimestamp, message);
    }

    uint64_t testTimestamp1 = initialTime_uS + (5*dT_uS);
    duty_t currentVals[numberOfColours+1];
    interp.findNextValues(currentVals, testTimestamp1);
    
    const duty_t targetBrightness = 0;
    const duty_t adjBrightness = interpolate(initialVals1[0], targetVals1[0], 0.5);
    TEST_ASSERT_EQUAL(adjBrightness, currentVals[0]);
    const uint64_t window2_uS = 2 * window1_uS;
    interp.newBrightnessVal_window(testTimestamp1, window2_uS, adjBrightness, targetBrightness);
    
    // up to end of colour window
    for(int k = 6; k < 10; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t expectedVals[numberOfColours+1];
      interpolateArrays(expectedVals, initialVals1, targetVals1, k/10., numberOfColours+1);
      expectedVals[0] = interpolate(adjBrightness, targetBrightness, (k-5)/20.);

      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = initialTime_uS + (k*dT_uS);
      interp.findNextValues(actualVals, currentTimestamp);

      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedVals, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(0, interp.isDone(), message.c_str());
      TEST_interpolateValue(expectedVals, interp, currentTimestamp, message);
    }

    // after colour window
    for(int k = 10; k < 24; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t expectedVals[numberOfColours+1];
      memcpy(expectedVals, targetVals1, numberOfColours+1);
      expectedVals[0] = interpolate(adjBrightness, targetBrightness, (k-5)/20.);

      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = initialTime_uS + (k*dT_uS);
      interp.findNextValues(actualVals, currentTimestamp);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedVals, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(IsDoneBitFlags::colours, interp.isDone(), message.c_str());
      TEST_interpolateValue(expectedVals, interp, currentTimestamp, message);
    }

    duty_t targetVals2[numberOfColours+1] = {targetBrightness};
    memcpy(&targetVals2[1], &targetVals1[1], numberOfColours);
    // after brightness window
    for(int k = 25; k < 35; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = initialTime_uS + (k*dT_uS);
      interp.findNextValues(actualVals, currentTimestamp);

      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(targetVals2, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(IsDoneBitFlags::both, interp.isDone(), message.c_str());
      TEST_interpolateValue(targetVals2, interp, currentTimestamp, message);
    }
  }

  // TODO: test adjusting the window mid-interpolation

  // TODO: test rate interpolation

  TEST_IGNORE_MESSAGE("some tests pass, but there's more to do");
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
  RUN_TEST(testInterpolationClass);
  RUN_TEST(SerializeAndDeserializeModeData);
  RUN_TEST(testConfigGuards);
  RUN_TEST(testRoundingDivide);
  RUN_TEST(testInitialisation);
  RUN_TEST(validateModePacketTest);
  RUN_TEST(testDeleteMode);
  RUN_TEST(testUpdateMode);
  RUN_TEST(testModeSwitching);
  RUN_TEST(testSetModeIgnoring);
  
  ConstantBrightnessModeTests::constBrightness_tests();
  UNITY_END();
}


#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif