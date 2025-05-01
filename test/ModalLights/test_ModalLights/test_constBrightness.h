#include <unity.h>
#include <ModalLights.h>

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

#define TEST_ASSERT_CURRENT_MODES(expBackgroundID, expActiveID, actualStruct)\
  TEST_ASSERT_EQUAL(expBackgroundID, actualStruct.backgroundMode); \
  TEST_ASSERT_EQUAL(expActiveID, actualStruct.activeMode)

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

void interpAndFillColourRatios(duty_t expectedArr[nChannels], duty_t expectedBrightness, const duty_t c0[nChannels], const duty_t c1[nChannels], float ratio){
  duty_t expectedRatios[nChannels];
  interpolateColourRatios(expectedRatios, c0, c1, ratio);
  fillChannelBrightness(expectedArr, expectedRatios, expectedBrightness);
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
    .softChangeWindow = halfSoftChangeWindow_S * 2
  };
  testObjects.configManager->setModalConfigs(testConfigs);

  const std::shared_ptr<ModalLightsController> testClass = testObjects.modalLights;

  TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
  TEST_ASSERT_EQUAL(1, testClass->getCurrentModes().backgroundMode);

  // adjust up and down within bounds
  {
    // 0 to 255 (up from off)
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
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
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(0, testClass->adjustBrightness(55, false));
    TEST_ASSERT_EQUAL(false, testClass->getState());
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
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

  // set state to off and adjusting brightness up should start from minimumOnBrightness
  const ModalConfigsStruct testConfigs2 = {
    .defaultOnBrightness = 10,
    .minOnBrightness = 5,
    .softChangeWindow = halfSoftChangeWindow_S * 2
  };
  testObjects.configManager->setModalConfigs(testConfigs2);
  const duty_t minOnBrightness2 = testConfigs2.minOnBrightness;
  {
    // adjust up to 200 first
    TEST_ASSERT_EQUAL(0, testClass->adjustBrightness(255, false));
    TEST_ASSERT_EQUAL(200, testClass->adjustBrightness(200 - minOnBrightness2 + 1, true));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    currentTestTime += halfSoftChangeWindow_S * 4;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(200, testClass->getBrightnessLevel());

    // turn off
    TEST_ASSERT_EQUAL(false, testClass->setState(false));
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(200, testClass->getSetBrightness());  // because we haven't touched the brightness level
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
    }

    // adjust the brightness to min
    TEST_ASSERT_TRUE(minOnBrightness2 > 1); // just making sure
    TEST_ASSERT_EQUAL(minOnBrightness2, testClass->adjustBrightness(1, true));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    for(int i = 1; i < nChannels; i++){
      TEST_ASSERT_EQUAL(minOnBrightness2, currentChannelValues[i]);
    }
  }

  // set state to off and adjusting brightness down should not affect brightness level when switched back on
  {
    // adjust up to 200 first
    TEST_ASSERT_EQUAL(0, testClass->adjustBrightness(255, false));
    TEST_ASSERT_EQUAL(200, testClass->adjustBrightness(200 - minOnBrightness2 + 1, true));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    currentTestTime += halfSoftChangeWindow_S * 4;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(200, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(200, testClass->getBrightnessLevel());

    // turn off
    TEST_ASSERT_EQUAL(false, testClass->setState(false));
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(200, testClass->getSetBrightness());  // because we haven't touched the brightness level
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
    }

    // adjust brightness down
    TEST_ASSERT_EQUAL(200, testClass->adjustBrightness(100, false));
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(200, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());

    // turn back on
    TEST_ASSERT_EQUAL(true, testClass->setState(true));
    TEST_ASSERT_EQUAL(200, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(minOnBrightness2, testClass->getBrightnessLevel());
    currentTestTime += halfSoftChangeWindow_S * 4;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(200, testClass->getSetBrightness());
    for(int i = 1; i < nChannels; i++){
      TEST_ASSERT_EQUAL(200, currentChannelValues[i]);
    }
  }

  // set brightness to high, then adjust to under minOnBrightness but above 0. brightness should be 0
  // const ModalConfigsStruct testConfigs2 = {
  //   .defaultOnBrightness = 10,
  //   .minOnBrightness = 5,
  //   .softChangeWindow = halfSoftChangeWindow_S * 2
  // };
  // testObjects.configManager->setModalConfigs(testConfigs2);
  {
    testClass->setBrightnessLevel(200);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(200, testClass->getSetBrightness());
    
    // adjust below min 
    testClass->adjustBrightness(199, false);
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
  }
  
  // adjust brightness to minOnBrightness. adjusting down by 1 should turn the lights off
  {
    // current brightness should be minOnBrightness
    testClass->setBrightnessLevel(minOnBrightness2);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(minOnBrightness2, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(minOnBrightness2, testClass->getBrightnessLevel());

    // adjust down by 1
    testClass->adjustBrightness(1, false);
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(false, testClass->getState());

    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(false, testClass->getState());
  }

  // adjust up from 0 should start with minOnBrightness
  {
    const duty_t defaultOnBrightness = testConfigs2.defaultOnBrightness;
    for(int n = 0; n <= defaultOnBrightness; n++){
      // set brightness to 0
      testClass->adjustBrightness(255, false);
      incrementTimeAndUpdate_S(1, testObjects);

      // adjust up by n + 1
      testClass->adjustBrightness(n+1, true);
      TEST_ASSERT_EQUAL(minOnBrightness2+n, testClass->getSetBrightness());
      TEST_ASSERT_EQUAL(minOnBrightness2+n, testClass->getBrightnessLevel());
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
    TEST_ASSERT_EQUAL(b0, testClass->getBrightnessLevel());

    // set to different brightness
    testClass->setBrightnessLevel(b1);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    const duty_t expB = round((b1+b0)/2.);
    TEST_ASSERT_EQUAL(expB, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(b1, testClass->getSetBrightness());

    // adjustment
    const duty_t adj = 1;
    testClass->adjustBrightness(adj, true);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(expB+adj, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(expB+adj, testClass->getBrightnessLevel());
  }

  // adjust by 0 during softChange should do nothing
  {
    // setup
    const duty_t b0 = 100;
    const duty_t b1 = 200;
    testClass->setBrightnessLevel(b0);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(b0, testClass->getBrightnessLevel());

    // set to different brightness
    testClass->setBrightnessLevel(b1);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    const duty_t expB = round((b1+b0)/2.);
    TEST_ASSERT_EQUAL(expB, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(b1, testClass->getSetBrightness());

    // adjustment
    testClass->adjustBrightness(0, true);
    TEST_ASSERT_EQUAL(b1, testClass->getSetBrightness());
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(b1, testClass->getBrightnessLevel());
  }
}

void testSetBrightness(){
  // TODO: make sure tests use getBrightnessLevel() where appropriate
  // test behaviour when the brightness is set to a value (should be soft change)
  const TestChannels channel = TestChannels::RGB; // TODO: iterate over all channels

  const uint64_t testStartTime = mondayAtMidnight;
  uint64_t currentTestTime = testStartTime;

  const TestObjectsStruct testObjects = modalLightsFactoryAllModes(channel, testStartTime);

  const uint8_t halfSoftChangeWindow_S = 1;
  const ModalConfigsStruct testConfigs = {
    .defaultOnBrightness = 10,
    .minOnBrightness = 1,
    .softChangeWindow = halfSoftChangeWindow_S * 2
  };
  testObjects.configManager->setModalConfigs(testConfigs);

  const std::shared_ptr<ModalLightsController> testClass = testObjects.modalLights;
  testObjects.timestamp->setTimestamp_S(currentTestTime);
  testClass->updateLights();
  
  // set to 255 from 0
  {
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(255, testClass->setBrightnessLevel(255));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    TEST_ASSERT_EQUAL(testConfigs.minOnBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(255, testClass->getSetBrightness());

    // test at half soft change
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness1 = round(255/2.);
    // TODO: keep checking for getBrightnessLevel()
    TEST_ASSERT_EQUAL(expectedBrightness1, testClass->getBrightnessLevel());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(expectedBrightness1, currentChannelValues[i]);
    }

    // after soft change finishes
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness2 = 255;
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getSetBrightness());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(expectedBrightness2, currentChannelValues[i]);
    }

    // stays at end brightness
    currentTestTime += halfSoftChangeWindow_S * 10;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getSetBrightness());
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
    TEST_ASSERT_EQUAL(newBrightness, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(oldBrightness, testClass->getBrightnessLevel());

    // test at half soft change
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(newBrightness, testClass->getSetBrightness());
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
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getSetBrightness());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(expectedBrightness2, currentChannelValues[i]);
    }

    // stays at end brightness
    currentTestTime += halfSoftChangeWindow_S * 10;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(expectedBrightness2, testClass->getSetBrightness());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(expectedBrightness2, currentChannelValues[i]);
    }
  }

  // change mode to warm, then set to 140 from 100
  const TestModeDataStruct modeData = mvpModes["warmConstBrightness"];
  TEST_ASSERT_CURRENT_MODES(1, 0, testClass->getCurrentModes());
  {
    const duty_t oldBrightness = 100;
    const duty_t newBrightness = 140;

    // change the mode
    testClass->setModeByUUID(modeData.ID, currentTestTime, false);
    testClass->updateLights();
    TEST_ASSERT_CURRENT_MODES(modeData.ID, 0, testClass->getCurrentModes());

    TEST_ASSERT_EQUAL(oldBrightness, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(true, testClass->getState());
    TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
    TEST_ASSERT_EQUAL(oldBrightness, testClass->getBrightnessLevel());
    
    // test at half soft change
    currentTestTime += halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    const duty_t expectedBrightness1 = round((oldBrightness + newBrightness)/2.);

    duty_t expColourChannels[nChannels];
    interpolateColourRatios(expColourChannels, constantBrightnessColourRatios, modeData.endColourRatios.RGB, 0.5);
    fillChannelBrightness(expColourChannels, expColourChannels, expectedBrightness1);
    
    
    TEST_ASSERT_EQUAL(expectedBrightness1, testClass->getBrightnessLevel());
    // TEST_ASSERT_COLOURS_WITHIN_1(modeData.endColourRatios.RGB, expectedBrightness1, currentChannelValues, nChannels);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expColourChannels, currentChannelValues, nChannels);

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

    TEST_ASSERT_EQUAL(oldBrightness, testClass->getSetBrightness());
    // TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
    TEST_ASSERT_EQUAL(oldBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(newBrightness, testClass->getSetBrightness());

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
  // TODO: no it should be minOnBrightness for background mode
  {
    
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
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
    .softChangeWindow = 15
  };
  testObjects.configManager->setModalConfigs(testConfigs2);
  {
    const duty_t minBrightness = testConfigs2.minOnBrightness;
    // const duty_t defaultBrightness = testConfigs2.defaultOnBrightness;
    const duty_t window_S = testConfigs2.softChangeWindow;
    const duty_t defaultBrightness = 5;

    // set brightness to 0
    TEST_ASSERT_EQUAL(0, testClass->setBrightnessLevel(0));
    currentTestTime += halfSoftChangeWindow_S*30;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(false, testClass->getState());

    // switch lights on, brightness should be 1
    currentTestTime += halfSoftChangeWindow_S*30;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    // testClass->setState(true);
    testClass->setBrightnessLevel(defaultBrightness);
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getSetBrightness());
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
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getSetBrightness());
    
    // brightness should still be 1 before 1.875 seconds
    testObjects.timestamp->setTimestamp_uS(currentTestStartTime_uS + 1.87*secondsToMicros);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(1, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getSetBrightness());

    // brightness should be 1.5 (i.e. 2) at 1.875 seconds
    testObjects.timestamp->setTimestamp_uS(currentTestStartTime_uS + (1.875 * secondsToMicros));
    testClass->updateLights();
    TEST_ASSERT_EQUAL(2, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getSetBrightness());

    // brightness should still be 2 before 5.625 seconds
    testObjects.timestamp->setTimestamp_uS(currentTestStartTime_uS + (5.62 * secondsToMicros)-1);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(2, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getSetBrightness());

    // brightness should be 5 at 15 seconds
    testObjects.timestamp->setTimestamp_uS(currentTestStartTime_uS + (15 * secondsToMicros));
    testClass->updateLights();
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getSetBrightness());

    // brightness should stay 5 after 15 seconds
    testObjects.timestamp->setTimestamp_uS(currentTestStartTime_uS + (150 * secondsToMicros));
    testClass->updateLights();
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getBrightnessLevel());

    // merge uS test time back to S test time as cleanup
    currentTestTime = (testObjects.timestamp->getTimestamp_uS()/secondsToMicros) + 1;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
  }

  // change should be instant when soft change window=0
  const ModalConfigsStruct testConfigs3 = {
    .defaultOnBrightness = 5,
    .minOnBrightness = 1,
    .softChangeWindow = 0
  };
  testObjects.configManager->setModalConfigs(testConfigs3);
  {
    {
      const duty_t newBrightness = 255;
      TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
      TEST_ASSERT_EQUAL(newBrightness, testClass->getSetBrightness());
      currentTestTime += 1;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(newBrightness, testClass->getSetBrightness());
    }

    {
      const duty_t newBrightness = 100;
      TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
      TEST_ASSERT_EQUAL(newBrightness, testClass->getSetBrightness());
      currentTestTime += 1;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(newBrightness, testClass->getSetBrightness());
    }

    {
      const duty_t newBrightness = 0;
      TEST_ASSERT_EQUAL(true, testClass->getState());
      TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
      TEST_ASSERT_EQUAL(newBrightness, testClass->getSetBrightness());
      TEST_ASSERT_EQUAL(false, testClass->getState());
      currentTestTime += 1;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(newBrightness, testClass->getSetBrightness());
    }

    {
      const duty_t newBrightness = 50;
      TEST_ASSERT_EQUAL(false, testClass->getState());
      TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
      TEST_ASSERT_EQUAL(newBrightness, testClass->getSetBrightness());
      TEST_ASSERT_EQUAL(true, testClass->getState());
      currentTestTime += 1;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(newBrightness, testClass->getSetBrightness());
    }
  }

  // brightness can't be set below minOnBrightness
  const ModalConfigsStruct testConfigs4 = {
    .defaultOnBrightness = 10,
    .minOnBrightness = 5,
    .softChangeWindow = halfSoftChangeWindow_S * 2
  };
  testObjects.configManager->setModalConfigs(testConfigs4);
  {
    const duty_t minOnBrightness = testConfigs4.minOnBrightness;
    TEST_ASSERT_TRUE(minOnBrightness > 2);  // just making sure
    
    // set brightness to 0
    incrementTimeAndUpdate_S(1, testObjects);
    testClass->setBrightnessLevel(0);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(false, testClass->getState());

    // set brightness to 1
    testClass->setBrightnessLevel(1);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(false, testClass->getState());
  
    // change to above minOnBrightness
    testClass->setBrightnessLevel(minOnBrightness + 1);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(minOnBrightness + 1, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(minOnBrightness+1, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    // set to below min brightness
    testClass->setBrightnessLevel(minOnBrightness-1);
    incrementTimeAndUpdate_S(2*halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(false, testClass->getState());
  }
  
  // set brightness mid soft-change should start a new soft change
  {
    const duty_t b0 = testConfigs4.minOnBrightness;
    const duty_t b1 = 255;

    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    // brightness adjustment 1
    testClass->setBrightnessLevel(b1);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    const duty_t expB1 = round((b0+b1)/2.);
    TEST_ASSERT_EQUAL(expB1, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(b1, testClass->getSetBrightness());

    // brightness adjustment 2
    const duty_t b2 = 200;
    testClass->setBrightnessLevel(b2);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    const duty_t expB2 = round((expB1+b2)/2.);
    TEST_ASSERT_EQUAL(expB2, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(b2, testClass->getSetBrightness());

    // end of second soft change
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(b2, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(b2, testClass->getSetBrightness());
  }

  // set brightness then adjust mid soft-change. change should be immediate and interpolation should stop
  {
    const duty_t b0 = 200;
    const duty_t b1 = 100;
    const duty_t adj = 25;
    
    // set to b0
    testClass->setBrightnessLevel(b0);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(b0, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(b0, testClass->getBrightnessLevel());

    // set to b1
    testClass->setBrightnessLevel(b1);
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    const duty_t expB1 = round((b0+b1)/2.);
    TEST_ASSERT_EQUAL(expB1, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(b1, testClass->getSetBrightness());

    // adjust down by 25
    testClass->adjustBrightness(adj, false);
    TEST_ASSERT_EQUAL(expB1-adj, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(expB1-adj, testClass->getBrightnessLevel());
    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(expB1-adj, testClass->getSetBrightness());
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
    TEST_ASSERT_EQUAL(brightness1, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    // lights should immediately be off
    testClass->setState(false);
    TEST_ASSERT_EQUAL(false, testClass->getState());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
    }
    TEST_ASSERT_EQUAL(brightness1, testClass->getSetBrightness());  // level shouldn't have changed

    // no change during or after softchange
    for(int i = 0; i < 5; i++){
      currentTestTime += halfSoftChangeWindow_S;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(false, testClass->getState());
      for(int i = 0; i < nChannels; i++){
        TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
      }
      TEST_ASSERT_EQUAL(brightness1, testClass->getSetBrightness());  // level shouldn't have changed
    }
  }
  // setting state to off again does nothing
  {
    testClass->setState(false);
    TEST_ASSERT_EQUAL(false, testClass->getState());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
    }
    TEST_ASSERT_EQUAL(brightness1, testClass->getSetBrightness());  // level shouldn't have changed

    // no change during or after softchange
    for(int i = 0; i < 5; i++){
      currentTestTime += halfSoftChangeWindow_S;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(false, testClass->getState());
      for(int i = 0; i < nChannels; i++){
        TEST_ASSERT_EQUAL(0, currentChannelValues[i]);
      }
      TEST_ASSERT_EQUAL(brightness1, testClass->getSetBrightness());  // level shouldn't have changed
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
    TEST_ASSERT_EQUAL(brightness1, testClass->getSetBrightness());


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
    TEST_ASSERT_EQUAL(brightness1, testClass->getSetBrightness());

    // after soft change
    currentTestTime += 2*halfSoftChangeWindow_S;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(true, testClass->getState());
      TEST_ASSERT_EQUAL(brightness1, currentChannelValues[i]);
    }
    TEST_ASSERT_EQUAL(brightness1, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(brightness1, testClass->getSetBrightness());
  }

  // setting state to on again does nothing
  {
    TEST_ASSERT_EQUAL(true, testClass->getState());
    testClass->setState(true);
    TEST_ASSERT_EQUAL(true, testClass->getState());
    for(int i = 0; i < nChannels; i++){
      TEST_ASSERT_EQUAL(brightness1, currentChannelValues[i]);
    }
    TEST_ASSERT_EQUAL(brightness1, testClass->getSetBrightness());  // level shouldn't have changed

    // no change during or after softchange
    for(int i = 0; i < 5; i++){
      currentTestTime += halfSoftChangeWindow_S;
      testObjects.timestamp->setTimestamp_S(currentTestTime);
      testClass->updateLights();
      TEST_ASSERT_EQUAL(true, testClass->getState());
      for(int i = 0; i < nChannels; i++){
        TEST_ASSERT_EQUAL(brightness1, currentChannelValues[i]);
      }
      TEST_ASSERT_EQUAL(brightness1, testClass->getSetBrightness());  // level shouldn't have changed
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

  const uint8_t halfSoftChangeWindow_S = 1;
  const ModalConfigsStruct testConfigs = {
    .defaultOnBrightness = 10,
    .minOnBrightness = 1,
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
  TEST_ASSERT_EQUAL(brightness, testClass->getSetBrightness());

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
  // test colour change
  {
    const duty_t* activeColours = activeMode.endColourRatios.RGB;
    const uint64_t t0_uS = currentTestTime * secondsToMicros;
    TEST_ASSERT_TRUE(testConfigs.softChangeWindow > 0);
    const uint64_t dT_uS = round(testConfigs.softChangeWindow*secondsToMicros/10.);
    for(uint8_t K = 0; K <= 10; K++){
      const std::string message = "K = " + std::to_string(K);
      // const uint64_t dT_uS = round(testConfigs.softChangeWindow * (K/10.))*secondsToMicros;
      testObjects.timestamp->setTimestamp_uS(t0_uS + dT_uS*K);
      testClass->updateLights();

      TEST_ASSERT_EQUAL(brightness, testClass->getBrightnessLevel());
      duty_t expectedColourRatios[nChannels];
      interpolateColourRatios(expectedColourRatios, constantBrightnessColourRatios, activeColours, K/10.);
      duty_t expectedChannelValues[nChannels];
      fillChannelBrightness(expectedChannelValues, expectedColourRatios, brightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedChannelValues, currentChannelValues, nChannels, message.c_str());
    }

    // re-sync currentTestTime as cleanup
    currentTestTime = incrementTimeAndUpdate_S(1, testObjects);

    // end of colour change
    TEST_ASSERT_EQUAL_COLOURS(activeColours, brightness, currentChannelValues, nChannels);
  }
  
  // setting state to false should switch active to background colours
  {
    testClass->setState(false);
    TEST_ASSERT_EQUAL(true, testClass->getState());
    TEST_ASSERT_EQUAL(brightness, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(brightness, testClass->getBrightnessLevel());

    const CurrentModeStruct currentModes = testClass->getCurrentModes();
    TEST_ASSERT_EQUAL(0, currentModes.activeMode);
    TEST_ASSERT_EQUAL(1, currentModes.backgroundMode);

    const duty_t* activeColours = activeMode.endColourRatios.RGB;
    const uint64_t t0_uS = currentTestTime * secondsToMicros;
    const uint64_t dT_uS = round(testConfigs.softChangeWindow * secondsToMicros/10.);
    for(uint8_t K = 0; K <= 10; K++){
      const std::string message = "K = " + std::to_string(K);
      testObjects.timestamp->setTimestamp_uS(t0_uS + dT_uS*K);
      testClass->updateLights();

      TEST_ASSERT_EQUAL(brightness, testClass->getBrightnessLevel());
      duty_t expectedColourRatios[nChannels];
      interpolateColourRatios(expectedColourRatios, activeColours, constantBrightnessColourRatios, K/10.);
      duty_t expectedChannelValues[nChannels];
      fillChannelBrightness(expectedChannelValues, expectedColourRatios, brightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedChannelValues, currentChannelValues, nChannels, message.c_str());
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
    const uint64_t dT_uS = round(testConfigs.softChangeWindow * secondsToMicros /10.);
    for(uint8_t K = 0; K <= 10; K++){
      const std::string message = "K = " + std::to_string(K);
      testObjects.timestamp->setTimestamp_uS(t0_uS + dT_uS*K);
      testClass->updateLights();

      TEST_ASSERT_EQUAL(brightness, testClass->getBrightnessLevel());
      duty_t expectedColourRatios[nChannels];
      interpolateColourRatios(expectedColourRatios, constantBrightnessColourRatios, activeColours, K/10.);
      duty_t expectedChannelValues[nChannels];
      fillChannelBrightness(expectedChannelValues, expectedColourRatios, brightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedChannelValues, currentChannelValues, nChannels, message.c_str());
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
    // TODO: carefully re-write when default brightness is removed
    // test that brightness can be set
    const duty_t newBrightness = 100;
    TEST_ASSERT_EQUAL(newBrightness, testClass->setBrightnessLevel(newBrightness));
    currentTestTime += 60*60;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(newBrightness, testClass->getSetBrightness());

    // setting brightness to 0 actually sets to default min
    const duty_t defaultBrightness = testConfigs.defaultOnBrightness;
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->setBrightnessLevel(0));
    currentTestTime += 60*60;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(true, testClass->getState());
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getSetBrightness());

    // test that brightness can be adjusted
    TEST_ASSERT_EQUAL(50 + defaultBrightness, testClass->adjustBrightness(50, true));
    currentTestTime += 60*60;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(50 + defaultBrightness, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    // test that brightness can't be set below default
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->adjustBrightness(52, false));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    currentTestTime += 60*60;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    // brightness can't be adjusted to 0
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->adjustBrightness(255, false));
    TEST_ASSERT_EQUAL(true, testClass->getState());
    currentTestTime += 60*60;
    testObjects.timestamp->setTimestamp_S(currentTestTime);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(defaultBrightness, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(true, testClass->getState());
  }
  
  // setting state to on does nothing
  {
    duty_t startChannelValues[nChannels];
    memcpy(startChannelValues, currentChannelValues, nChannels);
    testClass->setState(true);
    TEST_ASSERT_EQUAL(true, testClass->getState());

    currentTestTime += halfSoftChangeWindow_S;
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

  // loading active mode should turn lights on if brightness = 0, without any colour change
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
    TEST_ASSERT_EQUAL(testConfigs.defaultOnBrightness, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);

    TEST_ASSERT_EQUAL(testConfigs.defaultOnBrightness, testClass->getSetBrightness());
  }

  // loading active mode should turn lights on if state = 0, without any colour change
  {
    // set brightness to high
    testClass->cancelActiveMode();
    testClass->setModeByUUID(1, 69, false);
    testClass->setBrightnessLevel(255);
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());
    testClass->setState(false);
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(255, testClass->getSetBrightness());


    const uint64_t triggerTime_S = incrementTimeAndUpdate_S(60*60, testObjects);

    testClass->setModeByUUID(activeMode.ID, triggerTime_S, true);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(activeMode.ID, testClass->getCurrentModes().activeMode);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(255, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);

    TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());
  }

  // loading active mode when brightness is under default should raise brightness to default
  {
    // set mode to background and brightness to under default
    testClass->setState(false);
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
    testClass->setBrightnessLevel(testConfigs.minOnBrightness);
    const uint64_t triggerTime_S = incrementTimeAndUpdate_S(60*60, testObjects);
    TEST_ASSERT_TRUE(testClass->getSetBrightness() < testConfigs.defaultOnBrightness);
    TEST_ASSERT_EQUAL(true, testClass->getState());

    // set active mode
    testClass->setModeByUUID(activeMode.ID, triggerTime_S, true);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(activeMode.ID, testClass->getCurrentModes().activeMode);
    TEST_ASSERT_EQUAL(testConfigs.minOnBrightness, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(testConfigs.defaultOnBrightness, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(true, testClass->getState());

    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);

    TEST_ASSERT_EQUAL(testConfigs.defaultOnBrightness, testClass->getSetBrightness());
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

    incrementTimeAndUpdate_S(halfSoftChangeWindow_S, testObjects);
    TEST_ASSERT_EQUAL(255, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(activeMode.endColourRatios.RGB, currentChannelValues, nChannels); // active mode hasn't changed, so neither should the colour vals

    testClass->setState(false);
    testClass->updateLights();
    TEST_ASSERT_EQUAL_UINT8_ARRAY(activeMode.endColourRatios.RGB, currentChannelValues, nChannels); // colour vals should match active at T=0

    const duty_t* startRatios = activeMode.endColourRatios.RGB;
    const duty_t *endRatios = newBackgroundMode.endColourRatios.RGB;

    // test colour changeover from active mode to background mode
    const uint64_t dT_uS = round((2*halfSoftChangeWindow_S*secondsToMicros)/10.);
    for(int K = 1; K <= 10; K++){
      const std::string message = "K = " + std::to_string(K);
      incrementTimeAndUpdate_uS(dT_uS, testObjects);

      duty_t expectedRatios[nChannels];
      interpolateColourRatios(expectedRatios, startRatios, endRatios, K/10.);
      duty_t expectedChannelValues[nChannels];
      fillChannelBrightness(expectedChannelValues, expectedRatios, brightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedChannelValues, currentChannelValues, nChannels, message.c_str());
    }
  }

  // loading a background mode, setting the lights to off then loading a different background mode shouldn't force lights on (it's an active-only behaviour)
  {
    // set first background mode
    testClass->setBrightnessLevel(69);
    const uint64_t time1 = incrementTimeAndUpdate_S(60, testObjects);
    testClass->setModeByUUID(1, time1, false);
    testClass->setState(false);
    const uint64_t time2 = incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
    TEST_ASSERT_EQUAL(1, testClass->getCurrentModes().backgroundMode);
    for(int c = 0; c<nChannels; c++){
      TEST_ASSERT_EQUAL(0, currentChannelValues[c]);
    }

    // second background mode
    testClass->setModeByUUID(activeMode.ID, time2, false);
    testClass->updateLights();
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
    TEST_ASSERT_EQUAL(activeMode.ID, testClass->getCurrentModes().backgroundMode);

    // lights should still be off, but previous brightness level preserved
    for(int c = 0; c<nChannels; c++){
      TEST_ASSERT_EQUAL(0, currentChannelValues[c]);
    }
    TEST_ASSERT_EQUAL(69, testClass->getSetBrightness());
  }
}

void testChangeover(){
  // Test the colour changeover behaviour
  const TestChannels channel = TestChannels::RGB; // TODO: iterate over all channels

  const uint64_t testStartTime = mondayAtMidnight;
  uint64_t currentTestTime = testStartTime;

  const TestObjectsStruct testObjects = modalLightsFactoryAllModes(channel, testStartTime);

  const duty_t halfWindow_S = 5;
  const ModalConfigsStruct testConfigs = {
    .defaultOnBrightness = 10,
    .minOnBrightness = 1,
    .softChangeWindow = halfWindow_S * 2
  };
  const duty_t minBrightness = testConfigs.minOnBrightness;
  testObjects.configManager->setModalConfigs(testConfigs);

  const std::shared_ptr<ModalLightsController> testClass = testObjects.modalLights;
  testClass->setBrightnessLevel(255);
  incrementTimeAndUpdate_S(halfWindow_S, testObjects);
  TEST_ASSERT_EQUAL(255, testClass->getSetBrightness());
  TEST_ASSERT_EQUAL(true, testClass->getState());

  TEST_ASSERT_EQUAL(1, testClass->getCurrentModes().backgroundMode);
  const TestModeDataStruct newBackgroundMode = mvpModes["warmConstBrightness"];

  // set a background mode, set a different background mode with different colours, they should interpolate over the window
  {
    incrementTimeAndUpdate_S(60, testObjects);
    const CurrentModeStruct initialModes = testClass->getCurrentModes();
    TEST_ASSERT_EQUAL(0, initialModes.activeMode);
    TEST_ASSERT_EQUAL(1, initialModes.backgroundMode);
    TEST_ASSERT_EQUAL(255, testClass->getBrightnessLevel());

    
    const uint64_t currentTime = secondsToMicros*incrementTimeAndUpdate_S(1, testObjects);
    testClass->setModeByUUID(newBackgroundMode.ID, currentTime, false);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(newBackgroundMode.ID, testClass->getCurrentModes().backgroundMode);
    const uint64_t dT_uS = round(secondsToMicros*2*halfWindow_S/10.);
    for(int K = 0; K<=10; K++){
      const std::string message = "K = " + std::to_string(K);
      testObjects.timestamp->setTimestamp_uS(currentTime + dT_uS*K);
      testClass->updateLights();

      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, constantBrightnessColourRatios, newBackgroundMode.endColourRatios.RGB, K/10.);
      
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedColours, currentChannelValues, nChannels, message.c_str());
    }

    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(newBackgroundMode.endColourRatios.RGB, currentChannelValues, nChannels);
  }
  
  // no colour change when mode is switched when brightness = 0
  {
    testClass->setBrightnessLevel(0);
    uint64_t currentTime = incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(newBackgroundMode.ID, testClass->getCurrentModes().backgroundMode);
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);

    // set to constant brightness mode
    testClass->setModeByUUID(1, currentTestTime, false);
    const uint64_t testTime = secondsToMicros*incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());

    testClass->setBrightnessLevel(255);
    TEST_ASSERT_EQUAL(1, testClass->getCurrentModes().backgroundMode);
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
    TEST_ASSERT_EQUAL(255, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(minBrightness, testClass->getBrightnessLevel());
    // should see a soft change, but no colour change
    const uint64_t dT_uS = static_cast<uint64_t>(round(secondsToMicros*testConfigs.softChangeWindow/10.));
    const uint64_t startTime_uS = testObjects.timestamp->getTimestamp_uS();
    for(int K = 0; K<= 10; K++){
      const std::string message = "K = " + std::to_string(K);
      
      testObjects.timestamp->setTimestamp_uS((dT_uS * K) + startTime_uS);
      testClass->updateLights();

      duty_t expectedBrightness = interpolate(testConfigs.minOnBrightness, 255, K/10.);
      TEST_ASSERT_EQUAL_MESSAGE(expectedBrightness, testClass->getBrightnessLevel(), message.c_str());
      duty_t expectedColours[nChannels];
      fillChannelBrightness(expectedColours, constantBrightnessColourRatios, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
    }

    incrementTimeAndUpdate_S(10, testObjects);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(constantBrightnessColourRatios, currentChannelValues, nChannels);
  }

  // no colour change when mode is switched when state == off
  {
    const uint64_t startTime = incrementTimeAndUpdate_S(10, testObjects);
    testClass->setModeByUUID(newBackgroundMode.ID, startTime, false);
    testClass->setBrightnessLevel(minBrightness);
    incrementTimeAndUpdate_S(60, testObjects);
    testClass->setState(false);
    uint64_t currentTime = incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(minBrightness, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(newBackgroundMode.ID, testClass->getCurrentModes().backgroundMode);
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);

    // set to constant brightness mode
    testClass->setModeByUUID(1, currentTestTime, false);
    const uint64_t testTime = secondsToMicros*incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(false, testClass->getState());
    TEST_ASSERT_EQUAL(minBrightness, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());

    const duty_t newB = 255;
    testClass->setBrightnessLevel(newB);
    TEST_ASSERT_EQUAL(1, testClass->getCurrentModes().backgroundMode);
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
    TEST_ASSERT_EQUAL(newB, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(minBrightness, testClass->getBrightnessLevel());
    // should see a soft change, but no colour change
    const uint64_t dT_uS = static_cast<uint64_t>(round(secondsToMicros*testConfigs.softChangeWindow/10.));
    const uint64_t startTime_uS = testObjects.timestamp->getTimestamp_uS();
    for(int K = 0; K<= 10; K++){
      const std::string message = "K = " + std::to_string(K);
      
      testObjects.timestamp->setTimestamp_uS((dT_uS * K) + startTime_uS);
      testClass->updateLights();

      duty_t expectedBrightness = interpolate(testConfigs.minOnBrightness, newB, K/10.);
      TEST_ASSERT_EQUAL_MESSAGE(expectedBrightness, testClass->getBrightnessLevel(), message.c_str());
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
    TEST_ASSERT_EQUAL(255, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(1, testClass->getCurrentModes().backgroundMode);
    testClass->setModeByUUID(newBackgroundMode.ID, startTime, false);
    testClass->updateLights();

    // move time forward by 1 second and test values
    incrementTimeAndUpdate_S(1, testObjects);
    {
      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, constantBrightnessColourRatios, newBackgroundMode.endColourRatios.RGB, 1./(2*halfWindow_S));
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test only colour change is on-going
    }
    
    // set brightness to 100, then move forwards by halfWindow_S (current time = halfWindow_S + 1)
    testClass->setBrightnessLevel(100);
    incrementTimeAndUpdate_S(halfWindow_S, testObjects);
    {
      duty_t expectedBrightness = interpolate(255, 100, 0.5);
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      TEST_ASSERT_EQUAL(100, testClass->getSetBrightness());

      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, constantBrightnessColourRatios, newBackgroundMode.endColourRatios.RGB, (1. + halfWindow_S)/(2*halfWindow_S));
      fillChannelBrightness(expectedColours, expectedColours, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test both interpolations are on-going
    }

    // end of colour change (current time = halfWindow_S*2)
    incrementTimeAndUpdate_S(halfWindow_S - 1, testObjects);
    {
      duty_t expectedBrightness = interpolate(255, 100, (halfWindow_S*2 - 1)/(2.*halfWindow_S));
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      duty_t expectedColours[nChannels];
      fillChannelBrightness(expectedColours, newBackgroundMode.endColourRatios.RGB, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test colour change is finished, but brightness interpolation is on-going
    }

    // end of interpolations
    incrementTimeAndUpdate_S(1, testObjects);
    {
      duty_t expectedBrightness = 100;
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());
      duty_t expectedColours[nChannels];
      fillChannelBrightness(expectedColours, newBackgroundMode.endColourRatios.RGB, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test both interpolations are finished
    }

    // after interpolations
    incrementTimeAndUpdate_S(1, testObjects);
    {
      duty_t expectedBrightness = 100;
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
    TEST_ASSERT_EQUAL(255, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(newBackgroundMode.ID, testClass->getCurrentModes().backgroundMode);

    const duty_t *initialColourRatios = newBackgroundMode.endColourRatios.RGB;
    const duty_t *endColourRatios = constantBrightnessColourRatios;

    testClass->setModeByUUID(1, startTime, false);
    testClass->updateLights();

    // move time forward by 1 second and test values
    // incrementTimeAndUpdate_S(1, testObjects);
    incrementTimeAndUpdate_S(halfWindow_S, testObjects);
    {
      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, initialColourRatios, endColourRatios, 0.5);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test only colour change is on-going
    }
    
    // set brightness to 100
    testClass->adjustBrightness(155, false);
    // incrementTimeAndUpdate_S(halfWindow_S, testObjects);
    {
      duty_t expectedBrightness = 100;
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getSetBrightness());
      duty_t expectedColours[nChannels];
      interpolateColourRatios(expectedColours, initialColourRatios, endColourRatios, 0.5);
      fillChannelBrightness(expectedColours, expectedColours, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test only colour change is on-going
    }

    // end of colour change
    incrementTimeAndUpdate_S(halfWindow_S, testObjects);
    {
      duty_t expectedBrightness = 100;
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getSetBrightness());
      TEST_ASSERT_EQUAL(expectedBrightness, testClass->getBrightnessLevel());

      duty_t expectedColours[nChannels];
      fillChannelBrightness(expectedColours, endColourRatios, expectedBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedColours, currentChannelValues, nChannels);
      // TODO: test both interpolations are finished
    }


    // after colour change
    incrementTimeAndUpdate_S(1, testObjects);
    TEST_ASSERT_EQUAL(100, testClass->getBrightnessLevel());
    duty_t expectedColours[nChannels];
    fillChannelBrightness(expectedColours, endColourRatios, 100);
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
    TEST_ASSERT_EQUAL(0, testClass->getSetBrightness());
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());

    testClass->setModeByUUID(activeMode.ID, testStartTime, true);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(minB, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(defaultB, testClass->getSetBrightness());
    duty_t expColours0[nChannels];
    fillChannelBrightness(expColours0, activeMode.endColourRatios.RGB, minB);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expColours0, currentChannelValues, nChannels);

    // advance by half soft change
    // const duty_t expB1 = round((minB+defaultB)/2.);
    incrementTimeAndUpdate_S(halfWindow_S, testObjects);
    const duty_t expB1 = interpolate(minB, defaultB, 0.5);
    TEST_ASSERT_EQUAL(expB1, testClass->getBrightnessLevel());
    duty_t expColours1[nChannels];
    fillChannelBrightness(expColours1, activeMode.endColourRatios.RGB, expB1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expColours1, currentChannelValues, nChannels);

    // cancel active mode at middle of brightness change
    testClass->cancelActiveMode();
    TEST_ASSERT_EQUAL(expB1, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(defaultB, testClass->getSetBrightness());

    // NOTE: switching modes resets the interpolation. keeping the previous brightness interpolation with different colours adds way more complexity than it's worth
    // Increment to end of changeover
    incrementTimeAndUpdate_S(halfWindow_S*2, testObjects);
    TEST_ASSERT_EQUAL(defaultB, testClass->getBrightnessLevel());
    duty_t expColours2[nChannels];
    interpolateColourRatios(expColours2, activeMode.endColourRatios.RGB, backgroundMode.endColourRatios.RGB, ((float)halfWindow_S)/halfWindow_S);
    fillChannelBrightness(expColours2, expColours2, defaultB);
  }

  // TODO: loading an active mode mid brightness change should end at the previous target brightness
  {
    testClass->setModeByUUID(1, 69, false);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
    TEST_ASSERT_EQUAL(1, testClass->getCurrentModes().backgroundMode);
    testClass->adjustBrightness(255, false);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    const uint64_t startTime = incrementTimeAndUpdate_S(60, testObjects);
    
    const duty_t maxB = 255;
    testClass->setBrightnessLevel(maxB);
    TEST_ASSERT_EQUAL(maxB, testClass->getSetBrightness());
    TEST_ASSERT_EQUAL(minBrightness, testClass->getBrightnessLevel());

    const uint64_t newTime = incrementTimeAndUpdate_S(halfWindow_S, testObjects);
    const duty_t expB1 = interpolate(minBrightness, maxB, 0.5);
    TEST_ASSERT_EQUAL(expB1, testClass->getBrightnessLevel());

    TestModeDataStruct activeMode = testOnlyModes["purpleConstBrightness"];
    testClass->setModeByUUID(activeMode.ID, startTime, false);
    testClass->updateLights();
    const duty_t *colours1 = constantBrightnessColourRatios;
    const duty_t *colours2 = activeMode.endColourRatios.RGB;

    // half brightness window, start of changeover
    {
      duty_t expChannelVals[nChannels];
      interpAndFillColourRatios(expChannelVals, expB1, colours1, colours2, 0);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expChannelVals, currentChannelValues, nChannels);
    }

    // end of brightness window, half changeover
    incrementTimeAndUpdate_S(halfWindow_S, testObjects);
    {
      duty_t expB2 = interpolate(expB1, maxB, 0.5);
      duty_t expChannelVals[nChannels];
      interpAndFillColourRatios(expChannelVals, expB2, colours1, colours2, 0.5);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expChannelVals, currentChannelValues, nChannels);
    }

    // end of changeover
    incrementTimeAndUpdate_S(halfWindow_S, testObjects);
    {
      duty_t expChannelVals[nChannels];
      interpAndFillColourRatios(expChannelVals, maxB, colours1, colours2, 1.);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expChannelVals, currentChannelValues, nChannels);
    }
  }

  // set light to below default and set active mode. change brightness mid-colour change
  {
    const uint64_t testStartTime = incrementTimeAndUpdate_S(1, testObjects);
    const duty_t minB = testConfigs.minOnBrightness;
    const duty_t defaultB = testConfigs.defaultOnBrightness;

    const TestModeDataStruct backgroundMode = mvpModes["warmConstBrightness"];
    const TestModeDataStruct activeMode = testOnlyModes["purpleConstBrightness"];
    testClass->setModeByUUID(backgroundMode.ID, testStartTime, false);
    testClass->adjustBrightness(255, true);
    TEST_ASSERT_EQUAL(255, testClass->getSetBrightness());

    // set brightness to under default
    testClass->setBrightnessLevel(defaultB-1);
    // change time to when brightness is under default
    const duty_t expB1 = defaultB-1 ;
    const uint64_t currentTime_S = incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(expB1, testClass->getSetBrightness());

    // set active mode
    testClass->setModeByUUID(activeMode.ID, currentTime_S, true);
    incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(defaultB, testClass->getSetBrightness());
  }

  // TODO: changing the background mode and turning the lights off and on within soft change window, should immediately change the colours to the new brightness (i.e. turning lights off mid change should end the colour interpolation)
  {
    const uint64_t startTime_S = incrementTimeAndUpdate_S(1, testObjects);
    TestModeDataStruct mode1 = testOnlyModes["purpleConstBrightness"];
    TestModeDataStruct mode2 = mvpModes["warmConstBrightness"];

    const duty_t *ratios1 = mode1.endColourRatios.RGB;
    const duty_t *ratios2 = mode2.endColourRatios.RGB;

    testClass->setModeByUUID(mode1.ID, startTime_S, false);
    testClass->cancelActiveMode();
    TEST_ASSERT_EQUAL(mode1.ID, testClass->getCurrentModes().backgroundMode);
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
    
    const duty_t brightness = 200;
    testClass->setBrightnessLevel(brightness);
    const uint64_t time2 = incrementTimeAndUpdate_S(60, testObjects);
    TEST_ASSERT_EQUAL(brightness, testClass->getBrightnessLevel());

    {
      duty_t expectedChannelVals[nChannels];
      fillChannelBrightness(expectedChannelVals, ratios1, brightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedChannelVals, currentChannelValues, nChannels);
    }

    // change background mode to mode2
    testClass->setModeByUUID(mode2.ID, time2, false);
    testClass->updateLights();
    incrementTimeAndUpdate_S(halfWindow_S, testObjects);

    {
      duty_t expChannelVals[nChannels];
      interpAndFillColourRatios(expChannelVals, brightness, ratios1, ratios2, 0.5);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expChannelVals, currentChannelValues, nChannels);
    }

    // turn lights off and on
    testClass->setState(false);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    testClass->setState(true);
    TEST_ASSERT_EQUAL(minBrightness, testClass->getBrightnessLevel());
    {
      duty_t expectedChannelVals[nChannels];
      fillChannelBrightness(expectedChannelVals, ratios2, minBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedChannelVals, currentChannelValues, nChannels);
    }

    // half changeover
    const uint64_t time3 = incrementTimeAndUpdate_S(halfWindow_S, testObjects);
    {
      duty_t expectedChannelVals[nChannels];
      duty_t expB = round((minBrightness+brightness)/2.);
      fillChannelBrightness(expectedChannelVals, ratios2, expB);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedChannelVals, currentChannelValues, nChannels);
    }

    // switch mode to mode 1
    testClass->setModeByUUID(mode1.ID, time3, false);
    testClass->updateLights();
    TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
    TEST_ASSERT_EQUAL(mode1.ID, testClass->getCurrentModes().backgroundMode);

    const uint64_t time4 = incrementTimeAndUpdate_S(halfWindow_S, testObjects);
    {
      const duty_t expB = round((round((minBrightness+brightness)/2.) + brightness)/2.);
      duty_t expChannelVals[nChannels];
      interpAndFillColourRatios(expChannelVals, expB, ratios2, ratios1, 0.5);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expChannelVals, currentChannelValues, nChannels);
    }

    // adjust brightness off and on
    testClass->adjustBrightness(255, false);
    TEST_ASSERT_EQUAL(0, testClass->getBrightnessLevel());
    testClass->adjustBrightness(brightness -minBrightness + 1, true);
    {
      duty_t expectedChannelVals[nChannels];
      duty_t expB = brightness;
      TEST_ASSERT_EQUAL(expB, testClass->getBrightnessLevel());
      fillChannelBrightness(expectedChannelVals, ratios1, expB);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedChannelVals, currentChannelValues, nChannels);
    }

    // half soft change
    incrementTimeAndUpdate_S(halfWindow_S, testObjects);
    {
      duty_t expectedChannelVals[nChannels];
      duty_t expB = brightness;
      TEST_ASSERT_EQUAL(expB, testClass->getBrightnessLevel());
      fillChannelBrightness(expectedChannelVals, ratios1, expB);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedChannelVals, currentChannelValues, nChannels);
    }
  }
  
  // TODO: test with a time adjustment mid-way
}

void testTimeChanges(void){
  // test behaviour when time is changed. device time should automatically call modal lights controller
  const TestChannels channel = TestChannels::RGB; // TODO: iterate over all channels

  const uint64_t testStartTime = mondayAtMidnight;
  uint64_t currentTestTime = testStartTime;

  const TestObjectsStruct testObjects = modalLightsFactoryAllModes(channel, testStartTime);

  const duty_t minB = 1;
  const ModalConfigsStruct testConfigs = {
    .defaultOnBrightness = defaultConfigs.defaultOnBrightness,
    .minOnBrightness = minB,
    .softChangeWindow = 6
  };
  testObjects.configManager->setModalConfigs(testConfigs);

  const std::shared_ptr<ModalLightsController> testClass = testObjects.modalLights;
  const auto deviceTime = testObjects.deviceTime;

  TEST_ASSERT_EQUAL(0, testClass->getCurrentModes().activeMode);
  TEST_ASSERT_EQUAL(1, testClass->getCurrentModes().backgroundMode);

  const duty_t maxB = 255;
  testClass->setBrightnessLevel(maxB);
  incrementTimeAndUpdate_S(60, testObjects);
  TEST_ASSERT_EQUAL(maxB, testClass->getBrightnessLevel());

  // TODO: changing time in constant state shouldn't do anything
  {
    testClass->updateLights();
    {
      duty_t expColourVals[nChannels];
      fillChannelBrightness(expColourVals, constantBrightnessColourRatios, maxB);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expColourVals, currentChannelValues, nChannels);
    }
    currentTestTime += 60*60;
    deviceTime->setLocalTimestamp2000(currentTestTime, 0, 0);
    testClass->updateLights();
    {
      duty_t expColourVals[nChannels];
      fillChannelBrightness(expColourVals, constantBrightnessColourRatios, maxB);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expColourVals, currentChannelValues, nChannels);
    }
    currentTestTime -= 60;
    deviceTime->setUTCTimestamp2000(currentTestTime, -16*60*60, 60*60);
    testClass->updateLights();
    {
      duty_t expColourVals[nChannels];
      fillChannelBrightness(expColourVals, constantBrightnessColourRatios, maxB);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expColourVals, currentChannelValues, nChannels);
    }
  }

  // TODO: changing time during changeover or soft change shouldn't effect interpolation
  {
    testClass->setBrightnessLevel(minB);
    TEST_ASSERT_EQUAL(minB, testClass->getBrightnessLevel());
    const uint64_t time = incrementTimeAndUpdate_S(60, testObjects);
    TestModeDataStruct newMode = mvpModes["warmConstBrightness"];
    const duty_t *ratios1 = constantBrightnessColourRatios;
    const duty_t *ratios2 = newMode.endColourRatios.RGB;

    // simultaneous brightness and colour change
    testClass->setModeByUUID(newMode.ID, time, false);
    testClass->setBrightnessLevel(maxB);
    TEST_ASSERT_EQUAL(minB, testClass->getBrightnessLevel());
    TEST_ASSERT_EQUAL(maxB, testClass->getSetBrightness());

    {
      duty_t expColourVals[nChannels];
      fillChannelBrightness(expColourVals, ratios1, minB);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expColourVals, currentChannelValues, nChannels);
    }

    incrementTimeAndUpdate_S(2, testObjects);
    {
      duty_t expColourVals[nChannels];
      float interpRatio = 1/3.;
      const duty_t expB = interpolate(minB, maxB, interpRatio);
      interpAndFillColourRatios(expColourVals, expB, ratios1, ratios2, interpRatio);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expColourVals, currentChannelValues, nChannels);
    }
    currentTestTime += 60*60;
    deviceTime->setLocalTimestamp2000(currentTestTime, 0, 0);
    incrementTimeAndUpdate_S(2, testObjects);
    {
      duty_t expColourVals[nChannels];
      float interpRatio = 2/3.;
      const duty_t expB = interpolate(minB, maxB, interpRatio);
      interpAndFillColourRatios(expColourVals, expB, ratios1, ratios2, interpRatio);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expColourVals, currentChannelValues, nChannels);
    }
    currentTestTime -= 60;
    deviceTime->setUTCTimestamp2000(currentTestTime, -16*60*60, 60*60);
    incrementTimeAndUpdate_S(1, testObjects);
    {
      duty_t expColourVals[nChannels];
      float interpRatio = 5/6.;
      const duty_t expB = interpolate(minB, maxB, interpRatio);
      interpAndFillColourRatios(expColourVals, expB, ratios1, ratios2, interpRatio);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expColourVals, currentChannelValues, nChannels);
    }
  }
}

