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
}

void testConfigGuards(){
  const TestChannels channel = TestChannels::RGB; // iterating over all channels is unnessesery here
  
  const uint8_t halfSoftChangeWindow_S = 1;
  const ModalConfigsStruct goodConfigs = {
    .minOnBrightness = 50,
    .softChangeWindow = halfSoftChangeWindow_S*2
  };

  const uint64_t testStartTime = mondayAtMidnight;
  const TestObjectsStruct testObjects = modalLightsFactoryAllModes(channel, testStartTime, goodConfigs);

  const std::shared_ptr<ModalLightsController> testClass = testObjects.modalLights;

  const ModalConfigsStruct badConfigs = {
    .minOnBrightness = 0,
    .softChangeWindow = 16
  };

  TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
  TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
  TEST_ASSERT_EQUAL(false, testClass->getState());

  // test that minOnBrightness = 0 gets rejected
  {
    TEST_ASSERT_FALSE(testClass->changeMinOnBrightness(badConfigs.minOnBrightness));

    testClass->setState(true);
    TEST_ASSERT_EQUAL(goodConfigs.minOnBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(goodConfigs.minOnBrightness, testClass->getSetBrightness());
  }

  // test that soft change window over 15 gets rejected
  {
    TEST_ASSERT_FALSE(testClass->changeSoftChangeWindow(badConfigs.softChangeWindow));

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

  // TODO: check valid configs are written to ConfigManager
  TEST_FAIL_MESSAGE("need to test that configs are written to config manager");
}

void testRepeatBackgroundModesAreIgnored(){
  // if the set background is already the current background mode, it gets ignored (unless trigger time is important, and also different)
  // this is the behaviour expected by EventManager
  TEST_FAIL_MESSAGE("very important TODO, EventManager expects this behaviour");
}

void test_convertToDataPackets_helper(){
  // make sure the test helper function actually works properly!
  TEST_IGNORE_MESSAGE("TODO");
}

void testInitialisation(){
  // test initialisation
  // TODO: construction without any modes should default to constant brightness on update
  // TODO: passing a mode after construction but before update should change that mode
  TEST_IGNORE_MESSAGE("not implemented");
}

void validateModePacketTest(){
  // test that the mode packet is valid

  // for max ratios, at least one channel should have a brightness of 255

  // for min ratios, at least one channel should have a brightness of 255 or all channels should be 0 to start with the previous mode's colours
  TEST_IGNORE_MESSAGE("not implemented");
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

void testInterpolationClass(){
  const uint8_t numberOfColours = 3;
  InterpolationClass<numberOfColours> interp;


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
      interp.findNextValues(actualVals, currentTimestamp);
      for(int v = 0; v < numberOfColours+1; v++){
        std::string message2 = message + "; v = " + std::to_string(v);
        TEST_ASSERT_EQUAL_MESSAGE(expectedVals[v], interp.interpolateValue(currentTimestamp, v), message2.c_str());
      }
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
      for(int v = 0; v < numberOfColours+1; v++){
        std::string message2 = message + "; v = " + std::to_string(v);
        TEST_ASSERT_EQUAL_MESSAGE(targetVals[v], interp.interpolateValue(currentTimestamp, v), message2.c_str());
      }
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
      for(int v = 0; v < numberOfColours+1; v++){
        std::string message2 = message + "; v = " + std::to_string(v);
        TEST_ASSERT_EQUAL_MESSAGE(expectedVals[v], interp.interpolateValue(currentTimestamp, v), message2.c_str());
      }
    }

    // at and after window
    for(int k = 1; k <= 10; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = timestamp_uS + (k*window_uS);
      interp.findNextValues(actualVals, currentTimestamp);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(targetVals, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(IsDoneBitFlags::both, interp.isDone(), message.c_str());
      for(int v = 0; v < numberOfColours+1; v++){
        std::string message2 = message + "; v = " + std::to_string(v);
        TEST_ASSERT_EQUAL_MESSAGE(targetVals[v], interp.interpolateValue(currentTimestamp, v), message2.c_str());
      }
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
      for(int v = 0; v < numberOfColours+1; v++){
        std::string message2 = message + "; v = " + std::to_string(v);
        TEST_ASSERT_EQUAL_MESSAGE(expectedVals[v], interp.interpolateValue(currentTimestamp, v), message2.c_str());
      }
    }

    // at and after window
    for(int k = 1; k <= 10; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = timestamp_uS + (k*window_uS);
      interp.findNextValues(actualVals, currentTimestamp);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(targetVals1, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(IsDoneBitFlags::both, interp.isDone(), message.c_str());
      for(int v = 0; v < numberOfColours+1; v++){
        std::string message2 = message + "; v = " + std::to_string(v);
        TEST_ASSERT_EQUAL_MESSAGE(targetVals1[v], interp.interpolateValue(currentTimestamp, v), message2.c_str());
      }
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

      for(int v = 0; v < numberOfColours+1; v++){
        std::string message2 = message + "; v = " + std::to_string(v);
        TEST_ASSERT_EQUAL_MESSAGE(expectedVals[v], interp.interpolateValue(currentTimestamp, v), message2.c_str());
      }
      TEST_ASSERT_EQUAL_MESSAGE(IsDoneBitFlags::brightness, interp.isDone(), message.c_str());
    }

    // at and after window
    for(int k = 10; k < 20; k++){
      std::string message = "k = " + std::to_string(k);
      duty_t actualVals[numberOfColours+1];
      const uint64_t currentTimestamp = timestamp_uS + (k*window_uS);
      interp.findNextValues(actualVals, currentTimestamp);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(targetVals, actualVals, numberOfColours+1, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(IsDoneBitFlags::both, interp.isDone(), message.c_str());
      for(int v = 0; v < numberOfColours+1; v++){
        std::string message2 = message + "; v = " + std::to_string(v);
        TEST_ASSERT_EQUAL_MESSAGE(targetVals[v], interp.interpolateValue(currentTimestamp, v), message2.c_str());
      }
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
      for(int v = 0; v < numberOfColours+1; v++){
        std::string message2 = message + "; v = " + std::to_string(v);
        TEST_ASSERT_EQUAL_MESSAGE(expectedVals[v], interp.interpolateValue(currentTimestamp, v), message2.c_str());
      }
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
      for(int v = 0; v < numberOfColours+1; v++){
        std::string message2 = message + "; v = " + std::to_string(v);
        TEST_ASSERT_EQUAL_MESSAGE(expectedVals[v], interp.interpolateValue(currentTimestamp, v), message2.c_str());
      }
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
      for(int v = 0; v < numberOfColours+1; v++){
        std::string message2 = message + "; v = " + std::to_string(v);
        TEST_ASSERT_EQUAL_MESSAGE(expectedVals[v], interp.interpolateValue(currentTimestamp, v), message2.c_str());
      }
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
      for(int v = 0; v < numberOfColours+1; v++){
        std::string message2 = message + "; v = " + std::to_string(v);
        TEST_ASSERT_EQUAL_MESSAGE(targetVals2[v], interp.interpolateValue(currentTimestamp, v), message2.c_str());
      }
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
  RUN_TEST(testModeSwitching);
  RUN_TEST(testRepeatBackgroundModesAreIgnored);
  
  ConstantBrightnessModeTests::constBrightness_tests();
  UNITY_END();
}


#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif