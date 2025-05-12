#ifndef __TEST_MODES_H__
#define __TEST_MODES_H__

#include <Arduino.h>
#include <ModalLights.h>
#include <etl/flat_map.h>

enum class TestChannels : channelID {
  white = static_cast<channelID>(Channels::white),
  // warm = static_cast<channelID>(Channels::warm),
  whiteAndWarm = static_cast<channelID>(Channels::white) + static_cast<channelID>(Channels::warm),
  RGB = static_cast<channelID>(Channels::red) + static_cast<channelID>(Channels::green) + static_cast<channelID>(Channels::blue)
};

struct ColourRatiosStruct{
  duty_t white[8] = {0,0,0,0,0,0,0,0};
  duty_t whiteAndWarm[8] = {0,0,0,0,0,0,0,0};
  duty_t RGB[8] = {0,0,0,0,0,0,0,0};
};

const ColourRatiosStruct emptyColourRatios;

struct TestModeDataStruct {
  const modeUUID ID = 0;
  const ModeTypes type = ModeTypes::nullMode;
  const ColourRatiosStruct endColourRatios;
  const ColourRatiosStruct startColourRatios;

  const duty_t brightness1 = 0; // max/end/target
  const duty_t brightness0 = 0; // min/start
  const duty_t finalBrightness1 = 0;  // final max brightness
  const duty_t finalBrightness0 = 0;  // final min brightness

  const uint8_t time[3] = {0, 0, 0};  // [rate/window/period][0][0]
                    // [initialPeriod][finalPeriod][chirpDuration]
                    // [timeWindow MSB][timeWindow LSB][0]
};

/**
 * @brief convert a TestModeDataStruct into a ModeDataStruct
 * 
 * @param testStruct 
 * @param channel 
 * @return ModeDataStruct 
 */
ModeDataStruct convertTestModeStruct(TestModeDataStruct testStruct, TestChannels channel){
  ModeDataStruct dataStruct{
    .ID = testStruct.ID,
    .type = testStruct.type,
    .maxBrightness = testStruct.brightness1,
    .minBrightness = testStruct.brightness0,
    .finalMaxBrightness = testStruct.finalBrightness1,
    .finalMinBrightness = testStruct.finalBrightness0
  };
  switch(channel){
    case TestChannels::white:
      for(uint8_t i = 0; i < nChannels; i++){
        dataStruct.endColourRatios[i] = testStruct.endColourRatios.white[i];
        dataStruct.startColourRatios[i] = testStruct.startColourRatios.white[i];
      }
      break;

    case TestChannels::whiteAndWarm:
      for(uint8_t i = 0; i < nChannels; i++){
        dataStruct.endColourRatios[i] = testStruct.endColourRatios.whiteAndWarm[i];
        dataStruct.startColourRatios[i] = testStruct.startColourRatios.whiteAndWarm[i];
      }
      break;

    case TestChannels::RGB:
      for(uint8_t i = 0; i < nChannels; i++){
        dataStruct.endColourRatios[i] = testStruct.endColourRatios.RGB[i];
        dataStruct.startColourRatios[i] = testStruct.startColourRatios.RGB[i];
      }
      break;
    default:
      throw("channel doesn't exist");
  }
  memcpy(dataStruct.time, testStruct.time, 3);
  return dataStruct;
}

/**
 * @brief converts a vector array of TestModeDataStructs to a vector array of ModeDataStructs
 * 
 * @param testArray 
 * @param channel 
 * @return std::vector<ModeDataStruct> 
 */
std::vector<ModeDataStruct> makeModeDataStructArray(std::vector<TestModeDataStruct> testArray, TestChannels channel){
  std::vector<ModeDataStruct> dataArray;
  for(auto& testStruct : testArray){
    dataArray.push_back(convertTestModeStruct(testStruct, channel));
  }
  return dataArray;
};

modeUUID makeTestModeID(){
  static modeUUID id = 255;
  id--;
  return id + 1;
}

/**
 * @brief makes a constant brightness test mode struct. if modeID = 0, it creates an ID starting from 255
 * 
 * @param colours 
 * @param modeID 
 * @return TestModeDataStruct 
 */
TestModeDataStruct makeConstBrightnessTestStruct(ColourRatiosStruct colours, duty_t activeMinB, modeUUID modeID=0){
  modeID = modeID == 0 ? makeTestModeID() : modeID;
  if(modeID == 1){
    throw("you can't have an ID of 1, its taken");
  }
  TestModeDataStruct testStruct = {
    .ID = modeID,
    .type = ModeTypes::constantBrightness,
    .endColourRatios = colours,
    .brightness0 = activeMinB,
  };
  return testStruct;
}

/* ##### the actual mode data #####*/

// mvp

// expected constant brightness buffer. this is hard wired into the source code, so shouldn't be constructed through structs
// const uint8_t defaultConstBrightnessBuffer[modePacketSize] = {1, 1, 255, 255, 255, 255, 255, 255, 255, 255};

const TestModeDataStruct defaultConstantBrightness = {
  .ID = 1,
  .type = ModeTypes::constantBrightness,
  .endColourRatios = ColourRatiosStruct{.white = {255}, .whiteAndWarm = {255, 255}, .RGB = {255, 255, 255}},
  .brightness0 = 0
};

// these modes are intended to be used for the mvp, so have explicit IDs
static etl::flat_map<std::string, TestModeDataStruct, 255> mvpModes = {
  {"warmConstBrightness", makeConstBrightnessTestStruct(ColourRatiosStruct{
    .white = {255},
    .whiteAndWarm = {0, 255},
    .RGB = {255, 192, 111}
  }, 100, 2)},
};

static etl::flat_map<std::string, TestModeDataStruct, 255> testOnlyModes = {
  {"purpleConstBrightness", makeConstBrightnessTestStruct(ColourRatiosStruct{
    .white = {255},
    .whiteAndWarm = {126, 255},
    .RGB = {142, 111, 255}
  }, 100)},
};

// chuck all the mvp and test modes into a single vector array
std::vector<TestModeDataStruct> getAllTestingModes(){
  std::vector<TestModeDataStruct> modeArray;
  for(const auto& pair : mvpModes){
    TestModeDataStruct mode = pair.second;
    modeArray.push_back(mode);
  }
  for(const auto& pair : testOnlyModes){
    TestModeDataStruct mode = pair.second;
    modeArray.push_back(mode);
  }
  return modeArray;
}

#endif