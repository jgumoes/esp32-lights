#include <unity.h>
#include <ModalLights.h>

#include "PrintDebug.h"

#include "../../nativeMocksAndHelpers/mockConfig.h"
#include "../../nativeMocksAndHelpers/mockStorageHAL.hpp"
#include "../../EventManager/test_EventManager/testEvents.h"

const ModalConfigsStruct defaultConfigs;
static uint8_t currentChannelValues[nChannels]; // maybe turn this into a macro that gets the static variable from TestLEDClass?

class TestLEDClass : public VirtualLightsClass
{
public:

  // static uint8_t currentChannelValues[nChannels];

  TestLEDClass(){
    resetChannelValues();
  }

  void resetChannelValues(){
    for(uint8_t i = 0; i < nChannels; i++){
      currentChannelValues[i] = 0;
    }
  }

  void setChannelValues(duty_t newValues[nChannels]) override {
    memcpy(currentChannelValues, newValues, nChannels);
  };
};

struct TestObjectsStruct {
  std::unique_ptr<OnboardTimestamp> timestamp = std::make_unique<OnboardTimestamp>();
  std::shared_ptr<DeviceTimeInterface> deviceTime;
  std::shared_ptr<ConfigManagerClass> configManager;

  const std::vector<ModeDataStruct> initialModes;
  std::shared_ptr<ModalLightsController> modalLights;
};

TestObjectsStruct modalLightsFactory(TestChannels channel, const std::vector<TestModeDataStruct> modes, uint64_t startTime_S){
  TestObjectsStruct testObjects = {.initialModes = makeModeDataStructArray(modes, channel)};
  auto configHal = std::make_unique<MockConfigHal>();
  // auto configHal = makeConcreteConfigHal<MockConfigHal>();
  testObjects.configManager = std::make_shared<ConfigManagerClass>(std::move(configHal));
  
  testObjects.deviceTime = std::make_shared<DeviceTimeNoRTCClass>(testObjects.configManager);
  testObjects.deviceTime->setLocalTimestamp2000(startTime_S, 0, 0);

  // std::vector<EventDataPacket> events = getAllTestEvents();
  auto storageHAL = std::make_shared<MockStorageHAL>(testObjects.initialModes, getAllTestEvents());
  auto storage = std::make_shared<DataStorageClass>(storageHAL);
  
  // auto realLightsClass = std::make_unique<TestLEDClass>();
  // auto lightClass = std::static_pointer_cast<VirtualLightsClass>(std::move(realLightsClass));
  // std::unique_ptr<VirtualLightsClass> lightClass = std::static_pointer_cast<VirtualLightsClass>(std::make_unique<TestChannels>());
  auto lightsClass = concreteLightsClassFactory<TestLEDClass>();
  testObjects.modalLights = std::make_shared<ModalLightsController>(
    concreteLightsClassFactory<TestLEDClass>(),
    testObjects.deviceTime,
    storage,
    testObjects.configManager
  );
  return testObjects;
}

TestObjectsStruct modalLightsFactoryAllModes(TestChannels channel, uint64_t startTime_S){
  const std::vector<TestModeDataStruct> modes = getAllTestingModes();
  return modalLightsFactory(channel, modes, startTime_S);
}

duty_t channelBrightness(duty_t colourRatio, duty_t brightness){
  return round((colourRatio * brightness)/255.);
};

void fillChannelBrightness(duty_t expectedArr[nChannels], const duty_t colourRatios[nChannels], const duty_t brightness){
  for(int c = 0; c < nChannels; c++){
    expectedArr[c] = static_cast<duty_t>(round(colourRatios[c]*brightness/255.));
  }
}

uint64_t incrementTimeAndUpdate_uS(uint64_t increment_uS, const TestObjectsStruct &testObjects){
  const uint64_t newTimestamp = testObjects.timestamp->getTimestamp_uS() + increment_uS;
  testObjects.timestamp->setTimestamp_uS(newTimestamp);
  testObjects.modalLights->updateLights();
  return newTimestamp;
};

/**
 * @brief increments the current timestamp. rounds the existing timestamp to the closest second before incrementing. incrementing by 1 second guarantees the timestamp to be exactly on the second
 * 
 * @param increment_S 
 * @param testObjects 
 * @return the new timestamp in seconds
 */
uint64_t incrementTimeAndUpdate_S(uint64_t increment_S, const TestObjectsStruct &testObjects){
  const uint64_t currentTimestamp = (testObjects.timestamp->getTimestamp_uS() + (secondsToMicros/2))/secondsToMicros;
  const uint64_t newTimestamp = currentTimestamp + increment_S;
  
  testObjects.timestamp->setTimestamp_S(newTimestamp);
  testObjects.modalLights->updateLights();
  return newTimestamp;
};

#define TEST_ASSERT_EQUAL_COLOURS(expectedColours, expectedBrightness, actualColours, numberOfChannels) \
for(int i = 0; i < numberOfChannels; i++){\
  std::string current_i = "failed on i = " + std::to_string(i);\
  TEST_ASSERT_EQUAL_MESSAGE(\
    channelBrightness(expectedColours[i], expectedBrightness),\
    actualColours[i], current_i.c_str());\
}

// fuck it. 1 bit = +/- 0.4% error, it's fine
#define TEST_ASSERT_COLOURS_WITHIN_1(expectedColours, expectedBrightness, actualColours, numberOfChannels) \
for(int i = 0; i < numberOfChannels; i++){\
  std::string current_i = "failed on i = " + std::to_string(i);\
  TEST_ASSERT_UINT8_WITHIN_MESSAGE(\
    1,\
    channelBrightness(expectedColours[i], expectedBrightness),\
    actualColours[i], current_i.c_str());\
}

/**
 * @brief interpolate between two uint8_t values
 * 
 * @param initialVal 
 * @param finalVal 
 * @param ratio dT/window
 * @return duty_t 
 */
duty_t interpolate(duty_t initialVal, duty_t finalVal, float ratio){
  if(ratio == 0){return initialVal;}
  if(ratio >= 1){return finalVal;}
  return static_cast<duty_t>(round(initialVal + (finalVal-initialVal)*ratio));
}

/**
 * @brief interpolate colour ratios between initial and final values. brightness is assumed to be constant
 * 
 * @param expectedArr destination colour channel array
 * @param c0 initial colour channel array
 * @param c1 final colour channel array
 * @param ratio dT/window
 */
void interpolateColourRatios(duty_t expectedArr[nChannels], const duty_t c0[nChannels], const duty_t c1[nChannels], float ratio){
  if(ratio == 0){
    memcpy(expectedArr, c0, nChannels);
  }
  if(ratio >= 1){
    memcpy(expectedArr, c1, nChannels);
  }
  for(int c = 0; c < nChannels; c++){
    int db = c1[c] - c0[c];
    expectedArr[c] = static_cast<duty_t>(round(c0[c] + db*ratio));
  }
}






