#ifndef __MODAL_LIGHTS_TEST_HELPERS__
#define __MODAL_LIGHTS_TEST_HELPERS__

#include "ModalLights.h"
#include "testModes.h"

#include "../../EventManager/test_EventManager/testEvents.h"
#include "../../ConfigStorageClass/testObjects.hpp"
#include "../../nativeMocksAndHelpers/mockStorageHAL.hpp"

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
  std::shared_ptr<DeviceTimeClass> deviceTime;

  std::shared_ptr<ConfigManagerTestObjects::FakeStorageAdapter> storageAdapter;
  std::shared_ptr<ConfigStorageClass> configClass;
  
  // this is to test ModalLights getting a mode from storage
  std::shared_ptr<MockStorageHAL> mockStorageHAL; // TODO: delete

  const modesVector_t initialModes;
  std::shared_ptr<ModalLightsController> modalLights;

  errorCode_t setConfigs(const ModalConfigsStruct newConfigs) const {
    byte serialized[maxConfigSize];
    ConfigStructFncs::serialize(serialized, newConfigs);
    return configClass->setConfig(serialized);
  }
};

TestObjectsStruct modalLightsFactory(TestChannels channel, const etl::vector<TestModeDataStruct, UINT8_MAX> modes, uint64_t startTime_S, ModalConfigsStruct initialConfigs){
  TestObjectsStruct testObjects = {.initialModes = makeModeDataStructArray(modes, channel)};
  
  // set the configs
  if(initialConfigs.minOnBrightness == 0){throw("initialConfigs.minOnBrightness == 0");}
  if(initialConfigs.softChangeWindow > 15){throw("initialConfigs.softChangeWindow > 15");}

  using namespace ConfigManagerTestObjects;
  etl::map<ModuleID, GenericConfigStruct, 255> initialConfigMap = {
    {ModuleID::modalLights, makeGenericConfigStruct(
      initialConfigs
    )}
  };
  testObjects.storageAdapter = std::make_shared<FakeStorageAdapter>(initialConfigMap, true, "modalLightsFactory");
  testObjects.configClass = std::make_shared<ConfigStorageClass>(testObjects.storageAdapter);
  

  testObjects.deviceTime = std::make_shared<DeviceTimeClass>(testObjects.configClass);
  testObjects.deviceTime->setLocalTimestamp2000(startTime_S, 0, 0);

  testObjects.mockStorageHAL = std::make_shared<MockStorageHAL>(testObjects.initialModes, getAllTestEvents());
  auto storage = std::make_shared<DataStorageClass>(testObjects.mockStorageHAL);
  storage->loadIDs();
  
  auto lightsClass = concreteLightsClassFactory<TestLEDClass>();
  testObjects.modalLights = std::make_shared<ModalLightsController>(
    concreteLightsClassFactory<TestLEDClass>(),
    testObjects.deviceTime,
    storage,
    testObjects.configClass
  );

  testObjects.configClass->loadAllConfigs();
  return testObjects;
}

TestObjectsStruct modalLightsFactoryAllModes(TestChannels channel, uint64_t startTime_S, ModalConfigsStruct initialConfigs){
  etl::vector<TestModeDataStruct, UINT8_MAX> modes = getAllTestingModes();
  return modalLightsFactory(channel, modes, startTime_S, initialConfigs);
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

#define TEST_ASSERT_CURRENT_MODES(expBackgroundID, expActiveID, modalLightsInstance)\
  TEST_ASSERT_EQUAL(expBackgroundID, modalLightsInstance->getCurrentModes().backgroundMode); \
  TEST_ASSERT_EQUAL(expActiveID, modalLightsInstance->getCurrentModes().activeMode)

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
void interpolateArrays(duty_t *expectedArr, const duty_t *c0, const duty_t *c1, float ratio, uint8_t size){
  if(ratio == 0){
    memcpy(expectedArr, c0, size);
  }
  if(ratio >= 1){
    memcpy(expectedArr, c1, size);
  }
  for(int c = 0; c < size; c++){
    int db = c1[c] - c0[c];
    expectedArr[c] = static_cast<duty_t>(round(c0[c] + db*ratio));
  }
}

void interpAndFillColourRatios(duty_t expectedArr[nChannels], duty_t expectedBrightness, const duty_t c0[nChannels], const duty_t c1[nChannels], float ratio){
  duty_t expectedRatios[nChannels];
  interpolateArrays(expectedRatios, c0, c1, ratio, nChannels);
  fillChannelBrightness(expectedArr, expectedRatios, expectedBrightness);
}

/**
 * @brief find the time difference to get to the desiredValue. returns the time diff in the same units as window
 * 
 * @param desiredValue 
 * @param initialValue 
 * @param finalValue 
 * @param window 
 * @return uint64_t 
 */
uint64_t getTimeDiffToInterpValue(const duty_t desiredValue, const duty_t initialValue, const duty_t finalValue, const uint64_t window){
  if(window == 0 || initialValue == finalValue){
    return 0;
  }

  const bool isIncreasing = (finalValue > initialValue);

  if(
    (isIncreasing && (desiredValue < initialValue || desiredValue > finalValue))
    || (!isIncreasing && (desiredValue > initialValue || desiredValue < finalValue))
  ){
    throw("you done goofed: desired value is out of bounds");
  }

  const duty_t top = isIncreasing ? (desiredValue - initialValue) : (initialValue - desiredValue);
  const duty_t bottom = isIncreasing ? (finalValue - initialValue) : (initialValue - finalValue);
  const duty_t round = bottom/2;
  
  return ((window * top) + round)/bottom;
}

#endif