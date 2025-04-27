#pragma(once)

#include <Arduino.h>
#include <ModalLights.h>

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

struct ModeDataStruct {
  modeUUID ID;
  ModeTypes type;
  duty_t endColourRatios[nChannels] ;
  duty_t startColourRatios[nChannels];

  duty_t brightness1; // max/end/target
  duty_t brightness0; // min/start
  duty_t finalBrightness1;  // final max brightness
  duty_t finalBrightness0;  // final min brightness

  uint8_t time[3];  // [rate/window/period][0][0]
                    // [initialPeriod][finalPeriod][chirpDuration]
                    // [timeWindow MSB][timeWindow LSB][0]
};

void serializeModeDataStruct(ModeDataStruct dataStruct, uint8_t buffer[modePacketSize]){
  buffer[0] = dataStruct.ID;
  buffer[1] = static_cast<uint8_t>(dataStruct.type);
  switch(dataStruct.type){
    case ModeTypes::constantBrightness:
      for(uint8_t i = 0; i < nChannels; i++){
        buffer[2+i] = dataStruct.endColourRatios[i];
      }
      return;

    default:
      throw("mode type doesn't exist");
  }
}

ColourRatiosStruct emptyColourRatios;

struct TestModeDataStruct {
  const modeUUID ID = 0;
  const ModeTypes type = ModeTypes::nullMode;
  const ColourRatiosStruct endColourRatios = emptyColourRatios;
  const ColourRatiosStruct startColourRatios = emptyColourRatios;

  const duty_t brightness1 = 0; // max/end/target
  const duty_t brightness0 = 0; // min/start
  const duty_t finalBrightness1 = 0;  // final max brightness
  const duty_t finalBrightness0 = 0;  // final min brightness

  const uint8_t time[3] = {0, 0, 0};  // [rate/window/period][0][0]
                    // [initialPeriod][finalPeriod][chirpDuration]
                    // [timeWindow MSB][timeWindow LSB][0]
};

std::vector<ModeDataStruct> makeModeDataStructArray(std::vector<TestModeDataStruct> testArray, TestChannels channel){
  std::vector<ModeDataStruct> dataArray;
  for(auto& testStruct : testArray){
    ModeDataStruct dataStruct{.ID = testStruct.ID, .type = testStruct.type, .brightness1 = testStruct.brightness1, .brightness0 = testStruct.brightness0, .finalBrightness1 = testStruct.finalBrightness1, .finalBrightness0 = testStruct.finalBrightness0};
    for(uint8_t i = 0; i < nChannels; i++){
      switch(channel){
        case TestChannels::white:
          dataStruct.endColourRatios[i] = testStruct.endColourRatios.white[i];
          dataStruct.startColourRatios[i] = testStruct.startColourRatios.white[i];
          break;

        case TestChannels::whiteAndWarm:
          dataStruct.endColourRatios[i] = testStruct.endColourRatios.whiteAndWarm[i];
          dataStruct.startColourRatios[i] = testStruct.startColourRatios.whiteAndWarm[i];
          break;

        case TestChannels::RGB:
          dataStruct.endColourRatios[i] = testStruct.endColourRatios.RGB[i];
          dataStruct.startColourRatios[i] = testStruct.startColourRatios.RGB[i];
          break;
        default:
          throw("channel doesn't exist");
      }
    }
    memcpy(dataStruct.time, testStruct.time, 3);
    dataArray.push_back(dataStruct);
  }
  return dataArray;
}

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
TestModeDataStruct makeConstBrightnessTestStruct(ColourRatiosStruct colours, modeUUID modeID=0){
  modeID = modeID == 0 ? makeTestModeID() : modeID;
  if(modeID == 1){
    throw("you can't have an ID of 1, its taken");
  }
  TestModeDataStruct testStruct = {
    .ID = modeID,
    .type = ModeTypes::constantBrightness,
    .endColourRatios = colours,
  };
  return testStruct;
}

/* ##### the actual mode data #####*/

// mvp

// expected constant brightness buffer. this is hard wired into the source code, so shouldn't be constructed through structs
const uint8_t defaultConstBrightnessBuffer[modePacketSize] = {1, 1, 255, 255, 255, 255, 255, 255, 255, 255};

// these modes are intended to be used for the mvp, so have explicit IDs
static std::map<std::string, TestModeDataStruct> mvpModes = {
  {"warmConstBrightness", makeConstBrightnessTestStruct(ColourRatiosStruct{
    .white = {255},
    .whiteAndWarm = {0, 255},
    .RGB = {255, 192, 111}
  }, 2)},
};

static std::map<std::string, TestModeDataStruct> testOnlyModes = {
  {"purpleConstBrightness", makeConstBrightnessTestStruct(ColourRatiosStruct{
    .white = {255},
    .whiteAndWarm = {126, 255},
    .RGB = {142, 111, 255}
  })},
};

// chuck all the mvp and test modes into a single vector array
std::vector<TestModeDataStruct> getAllTestingModes(){
  std::vector<TestModeDataStruct> modeArray;
  for(const auto& pair : mvpModes){
    modeArray.push_back(pair.second);
  }
  for(const auto& pair : testOnlyModes){
    modeArray.push_back(pair.second);
  }
  return modeArray;
}

const duty_t constantBrightnessColourRatios[] = {255, 255, 255, 255, 255, 255, 255, 255};