namespace ConstantBrightnessModeTests
{
void testUpdateLights(){
  // Test the update behaviour (there shouldn't be any change)
  const TestChannels channel = TestChannels::RGB; // TODO: iterate over all channels
  
  const uint64_t testStartTime = mondayAtMidnight;
  const TestObjectsStruct testObjects = modalLightsFactoryAllModes(channel, testStartTime);

  // no quick changes for this test
  const ModalConfigsStruct testConfigs = {
    .defaultOnBrightness = defaultConfigs.defaultOnBrightness,
    .minOnBrightness = 1,
    .changeoverWindow = 0,
    .softChangeWindow = 0
  };
  testObjects.configManager->setModalConfigs(testConfigs);
  const std::shared_ptr<ModalLightsController> testClass = testObjects.modalLights;
  
  // test that updating doesn't change anything
  testClass->updateLights();
  volatile CurrentModeStruct currentModes1 = testClass->getCurrentModes();
  TEST_ASSERT_EQUAL(0, currentModes1.activeMode);
  TEST_ASSERT_EQUAL(1, currentModes1.backgroundMode);

  const duty_t testBrightness = 255;
  TEST_ASSERT_EQUAL(testBrightness, testClass->setBrightnessLevel(testBrightness));

  // start with default constant brightness as background
  {
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(255, currentChannelValues[i]);
    }
    uint64_t testTime = mondayAtMidnight + 5*60;
    testObjects.timestamp->setTimestamp_S(testTime);
    testClass->updateLights();
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(255, currentChannelValues[i]);
    }
  }

  // change to warm background
  {
    const uint64_t testTime = mondayAtMidnight + timeToSeconds(20, 0, 0);
    testObjects.timestamp->setTimestamp_S(testTime);
    const duty_t testBrightness = 125;
    testClass->setBrightnessLevel(testBrightness);
    TestModeDataStruct testMode = mvpModes["warmConstBrightness"];
    testClass->setModeByUUID(testMode.ID, testTime, false);
    testClass->updateLights();

    const uint8_t* expectedColourRatios = testMode.endColourRatios.RGB;  // TODO: make generic accessor

    TEST_ASSERT_EQUAL_COLOURS(expectedColourRatios, testBrightness, currentChannelValues, nChannels);

    testObjects.timestamp->setTimestamp_S(testTime + 60*60);
    testClass->updateLights();
    TEST_ASSERT_EQUAL_COLOURS(expectedColourRatios, testBrightness, currentChannelValues, nChannels);
  }
}

void testBrightnessAdjustment(){
  // test behaviour when the brightness is adjusted (change should be instant)
  const TestChannels channel = TestChannels::RGB; // TODO: iterate over all channels

  const uint64_t testStartTime = mondayAtMidnight;
  uint64_t currentTestTime = testStartTime;

  const TestObjectsStruct testObjects = modalLightsFactoryAllModes(channel, testStartTime);

  // softChange shouldn't occur with adjustments
  const uint8_t halfSoftChangeWindow_S = 1;
  // const uint64_t halfWindow_uS = halfSoftChangeWindow_S * secondsToMicros;
  const ModalConfigsStruct testConfigs = {
    .defaultOnBrightness = defaultConfigs.defaultOnBrightness,
    .minOnBrightness = 1,
    .changeoverWindow = 0,
    .softChangeWindow = halfSoftChangeWindow_S * 2
  };
  testObjects.configManager->setModalConfigs(testConfigs);

  const std::shared_ptr<ModalLightsController> testClass = testObjects.modalLights;

  // adjust up and down within bounds
  {
    // 0 to 255 (up from off)
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(255, testClass->adjustBrightness(255, true));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());
    
    // 255 to 100 (down)
    TEST_ASSERT_EQUAL(100, testClass->adjustBrightness(155, false));
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(100, testClass->getBrightnessLevel());

    // 100 to 155 (up)
    TEST_ASSERT_EQUAL(155, testClass->adjustBrightness(55, true));
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(155, testClass->getBrightnessLevel());

    // 155 to 0 (down to off)
    TEST_ASSERT_EQUAL(true, testClass->getState());
    TEST_ASSERT_EQUAL(0, testClass->adjustBrightness(155, false));
    TEST_ASSERT_EQUAL(false, testClass->getState());
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
  }

  // adjust up and down out of bounds
  {
    // adjust down from 0
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(0, testClass->adjustBrightness(55, false));
    TEST_ASSERT_EQUAL(false, testClass->getState());
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());

    // adjust up to 200
    const duty_t minB = testConfigs.minOnBrightness;
    TEST_ASSERT_EQUAL(200, testClass->adjustBrightness(200, true));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(200, testClass->getBrightnessLevel());

    // adjust up beyond 255
    TEST_ASSERT_EQUAL(255, testClass->adjustBrightness(200, true));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());

    // adjust down to 55
    TEST_ASSERT_EQUAL(55, testClass->adjustBrightness(200, false));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(55, testClass->getBrightnessLevel());

    // adjust down below 0
    TEST_ASSERT_EQUAL(0, testClass->adjustBrightness(200, false));
    TEST_ASSERT_EQUAL(false, testClass->getState());
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
  }

  // set state to off and adjusting brightness up should start from 0
  {
    // adjust up to 200 first
    TEST_ASSERT_EQUAL(0, testClass->adjustBrightness(255, false));
    TEST_ASSERT_EQUAL(200, testClass->adjustBrightness(200, true));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    currentTestTime += halfSoftChangeWindow_S * 4;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(200, testClass->getBrightnessLevel());

    // turn off
    TEST_ASSERT_EQUAL(false, testClass->setState(false));
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(200, testClass->getBrightnessLevel());  // because we haven't touched the brightness level
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
    }

    // adjust the brightness to 1
    TEST_ASSERT_EQUAL(1, testClass->adjustBrightness(1, true));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    for(int i = 1; i < nChannels; i++){
      TEST_ASSERT_EQUAL(1, currentChannelValues[i]);
    }
  }

  // set state to off and adjusting brightness down should not affect brightness level when switched back on
  {
    // adjust up to 200 first
    TEST_ASSERT_EQUAL(0, testClass->adjustBrightness(255, false));
    TEST_ASSERT_EQUAL(200, testClass->adjustBrightness(200, true));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    currentTestTime += halfSoftChangeWindow_S * 4;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(200, testClass->getBrightnessLevel());

    // turn off
    TEST_ASSERT_EQUAL(false, testClass->setState(false));
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(200, testClass->getBrightnessLevel());  // because we haven't touched the brightness level
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
    }

    // adjust brightness down
    TEST_ASSERT_EQUAL(200, testClass->adjustBrightness(100, false));
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(200, testClass->getBrightnessLevel());

    // turn back on
    TEST_ASSERT_EQUAL(true, testClass->setState(true));
    currentTestTime += halfSoftChangeWindow_S * 4;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(200, testClass->getBrightnessLevel());
    for(int i = 1; i < nChannels; i++){
      TEST_ASSERT_EQUAL(200, currentChannelValues[i]);
    }
  }

  // set brightness to high, then adjust to under minOnBrightness but above 0. brightness should be 0
  const ModalConfigsStruct testConfigs2 = {
    .defaultOnBrightness = 10,
    .minOnBrightness = 5,
    .changeoverWindow = 0,
    .softChangeWindow = halfSoftChangeWindow_S * 2
  };
  testObjects.configManager->setModalConfigs(testConfigs2);
  {
    testClass->setBrightnessLevel(200);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(200, testClass->getBrightnessLevel());

    const duty_t minOnBrightness = testConfigs2.minOnBrightness;
    
    // adjust below min 
    testClass->adjustBrightness(199, false);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
  }
  
  // adjust brightness to minOnBrightness. adjusting down by 1 should turn the lights off
  {
    // current brightness should be minOnBrightness
    const duty_t minOnBrightness = testConfigs2.minOnBrightness;
    testClass->setBrightnessLevel(minOnBrightness);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(minOnBrightness, testClass->getBrightnessLevel());

    // adjust down by 1
    testClass->adjustBrightness(1, false);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(false, testClass->getState());

    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(false, testClass->getState());
  }

  // adjust up from 0 should start with minOnBrightness
  {
    const duty_t defaultOnBrightness = testConfigs2.defaultOnBrightness;
    const duty_t minOnBrightness = testConfigs2.minOnBrightness;
    for(int n = 0; n <= defaultOnBrightness; n++){
      // set brightness to 0
      testClass->adjustBrightness(255, false);
      incrementTimeAndUpdate_S(1, testObjects);

      // adjust up by n + 1
      testClass->adjustBrightness(n+1, true);
      TEST_ASSERT_EQUAL(minOnBrightness+n, testClass->getBrightnessLevel());
      TEST_ASSERT_EQUAL(true, testClass->getState());

      // incrementTimeAndUpdate_S(1, testObjects);
    }

  }

  // adjusting brightness during a softchange should immediately change and stay at the new brightness
  {
    // setup
    const duty_t b0 = 100;
    const duty_t b1 = 200;
    testClass->setBrightnessLevel(b0);
    incrementTimeAndUpdate_S(60, testObjects);

    // set to different brightness
    testClass->setBrightnessLevel(b1);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    const duty_t expB = round((b1+b0)/2.);
    TEST_ASSERT_EQUAL(expB, testClass->getBrightnessLevel());

    // adjustment
    const duty_t adj = 1;
    testClass->adjustBrightness(adj, true);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(expB+adj, testClass->getBrightnessLevel());
  }

  // adjust by 0 during softChange should do nothing
  {
    // setup
    const duty_t b0 = 100;
    const duty_t b1 = 200;
    testClass->setBrightnessLevel(b0);
    incrementTimeAndUpdate_S(60, testObjects);

    // set to different brightness
    testClass->setBrightnessLevel(b1);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    const duty_t expB = round((b1+b0)/2.);
    TEST_ASSERT_EQUAL(expB, testClass->getBrightnessLevel());

    // adjustment
    testClass->adjustBrightness(0, true);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(b1, testClass->getBrightnessLevel());
  }

  // set brightness high then set to 0. adjust up by 1 when brightness is under min but above 0. brightness should be minOnBrightness
  {
    // setup
    const duty_t b0 = 255;
    testClass->setBrightnessLevel(b0);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(b0, testClass->getBrightnessLevel());

    PrintDebug_reset;
    const duty_t expB = 3;
    TEST_ASSERT_TRUE(expB < testConfigs2.minOnBrightness);  // just making sure
    testClass->setBrightnessLevel(0);
    const uint64_t dT_uS = round(((b0 - expB)*secondsToMicros*halfSoftChangeWindow_S*2.)/255);
    PrintDebug_variable("dT_uS", dT_uS);
    incrementTimeAndUpdate_uS(dT_uS, testObjects);
    TEST_ASSERT_EQUAL(expB, testClass->getBrightnessLevel());

    // adjust up
    testClass->adjustBrightness(1, true);
    TEST_ASSERT_EQUAL(testConfigs2.minOnBrightness, testClass->getBrightnessLevel());
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(testConfigs2.minOnBrightness, testClass->getBrightnessLevel());
  }

}