void testConfigChanges(){
  // TODO: changes in minOnBrightness should be instant. if brightnesss is constant, change should be instant. if brightness is changing, adjust interpolation window so that interpolation ends at the same time. colour interpolation shouldn't be affected

  // TODO: if brightness is constant and b<minB0, b=minB1-1
  // TODO: if brightness is constant and b<minB1 & b>=minB0, b=minB1

  // TODO: if brightness is decreasing and bt>minB1, no change
  // TODO: if brightness is decreasing and bt< minB0, bt=minB1-1 and ends at the same time
  // TODO: if brightness is decreasing and bt<minB1 & bt>=minB0, bt = minB1
  // TODO: if b(c) <= minB1 & bt >= minB0, interpolation should stop and b=minB1
  // TODO: if b(c) < minB1 & bt < minB0, interpolation should stop and b=0

  // TODO: if brightness is increasing and b(c) >= minB1, no change
  // TODO: if brightness is increasing and b(c) < minB1, b(c) = minB1 and ends at the same time
  // TODO: if brightness is increasing and bt <= minB1, interpolation should stop and b=minB1
  
  // TODO: changes in changeover: if brightness and colours are constant, nothing happens. if brightness or colours is changing, rearrange interpolation to end at the time it would've, with the same initialVals but different window 

  TEST_IGNORE_MESSAGE("not implemented: config manager needs to alert ModalController of changes, and interpolation class needs to be completely dissasembled. also, remove defaultOnBrightness from configs first");
}

void constBrightness_tests(){
  UNITY_BEGIN();
  RUN_TEST(testUpdateLights);
  RUN_TEST(testBrightnessAdjustment);
  RUN_TEST(testSetBrightness);
  RUN_TEST(testSetState);
  RUN_TEST(testActiveBehaviour);
  RUN_TEST(testChangeover);
  RUN_TEST(testTimeChanges);
  RUN_TEST(testConfigChanges);
  UNITY_END();
}

} // end namespace