void testSetBrightness(){
  // test behaviour when the brightness is set to a value (should be soft change)
  const TestChannels channel = TestChannels::RGB; // TODO: iterate over all channels

  const uint64_t testStartTime = mondayAtMidnight;
  uint64_t currentTestTime = testStartTime;

  const TestObjectsStruct testObjects = modalLightsFactoryAllModes(channel, testStartTime);

  const uint8_t halfSoftChangeWindow_S = 1;
  const ModalConfigsStruct testConfigs = {
    .defaultOnBrightness = 10,
    .minOnBrightness = 1,
    .changeoverWindow = 0,
    .softChangeWindow = halfSoftChangeWindow_S * 2
  };
  testObjects.configManager->setModalConfigs(testConfigs);

  const std::shared_ptr<ModalLightsController> testClass = testObjects.modalLights;
  testObjects.timestamp->setTimestamp_S(currentTestTime);
  testClass->updateLights();
  
  // set to 255 from 0
  {
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(255, testClass->setBrightnessLevel(255));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    TEST_ASSERT_EQUAL(testConfigs.minOnBrightness, testClass->getBrightnessLevel());

    // test at half soft change
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness1 = round(255/2.);
    TEST_ASSERT_EQUAL(expectedBrightness1, testClass->getBrightnessLevel());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(expectedBrightness1, currentChannelValues[i]);
    }

    // after soft change finishes
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness2 = 255;
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getBrightnessLevel());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(expectedBrightness2, currentChannelValues[i]);
    }

    // stays at end brightness
    currentTestTime += halfSoftChangeWindow_S * 10;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getBrightnessLevel());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(expectedBrightness2, currentChannelValues[i]);
    }
  }

  // set to 100 from 255
  {
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();

    const duty_t oldBrightness = 255;
    const duty_t newBrightness = 100;
    TEST_ASSERT_EQUAL(oldBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(true, testClass->getState());
    TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
    TEST_ASSERT_EQUAL(oldBrightness, testClass->getBrightnessLevel());

    // test at half soft change
    PrintDebug_reset;
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness1 = round((oldBrightness + newBrightness)/2.);
    TEST_ASSERT_UINT8_WITHIN(1, expectedBrightness1, testClass->getBrightnessLevel());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(expectedBrightness1, currentChannelValues[i]);
    }

    // after soft change finishes
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness2 = newBrightness;
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getBrightnessLevel());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(expectedBrightness2, currentChannelValues[i]);
    }

    // stays at end brightness
    currentTestTime += halfSoftChangeWindow_S * 10;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getBrightnessLevel());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(expectedBrightness2, currentChannelValues[i]);
    }
  }

  // change mode to warm, then set to 140 from 100
  const TestModeDataStruct modeData = mvpModes["warmConstBrightness"];
  PrintDebug_UINT8_array("expected colour ratios", modeData.endColourRatios.RGB, nChannels);

  {
    const duty_t oldBrightness = 100;
    const duty_t newBrightness = 140;

    // change the mode
    testClass->setModeByUUID(modeData.ID, currentTestTime, false);
    testClass->updateLights();

    TEST_ASSERT_EQUAL(oldBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(true, testClass->getState());
    TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
    TEST_ASSERT_EQUAL(oldBrightness, testClass->getBrightnessLevel());

    // test at half soft change
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness1 = round((oldBrightness + newBrightness)/2.);

    
    PrintDebug_UINT8_array("expected colour ratios", modeData.endColourRatios.RGB, nChannels);
    TEST_ASSERT_EQUAL(expectedBrightness1, testClass->getBrightnessLevel());
    TEST_ASSERT_COLOURS_WITHIN_1(modeData.endColourRatios.RGB, expectedBrightness1, currentChannelValues, nChannels);

    // after soft change finishes
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness2 = newBrightness;
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL_COLOURS(modeData.endColourRatios.RGB, expectedBrightness2, currentChannelValues, nChannels);

    // stays at end brightness
    currentTestTime += halfSoftChangeWindow_S * 10;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL_COLOURS(modeData.endColourRatios.RGB, expectedBrightness2, currentChannelValues, nChannels);
  }

  // set brightness to 0
  {
    const duty_t oldBrightness = 140;
    const duty_t newBrightness = 0;

    // change the mode
    // testClass->setModeByUUID(modeData.ID, currentTestTime, false);
    testClass->updateLights();

    TEST_ASSERT_EQUAL(oldBrightness, testClass->getBrightnessLevel());
    // TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
    TEST_ASSERT_EQUAL(oldBrightness, testClass->getBrightnessLevel());

    // test at half soft change
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness1 = round((oldBrightness + newBrightness)/2);
    TEST_ASSERT_EQUAL(expectedBrightness1, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL_COLOURS(modeData.endColourRatios.RGB, expectedBrightness1, currentChannelValues, nChannels);

    // after soft change finishes
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness2 = newBrightness;
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL_COLOURS(modeData.endColourRatios.RGB, expectedBrightness2, currentChannelValues, nChannels);
    TEST_ASSERT_EQUAL(false, testClass->getState());

    // stays at end brightness
    currentTestTime += halfSoftChangeWindow_S * 10;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL_COLOURS(modeData.endColourRatios.RGB, expectedBrightness2, currentChannelValues, nChannels);
  }

  // switching lights on from 0 should soft change to default
  {
    
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_NOT_EQUAL(0, testConfigs.defaultOnBrightness);

    TEST_ASSERT_EQUAL(true, testClass->setState(true));

    // test at half soft change
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness1 = round((testConfigs.defaultOnBrightness+testConfigs.minOnBrightness)/2.);
    TEST_ASSERT_EQUAL(expectedBrightness1, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL_COLOURS(modeData.endColourRatios.RGB, expectedBrightness1, currentChannelValues, nChannels);

    // after soft change finishes
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness2 = testConfigs.defaultOnBrightness;
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL_COLOURS(modeData.endColourRatios.RGB, expectedBrightness2, currentChannelValues, nChannels);

    // stays at end brightness
    currentTestTime += halfSoftChangeWindow_S * 10;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL_COLOURS(modeData.endColourRatios.RGB, expectedBrightness2, currentChannelValues, nChannels);
  }

  // when the brightness is set to a low value (i.e. 1) from 0, the lights should immediately turn on even when the interpolation still sets it to 0
  const ModalConfigsStruct testConfigs2 = {
    .defaultOnBrightness = 5,
    .minOnBrightness = 1,
    .changeoverWindow = 0,
    .softChangeWindow = 15
  };
  testObjects.configManager->setModalConfigs(testConfigs2);
  {
    const duty_t minBrightness = testConfigs2.minOnBrightness;
    const duty_t defaultBrightness = testConfigs2.defaultOnBrightness;
    const duty_t window_S = testConfigs2.softChangeWindow;

    // set brightness to 0
    TEST_ASSERT_EQUAL(0, testClass->setBrightnessLevel(0));
    currentTestTime += halfSoftChangeWindow_S*30;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(false, testClass->getState());

    // switch lights on, brightness should be 1
    currentTestTime += halfSoftChangeWindow_S*30;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->setState(true);
    TEST_ASSERT_EQUAL(1, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    // at least one of the colours should be non-zero
    bool colourOverZero = false;
    for(int i = 0; i<nChannels; i++){
      colourOverZero |= currentChannelValues[i] > 0;
    }
    TEST_ASSERT_EQUAL(true, colourOverZero);

    uint64_t currentTestStartTime_uS = (currentTestTime*secondsToMicros); // need a new test time that can handle half seconds

    // brightness should still be 1 at 1 second
    testObjects.timestamp->setTimestamp_uS(currentTestStartTime_uS + 1*secondsToMicros);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(1, testClass->getBrightnessLevel());
    
    // brightness should still be 1 before 1.875 seconds
    testObjects.timestamp->setTimestamp_uS(currentTestStartTime_uS + 1.87*secondsToMicros);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(1, testClass->getBrightnessLevel());

    // brightness should be 1.5 (i.e. 2) at 1.875 seconds
    testObjects.timestamp->setTimestamp_uS(currentTestStartTime_uS + (1.875 * secondsToMicros));
    testClass->updateLights();
    TEST_ASSERT_EQUAL(2, testClass->getBrightnessLevel());

    // brightness should still be 2 before 5.625 seconds
    testObjects.timestamp->setTimestamp_uS(currentTestStartTime_uS + (5.62 * secondsToMicros)-1);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(2, testClass->getBrightnessLevel());

    // brightness should be 5 at 15 seconds
    testObjects.timestamp->setTimestamp_uS(currentTestStartTime_uS + (15 * secondsToMicros));
    testClass->updateLights();
    TEST_ASSERT_EQUAL(5, testClass->getBrightnessLevel());

    // brightness should stay 5 after 15 seconds
    testObjects.timestamp->setTimestamp_uS(currentTestStartTime_uS + (150 * secondsToMicros));
    testClass->updateLights();
    TEST_ASSERT_EQUAL(5, testClass->getBrightnessLevel());

    // merge uS test time back to S test time as cleanup
    currentTestTime = (testObjects.timestamp->getTimestamp_uS()/secondsToMicros) + 1;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
  }

  // brightness can be set below default minimum from off
  const ModalConfigsStruct testConfigs3 = {
    .defaultOnBrightness = 10,
    .minOnBrightness = 5,
    .changeoverWindow = 0,
    .softChangeWindow = halfSoftChangeWindow_S*2
  };
  testObjects.configManager->setModalConfigs(testConfigs3);
  {
    const duty_t newBrightness = 8;
    testClass->setState(false);
    currentTestTime += halfSoftChangeWindow_S * 10;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
    TEST_ASSERT_EQUAL(testConfigs3.minOnBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(true, testClass->getState());
    
    // half soft change
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness1 = round((newBrightness+testConfigs3.minOnBrightness)/2.);
    TEST_ASSERT_EQUAL(expectedBrightness1, testClass->getBrightnessLevel());

    // at soft change end
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(newBrightness, testClass->getBrightnessLevel());

    // after soft change
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(newBrightness, testClass->getBrightnessLevel());
  }

  // change should be instant when soft change window=0
  const ModalConfigsStruct testConfigs4 = {
    .defaultOnBrightness = 5,
    .minOnBrightness = 1,
    .changeoverWindow = 0,
    .softChangeWindow = 0
  };
  testObjects.configManager->setModalConfigs(testConfigs4);
  {
    {
      const duty_t newBrightness = 255;
      TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
      TEST_ASSERT_EQUAL(newBrightness, testClass->getBrightnessLevel());
      currentTestTime += 1;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(newBrightness, testClass->getBrightnessLevel());
    }

    {
      const duty_t newBrightness = 100;
      TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
      TEST_ASSERT_EQUAL(newBrightness, testClass->getBrightnessLevel());
      currentTestTime += 1;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(newBrightness, testClass->getBrightnessLevel());
    }

    {
      const duty_t newBrightness = 0;
      TEST_ASSERT_EQUAL(true, testClass->getState());
      TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
      TEST_ASSERT_EQUAL(newBrightness, testClass->getBrightnessLevel());
      TEST_ASSERT_EQUAL(false, testClass->getState());
      currentTestTime += 1;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(newBrightness, testClass->getBrightnessLevel());
    }

    {
      const duty_t newBrightness = 50;
      TEST_ASSERT_EQUAL(false, testClass->getState());
      TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
      TEST_ASSERT_EQUAL(newBrightness, testClass->getBrightnessLevel());
      TEST_ASSERT_EQUAL(true, testClass->getState());
      currentTestTime += 1;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(newBrightness, testClass->getBrightnessLevel());
    }
  }

  // brightness can't be set below minOnBrightness
  const ModalConfigsStruct testConfigs5 = {
    .defaultOnBrightness = 10,
    .minOnBrightness = 5,
    .changeoverWindow = 0,
    .softChangeWindow = halfSoftChangeWindow_S * 2
  };
  testObjects.configManager->setModalConfigs(testConfigs5);
  {
    const duty_t minOnBrightness = testConfigs5.minOnBrightness;
    
    // set brightness to 0
    incrementTimeAndUpdate_S(1, testObjects);
    testClass->setBrightnessLevel(0);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(false, testClass->getState());

    // set brightness to 1
    testClass->setBrightnessLevel(1);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(minOnBrightness, testClass->getBrightnessLevel());
  
    // change to below minOnBrightness
    testClass->setBrightnessLevel(1);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(minOnBrightness, testClass->getBrightnessLevel());

    // can still be set to 0
    testClass->setBrightnessLevel(0);
    incrementTimeAndUpdate_S(2*halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(false, testClass->getState());
  }
  
  // set brightness mid soft-change should start a new soft change
  {
    const duty_t b0 = testConfigs5.minOnBrightness;
    const duty_t b1 = 255;

    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    // brightness adjustment 1
    testClass->setBrightnessLevel(b1);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    const duty_t expB1 = round((b0+b1)/2.);
    TEST_ASSERT_EQUAL(expB1, testClass->getBrightnessLevel());

    // brightness adjustment 2
    const duty_t b2 = 200;
    testClass->setBrightnessLevel(b2);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    const duty_t expB2 = round((expB1+b2)/2.);
    TEST_ASSERT_EQUAL(expB2, testClass->getBrightnessLevel());

    // end of second soft change
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(b2, testClass->getBrightnessLevel());
  }

  // set brightness then adjust mid soft-change. change should be immediate and interpolation should stop
  {
    const duty_t b0 = 200;
    const duty_t b1 = 100;
    const duty_t adj = 25;
    
    // set to b0
    testClass->setBrightnessLevel(b0);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(b0, testClass->getBrightnessLevel());

    // set to b1
    testClass->setBrightnessLevel(b1);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    const duty_t expB1 = round((b0+b1)/2.);
    TEST_ASSERT_EQUAL(expB1, testClass->getBrightnessLevel());

    // adjust down by 25
    testClass->adjustBrightness(adj, false);
    TEST_ASSERT_EQUAL(expB1-adj, testClass->getBrightnessLevel());
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(expB1-adj, testClass->getBrightnessLevel());
  }
}

void testSetState(){
  // test setState behaviour for background modes
  const TestChannels channel = TestChannels::RGB; // TODO: iterate over all channels

  const uint64_t testStartTime = mondayAtMidnight;
  uint64_t currentTestTime = testStartTime;

  const TestObjectsStruct testObjects = modalLightsFactoryAllModes(channel, testStartTime);

  const uint8_t halfSoftChangeWindow_S = 1;
  const ModalConfigsStruct testConfigs = {
    .defaultOnBrightness = 10,
    .minOnBrightness = 1,
    .changeoverWindow = 0,
    .softChangeWindow = halfSoftChangeWindow_S * 2
  };
  const duty_t minBrightness = testConfigs.minOnBrightness;
  testObjects.configManager->setModalConfigs(testConfigs);

  const std::shared_ptr<ModalLightsController> testClass = testObjects.modalLights;
  testObjects.timestamp->setTimestamp_S(currentTestTime);
  testClass->updateLights();

  // make sure current mode is constant brightness in background
  CurrentModeStruct currentModes = testClass->getCurrentModes();
  TEST_ASSERT_EQUAL(1, currentModes.backgroundMode);
  TEST_ASSERT_EQUAL(0, currentModes.activeMode);
  // set state to off should immediately turn lights off
  const duty_t brightness1 = 200;
  {
    // set brightness high
    testClass->setBrightnessLevel(brightness1);
    currentTestTime += 10*halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(brightness1, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    // lights should immediately be off
    testClass->setState(false);
    TEST_ASSERT_EQUAL(false, testClass->getState());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
    }
    TEST_ASSERT_EQUAL(brightness1, testClass->getBrightnessLevel());  // level shouldn't have changed

    // no change during or after softchange
    for(int i = 0; i < 5; i++){
      currentTestTime += halfSoftChangeWindow_S;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(false, testClass->getState());
      for(int i = 0; i < nChannels; i++){
        TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
      }
      TEST_ASSERT_EQUAL(brightness1, testClass->getBrightnessLevel());  // level shouldn't have changed
    }
  }
  // setting state to off again does nothing
  {
    testClass->setState(false);
    TEST_ASSERT_EQUAL(false, testClass->getState());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
    }
    TEST_ASSERT_EQUAL(brightness1, testClass->getBrightnessLevel());  // level shouldn't have changed

    // no change during or after softchange
    for(int i = 0; i < 5; i++){
      currentTestTime += halfSoftChangeWindow_S;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(false, testClass->getState());
      for(int i = 0; i < nChannels; i++){
        TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
      }
      TEST_ASSERT_EQUAL(brightness1, testClass->getBrightnessLevel());  // level shouldn't have changed
    }
  }

  // setting state to on should soft change to previous brightness
  {
    testClass->setState(true);
    TEST_ASSERT_EQUAL(true, testClass->getState());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(minBrightness, currentChannelValues[i]);
    }
    TEST_ASSERT_EQUAL(minBrightness, testClass->getBrightnessLevel());

    // half soft change
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(true, testClass->getState());
    const duty_t expectedBrightness = round((brightness1+minBrightness)/2.);
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(expectedBrightness, currentChannelValues[i]);
    }
    TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());

    // after soft change
    currentTestTime += 2*halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(true, testClass->getState());
      TEST_ASSERT_EQUAL(brightness1, currentChannelValues[i]);
    }
    TEST_ASSERT_EQUAL(brightness1, testClass->getBrightnessLevel());
  }

  // setting state to on again does nothing
  {
    TEST_ASSERT_EQUAL(true, testClass->getState());
    testClass->setState(true);
    TEST_ASSERT_EQUAL(true, testClass->getState());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(brightness1, currentChannelValues[i]);
    }
    TEST_ASSERT_EQUAL(brightness1, testClass->getBrightnessLevel());  // level shouldn't have changed

    // no change during or after softchange
    for(int i = 0; i < 5; i++){
      currentTestTime += halfSoftChangeWindow_S;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(true, testClass->getState());
      for(int i = 0; i < nChannels; i++){
        TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
      }
      TEST_ASSERT_EQUAL(brightness1, testClass->getBrightnessLevel());  // level shouldn't have changed
    }
  }

  // toggle state off and on
  {
    TEST_ASSERT_EQUAL(true, testClass->getState());
    testClass->toggleState();
    TEST_ASSERT_EQUAL(false, testClass->getState());

    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();

    TEST_ASSERT_EQUAL(false, testClass->getState());
    testClass->toggleState();
    TEST_ASSERT_EQUAL(true, testClass->getState());
  }
}

void testActiveBehaviour(){
  // test an active mode changes quickly, and cancels to background when state set to off
  const TestChannels channel = TestChannels::RGB; // TODO: iterate over all channels

  const uint64_t testStartTime = mondayAtMidnight;
  uint64_t currentTestTime = testStartTime;

  const TestObjectsStruct testObjects = modalLightsFactoryAllModes(channel, testStartTime);

  const duty_t colourChangeWindow = 10;
  const uint8_t halfSoftChangeWindow_S = 1;
  const ModalConfigsStruct testConfigs = {
    .defaultOnBrightness = 10,
    .minOnBrightness = 1,
    .changeoverWindow = colourChangeWindow,
    .softChangeWindow = halfSoftChangeWindow_S * 2
  };
  const duty_t minBrightness = testConfigs.minOnBrightness;
  testObjects.configManager->setModalConfigs(testConfigs);

  const std::shared_ptr<ModalLightsController> testClass = testObjects.modalLights;
  testObjects.timestamp->setTimestamp_S(currentTestTime);
  testClass->updateLights();

  // make sure current mode is constant brightness in background
  CurrentModeStruct currentModes = testClass->getCurrentModes();
  TEST_ASSERT_EQUAL(1, currentModes.backgroundMode);
  TEST_ASSERT_EQUAL(0, currentModes.activeMode);

  // set brightness to max
  const duty_t brightness = 255;
  testClass->setBrightnessLevel(brightness);
  currentTestTime += 10 * halfSoftChangeWindow_S;
  testObjects.timestamp->setTimestamp_S(currentTestTime);
  testClass->updateLights();
  TEST_ASSERT_EQUAL(true, testClass->getState());
  TEST_ASSERT_EQUAL(brightness, testClass->getBrightnessLevel());

  // load purple as active mode
  const TestModeDataStruct activeMode = testOnlyModes["purpleConstBrightness"];
  testClass->setModeByUUID(activeMode.ID, currentTestTime, true);
  testClass->updateLights();
  currentModes = testClass->getCurrentModes();
  TEST_ASSERT_EQUAL(1, currentModes.backgroundMode);
  TEST_ASSERT_EQUAL(activeMode.ID, currentModes.activeMode);

  // double check the current colours
  for(int i = 0; i < nChannels; i++){
    TEST_ASSERT_EQUAL(brightness, currentChannelValues[i]);
  }
  PrintDebug_reset;
  // test colour change
  {
    const duty_t* activeColours = activeMode.endColourRatios.RGB;
    const uint64_t t0_uS = currentTestTime * secondsToMicros;
    for(uint8_t K = 0; K <= 10; K++){
      const uint64_t dT_uS = round(testConfigs.changeoverWindow * (K/10.))*secondsToMicros;
      testObjects.timestamp->setTimestamp_uS(t0_uS + dT_uS);
      testClass->updateLights();

      for(uint8_t i = 0; i < nChannels; i++){
        const int16_t bDiff = activeColours[i] - brightness;
        const duty_t expectedColourBrightness = static_cast<duty_t>(round(brightness + bDiff*K/10.));
        std::string message = "K = " + std::to_string(K) + "; i = " + std::to_string(i) + "; final brightness = " + std::to_string(activeColours[i]);
        TEST_ASSERT_EQUAL_MESSAGE(expectedColourBrightness, currentChannelValues[i], message.c_str());
      }
    }

    // re-sync currentTestTime as cleanup
    currentTestTime = 1+ (testObjects.timestamp->getTimestamp_uS()/secondsToMicros);
    testObjects.timestamp->setTimestamp_S(currentTestTime);

    // end of colour change
    testClass->updateLights();
    TEST_ASSERT_EQUAL_COLOURS(activeColours, brightness, currentChannelValues, nChannels);
  }
  
  // setting state to false should switch active to background colours
  {
    testClass->setState(false);
    TEST_ASSERT_EQUAL(true, testClass->getState());
    TEST_ASSERT_EQUAL(brightness, testClass->getBrightnessLevel());

    const CurrentModeStruct currentModes = testClass->getCurrentModes();
    TEST_ASSERT_EQUAL(0, currentModes.activeMode);
    TEST_ASSERT_EQUAL(1, currentModes.backgroundMode);

    const duty_t* activeColours = activeMode.endColourRatios.RGB;
    const uint64_t t0_uS = currentTestTime * secondsToMicros;
    for(uint8_t K = 0; K <= 10; K++){
      const uint64_t dT_uS = round(testConfigs.changeoverWindow * (K/10.))*secondsToMicros;
      testObjects.timestamp->setTimestamp_uS(t0_uS + dT_uS);
      testClass->updateLights();

      for(uint8_t i = 0; i < nChannels; i++){
        const int16_t bDiff = brightness - activeColours[i];
        const duty_t expectedColourBrightness = static_cast<duty_t>(round(activeColours[i] + bDiff*K/10.));
        std::string message = "K = " + std::to_string(K) + "; i = " + std::to_string(i) + "; initial brightness = " + std::to_string(activeColours[i]);
        TEST_ASSERT_EQUAL_MESSAGE(expectedColourBrightness, currentChannelValues[i], message.c_str());
      }
    }

    // re-sync currentTestTime as cleanup
    currentTestTime = 1+ (testObjects.timestamp->getTimestamp_uS()/secondsToMicros);
    testObjects.timestamp->setTimestamp_S(currentTestTime);

    // after end of colour change
    testClass->updateLights();
    for(int c = 0; c < nChannels; c++){
      TEST_ASSERT_EQUAL(brightness, currentChannelValues[c]);
    }   
  }

  // loading active with trigger time in the past shouldn't effect colour change
  {
    testClass->setModeByUUID(activeMode.ID, currentTestTime - 2*60*60, true);
    testClass->updateLights();
    CurrentModeStruct currentModes = testClass->getCurrentModes();
    TEST_ASSERT_EQUAL(1, currentModes.backgroundMode);
    TEST_ASSERT_EQUAL(activeMode.ID, currentModes.activeMode);

    // same colour change test as above
    const duty_t* activeColours = activeMode.endColourRatios.RGB;
    const uint64_t t0_uS = currentTestTime * secondsToMicros;
    for(uint8_t K = 0; K <= 10; K++){
      const uint64_t dT_uS = round(testConfigs.changeoverWindow * (K/10.))*secondsToMicros;
      testObjects.timestamp->setTimestamp_uS(t0_uS + dT_uS);
      testClass->updateLights();

      for(uint8_t i = 0; i < nChannels; i++){
        const int16_t bDiff = activeColours[i] - brightness;
        const duty_t expectedColourBrightness = static_cast<duty_t>(round(brightness + bDiff*K/10.));
        std::string message = "K = " + std::to_string(K) + "; i = " + std::to_string(i) + "; final brightness = " + std::to_string(activeColours[i]);
        TEST_ASSERT_EQUAL_MESSAGE(expectedColourBrightness, currentChannelValues[i], message.c_str());
      }
    }

    // re-sync currentTestTime as cleanup
    currentTestTime = 1+ (testObjects.timestamp->getTimestamp_uS()/secondsToMicros);
    testObjects.timestamp->setTimestamp_S(currentTestTime);

    // end of colour change
    testClass->updateLights();
    TEST_ASSERT_EQUAL_COLOURS(activeColours, brightness, currentChannelValues, nChannels);
  }
  
  // brightness cannot be set or adjusted below default
  {
    // test that brightness can be set
    const duty_t newBrightness = 100;
    TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
    currentTestTime += 60*60;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(newBrightness, testClass->getBrightnessLevel());

    // setting brightness to 0 actually sets to default min
    const duty_t defaultBrightness = testConfigs.defaultOnBrightness;
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->setBrightnessLevel(0));
    currentTestTime += 60*60;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(true, testClass->getState());
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getBrightnessLevel());

    // test that brightness can be adjusted
    TEST_ASSERT_EQUAL(50 + defaultBrightness, testClass->adjustBrightness(50, true));
    currentTestTime += 60*60;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(50 + defaultBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    // test that brightness can't be set below default
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->adjustBrightness(52, false));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    currentTestTime += 60*60;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    // brightness can't be adjusted to 0
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->adjustBrightness(255, false));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    currentTestTime += 60*60;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(true, testClass->getState());
  }
  
  // setting state to on does nothing
  {
    duty_t startChannelValues[nChannels];
    memcpy(startChannelValues, currentChannelValues, nChannels);
    testClass->setState(true);
    TEST_ASSERT_EQUAL(true, testClass->getState());

    currentTestTime += colourChangeWindow;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL_UINT8_ARRAY(startChannelValues, currentChannelValues, nChannels);

    const CurrentModeStruct currentModes = testClass->getCurrentModes();
    TEST_ASSERT_EQUAL(activeMode.ID, currentModes.activeMode);
    TEST_ASSERT_EQUAL(1, currentModes.backgroundMode);
  }

  // toggle state is identical to setState(false)
  {
    testClass->toggleState();
    TEST_ASSERT_EQUAL(true, testClass->getState());
    const CurrentModeStruct currentModes = testClass->getCurrentModes();
    TEST_ASSERT_EQUAL(0, currentModes.activeMode);
    TEST_ASSERT_EQUAL(1, currentModes.backgroundMode);
  }

  // loading active mode should turn lights on if brightness or state = 0, without any colour change
  {
    // set brightness to 0
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
    TEST_ASSERT_EQUAL(0, testClass->setBrightnessLevel(0));
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(false, testClass->getState());

    const uint64_t triggerTime_S = incrementTimeAndUpdate_S(60*60, testObjects);

    testClass->setModeByUUID(activeMode.ID, triggerTime_S, true);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(activeMode.ID, testClass->getCurrentModes().activeMode);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(testConfigs.defaultOnBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    incrementTimeAndUpdate_S(colourChangeWindow, testObjects);

    TEST_ASSERT_EQUAL(testConfigs.defaultOnBrightness, testClass->getBrightnessLevel());
  }

  // loading active mode when brightness is under default should raise brightness to default
  {
    // set mode to background and brightness to under default
    testClass->setState(false);
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
    testClass->setBrightnessLevel(testConfigs.minOnBrightness);
    const uint64_t triggerTime_S = incrementTimeAndUpdate_S(60*60, testObjects);
    TEST_ASSERT_TRUE(testClass->getBrightnessLevel() < testConfigs.defaultOnBrightness);
    TEST_ASSERT_EQUAL(true, testClass->getState());

    // set active mode
    testClass->setModeByUUID(activeMode.ID, triggerTime_S, true);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(activeMode.ID, testClass->getCurrentModes().activeMode);
    TEST_ASSERT_EQUAL(testConfigs.minOnBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    incrementTimeAndUpdate_S(colourChangeWindow, testObjects);

    TEST_ASSERT_EQUAL(testConfigs.defaultOnBrightness, testClass->getBrightnessLevel());
  }
  
  // loading an active mode then loading a background mode shouldn't change behaviour until active mode is cancelled (best tested through colour interpolation)
  {
    // active mode should still be set
    TEST_ASSERT_EQUAL(activeMode.ID, testClass->getCurrentModes().activeMode);
    testClass->setBrightnessLevel(255);
    const uint64_t triggerTime_S = incrementTimeAndUpdate_S(60, testObjects);
    
    // set new background mode
    const TestModeDataStruct newBackgroundMode = mvpModes["warmConstBrightness"];
    testClass->setModeByUUID(newBackgroundMode.ID, triggerTime_S, false);
    incrementTimeAndUpdate_S(1, testObjects);
    testClass->updateLights();
    const CurrentModeStruct currentModes = testClass->getCurrentModes();
    TEST_ASSERT_EQUAL(activeMode.ID, currentModes.activeMode);
    TEST_ASSERT_EQUAL(newBackgroundMode.ID, currentModes.backgroundMode);
    TEST_ASSERT_NOT_EQUAL(activeMode.ID, newBackgroundMode.ID); // make sure the modes are different

    incrementTimeAndUpdate_S(colourChangeWindow, testObjects);
    TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(activeMode.endColourRatios.RGB, currentChannelValues, nChannels); // active mode hasn't changed, so neither should the colour vals

    testClass->setState(false);
    testClass->updateLights();
    TEST_ASSERT_EQUAL_UINT8_ARRAY(activeMode.endColourRatios.RGB, currentChannelValues, nChannels); // colour vals should match active at T=0

    // test colour changeover from active mode to background mode
    const uint64_t dT_uS = round((colourChangeWindow*secondsToMicros)/10.);
    for(int K = 1; K <= 10; K++){
      // testObjects.timestamp->setTimestamp_uS(dT_uS*K);
      // testClass->updateLights();
      incrementTimeAndUpdate_uS(dT_uS, testObjects);
      for(int c = 0; c<nChannels; c++){
        int16_t bDiff = newBackgroundMode.endColourRatios.RGB[c] - activeMode.endColourRatios.RGB[c];
        duty_t expectedBrightness = static_cast<duty_t>(round((activeMode.endColourRatios.RGB[c] +(bDiff*K/10.))));
        std::string message = "K = " + std::to_string(K) + "; c = " + std::to_string(c) + ";";
        TEST_ASSERT_EQUAL_MESSAGE(expectedBrightness, currentChannelValues[c], message.c_str());
      }
    }
  }
}

void testColourChange(){
  // Test the colour changeover behaviour
  const TestChannels channel = TestChannels::RGB; // TODO: iterate over all channels

  const uint64_t testStartTime = mondayAtMidnight;
  uint64_t currentTestTime = testStartTime;

  const TestObjectsStruct testObjects = modalLightsFactoryAllModes(channel, testStartTime);

  const duty_t colourChangeWindow = 10;
  const uint8_t halfSoftChangeWindow_S = 1;
  const ModalConfigsStruct testConfigs = {
    .defaultOnBrightness = 10,
    .minOnBrightness = 1,
    .changeoverWindow = colourChangeWindow,
    .softChangeWindow = halfSoftChangeWindow_S * 2
  };
  const duty_t minBrightness = testConfigs.minOnBrightness;
  testObjects.configManager->setModalConfigs(testConfigs);

  const std::shared_ptr<ModalLightsController> testClass = testObjects.modalLights;
  testClass->setBrightnessLevel(255);
  incrementTimeAndUpdate_S(colourChangeWindow, testObjects);
  TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());
  TEST_ASSERT_EQUAL(true, testClass->getState());

  TEST_ASSERT_EQUAL(1, testClass->getCurrentModes().backgroundMode);
  const TestModeDataStruct newBackgroundMode = mvpModes["warmConstBrightness"];

  // set a background mode, set a different background mode with different colours, they should interpolate over the window
  {
    const CurrentModeStruct initialModes = testClass->getCurrentModes();
    TEST_ASSERT_EQUAL(0, initialModes.activeMode);
    TEST_ASSERT_EQUAL(1, initialModes.backgroundMode);
    
    const uint64_t currentTime = secondsToMicros*incrementTimeAndUpdate_S(1, testObjects);
    testClass->setModeByUUID(newBackgroundMode.ID, currentTime, false);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(newBackgroundMode.ID, testClass->getCurrentModes().backgroundMode);
    for(int K = 0; K<=10; K++){
      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, constantBrightnessColourRatios, newBackgroundMode.endColourRatios.RGB, K/10.);
      // incrementTimeAndUpdate_uS(round(secondsToMicros*K/10.), testObjects);
      testObjects.timestamp->setTimestamp_uS(currentTime + round(secondsToMicros*K*colourChangeWindow/10.));
      testClass->updateLights();
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
    }

    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(newBackgroundMode.endColourRatios.RGB, currentChannelValues, nChannels);
  }
  
  // no colour change when mode is switched when lights are off
  {
    testClass->setBrightnessLevel(0);
    uint64_t currentTime = incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());

    // set to constant brightness mode
    testClass->setModeByUUID(1, currentTestTime, false);
    const uint64_t testTime = secondsToMicros*incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());

    testClass->setBrightnessLevel(255);
    TEST_ASSERT_EQUAL(1, testClass->getCurrentModes().backgroundMode);
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
    // should see a soft change, but no colour change
    const uint64_t dT_uS = static_cast<uint64_t>(round(secondsToMicros*testConfigs.softChangeWindow/10.));
    const uint64_t startTime_uS = testObjects.timestamp->getTimestamp_uS();
    for(int K = 0; K<= 10; K++){
      
      testObjects.timestamp->setTimestamp_uS(dT_uS * K + startTime_uS);
      testClass->updateLights();

      duty_t expectedBrightness = interpolate(testConfigs.minOnBrightness, 255, K/10.);
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      duty_t expectedColours[nChannels];
      fillChannelBrightness(expectedColours, constantBrightnessColourRatios, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
    }

    incrementTimeAndUpdate_S(10, testObjects);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(constantBrightnessColourRatios, currentChannelValues, nChannels);
  }

  // set the brightness during colour change, brightness should change by soft change
  {
    const uint64_t startTime = incrementTimeAndUpdate_S(1, testObjects);
    TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(1, testClass->getCurrentModes().backgroundMode);
    testClass->setModeByUUID(newBackgroundMode.ID, startTime, false);
    testClass->updateLights();

    // move time forward by 1 second and test values
    incrementTimeAndUpdate_S(1, testObjects);
    {
      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, constantBrightnessColourRatios, newBackgroundMode.endColourRatios.RGB, 1./colourChangeWindow);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test only colour change is on-going
    }
    
    // set brightness to 100
    testClass->setBrightnessLevel(100);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    {
      duty_t expectedBrightness = interpolate(255, 100, 0.5);
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, constantBrightnessColourRatios, newBackgroundMode.endColourRatios.RGB, (1. + halfSoftChangeWindow_S)/colourChangeWindow);
      fillChannelBrightness(expectedColours, expectedColours, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test both interpolations are on-going
    }

    // end of soft change
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    {
      duty_t expectedBrightness = 100;
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, constantBrightnessColourRatios, newBackgroundMode.endColourRatios.RGB, (1. + 2*halfSoftChangeWindow_S)/colourChangeWindow);
      fillChannelBrightness(expectedColours, expectedColours, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test brightness interpolation is finished, but colour change is on-going
    }

    // change brightness at end of colour change minus halfSoftChangeWindow
    testObjects.timestamp->setTimestamp_S(startTime + colourChangeWindow - halfSoftChangeWindow_S);
    testClass->updateLights();
    testClass->setBrightnessLevel(255);
    {
      duty_t expectedBrightness = 100;
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, constantBrightnessColourRatios, newBackgroundMode.endColourRatios.RGB, (colourChangeWindow - (float)(halfSoftChangeWindow_S))/colourChangeWindow);
      fillChannelBrightness(expectedColours, expectedColours, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test both interpolations are ongoing
    }

    // end of colour change
    testObjects.timestamp->setTimestamp_S(startTime + colourChangeWindow);
    testClass->updateLights();
    {
      duty_t expectedBrightness = interpolate(100, 255, 0.5);
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      duty_t expectedColours[nChannels];
      fillChannelBrightness(expectedColours, newBackgroundMode.endColourRatios.RGB, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test colour change finished but soft change on-going
    }

    // end of interpolations
    testObjects.timestamp->setTimestamp_S(startTime + colourChangeWindow + halfSoftChangeWindow_S);
    testClass->updateLights();
    {
      duty_t expectedBrightness = 255;
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      duty_t expectedColours[nChannels];
      fillChannelBrightness(expectedColours, newBackgroundMode.endColourRatios.RGB, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test both interpolations are finished
    }

    // after interpolations
    incrementTimeAndUpdate_S(1, testObjects);
    {
      duty_t expectedBrightness = 255;
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      duty_t expectedColours[nChannels];
      fillChannelBrightness(expectedColours, newBackgroundMode.endColourRatios.RGB, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test both interpolations are finished
    }
  }
  
  // adjusting the brightness during colour change should be instant
  {
    testClass->setBrightnessLevel(255);
    const uint64_t startTime = incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(newBackgroundMode.ID, testClass->getCurrentModes().backgroundMode);

    const duty_t *initialColourRatios = newBackgroundMode.endColourRatios.RGB;
    const duty_t *endColourRatios = constantBrightnessColourRatios;

    testClass->setModeByUUID(1, startTime, false);
    testClass->updateLights();

    // move time forward by 1 second and test values
    incrementTimeAndUpdate_S(1, testObjects);
    {
      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, initialColourRatios, endColourRatios, 1./colourChangeWindow);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test only colour change is on-going
    }
    
    // set brightness to 100
    testClass->adjustBrightness(155, false);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    {
      duty_t expectedBrightness = 100;
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, initialColourRatios, endColourRatios, (1. + halfSoftChangeWindow_S)/colourChangeWindow);
      fillChannelBrightness(expectedColours, expectedColours, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test only colour change is on-going
    }

    // change brightness at end of colour change minus halfSoftChangeWindow
    testObjects.timestamp->setTimestamp_S(startTime + colourChangeWindow - halfSoftChangeWindow_S);
    testClass->updateLights();
    testClass->adjustBrightness(55, true);
    {
      duty_t expectedBrightness = 155;
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, initialColourRatios, endColourRatios, (colourChangeWindow - (float)(halfSoftChangeWindow_S))/colourChangeWindow);
      fillChannelBrightness(expectedColours, expectedColours, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test only colour change is on-going
    }

    // end of colour change
    testObjects.timestamp->setTimestamp_S(startTime + colourChangeWindow);
    testClass->updateLights();
    {
      duty_t expectedBrightness = 155;
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      duty_t expectedColours[nChannels];
      fillChannelBrightness(expectedColours, endColourRatios, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test both interpolations are finished
    }


    // after colour change
    incrementTimeAndUpdate_S(1, testObjects);
    TEST_ASSERT_EQUAL(155, testClass->getBrightnessLevel());
    duty_t expectedColours[nChannels];
    fillChannelBrightness(expectedColours, endColourRatios, 155);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
  }

  // TODO: modal lights should return a bit flag indicating on-going interpolations, where b1 == brightness, b10 == colour, b100 == main

  // set lights to off and set active mode. cancel mid soft-change. brightness should still soft-change to default on, while colours colour-change to background mode
  // ideally, the brightness interpolation shouldn't change, but colour-change starts anew from ratios at cancel time
  {
    const uint64_t testStartTime = incrementTimeAndUpdate_S(1, testObjects);
    const duty_t minB = testConfigs.minOnBrightness;
    const duty_t defaultB = testConfigs.defaultOnBrightness;
    
    const TestModeDataStruct backgroundMode = mvpModes["warmConstBrightness"];
    const TestModeDataStruct activeMode = testOnlyModes["purpleConstBrightness"];
    testClass->setModeByUUID(backgroundMode.ID, testStartTime, false);
    testClass->adjustBrightness(255, false);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    PrintDebug_reset;
    testClass->setModeByUUID(activeMode.ID, testStartTime, true);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(minB, testClass->getBrightnessLevel());

    // advance by half soft change
    PrintDebug_message("advance by half soft change");
    // const duty_t expB1 = round((minB+defaultB)/2.);
    const duty_t expB1 = round(minB + ((defaultB-minB)*1.)/colourChangeWindow);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(expB1, testClass->getBrightnessLevel());
    duty_t expColours1[nChannels];
    fillChannelBrightness(expColours1, activeMode.endColourRatios.RGB, expB1);
    PrintDebug_UINT8_array("expected colour vals", expColours1, nChannels);
    PrintDebug_UINT8_array("actual colour vals", currentChannelValues, nChannels);
    PrintDebug_UINT8("expected brightness", expB1);
    PrintDebug_UINT8_array("colour ratios", activeMode.endColourRatios.RGB, nChannels);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expColours1, currentChannelValues, nChannels);

    // cancel active mode
    testClass->cancelActiveMode();
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(defaultB, testClass->getBrightnessLevel());
    duty_t expColours2[nChannels];
    interpolateColourRatios(expColours2, activeMode.endColourRatios.RGB, backgroundMode.endColourRatios.RGB, ((float)halfSoftChangeWindow_S)/colourChangeWindow);
    fillChannelBrightness(expColours2, expColours2, defaultB);
  }

  // set light to below default and set active mode. change brightness mid-colour change
  // brightness window can be colour-change by default, but should be soft-change when set and no interpolation when adjusted
  {
    const uint64_t testStartTime = incrementTimeAndUpdate_S(1, testObjects);
    const duty_t minB = testConfigs.minOnBrightness;
    const duty_t defaultB = testConfigs.defaultOnBrightness;

    const TestModeDataStruct backgroundMode = mvpModes["warmConstBrightness"];
    const TestModeDataStruct activeMode = testOnlyModes["purpleConstBrightness"];
    testClass->setModeByUUID(backgroundMode.ID, testStartTime, false);
    testClass->adjustBrightness(255, true);
    TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());

    // set brightness to under default
    testClass->setBrightnessLevel(minB);
    // change time to when brightness is under default
    const duty_t expB1 = minB + 1;
    const uint64_t dT_uS = round(2.*halfSoftChangeWindow_S*secondsToMicros*(255.-expB1)/(255.-minB));
    const uint64_t currentTime_uS = incrementTimeAndUpdate_uS(dT_uS, testObjects);
    TEST_ASSERT_EQUAL(expB1, testClass->getBrightnessLevel());

    // set active mode
    testClass->setModeByUUID(activeMode.ID, currentTime_uS/secondsToMicros, true);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(defaultB, testClass->getBrightnessLevel());
  }

  // TODO: test with a time adjustment mid-way
}

void testTimeChanges(void){
  // test behaviour when time is changed. device time should automatically call modal lights controller
  // TODO: changing time in constant state shouldn't do anything
  // TODO: changing time during quick change or soft on shouldn't effect interpolation
  TEST_IGNORE_MESSAGE("not implemented");
}

void constBrightness_tests(){
  UNITY_BEGIN();
  RUN_TEST(testUpdateLights);
  RUN_TEST(testBrightnessAdjustment);
  RUN_TEST(testSetBrightness);
  RUN_TEST(testActiveBehaviour);
  RUN_TEST(testColourChange);
  RUN_TEST(testTimeChanges);
  UNITY_END();
}

} // end namespace