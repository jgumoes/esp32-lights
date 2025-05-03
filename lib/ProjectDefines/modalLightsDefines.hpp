#ifndef __MODAL_LIGHTS_DEFINES_HPP__
#define __MODAL_LIGHTS_DEFINES_HPP__

#include <Arduino.h>

#include "lightDefines.h"

typedef uint8_t modeUUID;

struct CurrentModeStruct {
  modeUUID activeMode;
  modeUUID backgroundMode;
};

struct ModalConfigsStruct {
  duty_t minOnBrightness = 1;       // the absolute minimum brightness when state == on
  uint8_t softChangeWindow = 1;   // 1 second change for sudden brightness changes
};

enum class ModeTypes : uint8_t {
  nullMode = 0,             // this ID cancels the active mode, and gets sent to the server if an error occurs
  constantBrightness = 1,   // normal, dimmable lamp
  sunrise = 2,              // increasing brightness that reaches max at a defined time. not dimmable, toggle switches to constant brightness
  sunset = 3,               // brightness dims at a constant rate
  pulse = 4,                // flashing at a constant rate
  chirp = 5,                // flashing of increasing intensity
  changing = 6
};

typedef uint8_t channelID;

enum class Channels : channelID {
  white = 1 << 0,
  warm = 1 << 1,
  red = 1 << 2,
  green = 1 << 3,
  blue = 1 << 4,
  uv = 1 << 5
};

// size of the header, i.e. modeUUID and ModeTypes
const uint8_t modePacketHeaderSize = 2;
// size of all of the stored mode data and header
const uint8_t modePacketSize = 9 + 2*nChannels;

// the position in the mode data array should match the position in ModeDataStruct
struct ModeDataStruct {
  modeUUID ID = 0;
  ModeTypes type = ModeTypes::nullMode;
  duty_t endColourRatios[nChannels];
  duty_t startColourRatios[nChannels];

  duty_t maxBrightness = 0; // max/end/target
  duty_t minBrightness = 0; // min/start
  duty_t finalMaxBrightness = 0;  // (chirp only) final max brightness
  duty_t finalMinBrightness = 0;  // (chirp only) final min brightness

  uint8_t time[3] = {0,0,0};  // [rate/window/period][0][0]
                    // [initialPeriod][finalPeriod][chirpDuration]
                    // [timeWindow MSB][timeWindow LSB][0]
};

void static serializeModeDataStruct(ModeDataStruct dataStruct, uint8_t buffer[modePacketSize]){
  uint8_t i = 0;
  buffer[i] = dataStruct.ID;
  i++;
  buffer[i] = static_cast<uint8_t>(dataStruct.type);
  i++;
  switch(dataStruct.type){
    case ModeTypes::constantBrightness:
      for(uint8_t c = 0; c < nChannels; c++){
        buffer[i] = dataStruct.endColourRatios[c];
        i++;
      }
      buffer[i] = dataStruct.minBrightness;
      i++;
      break;

    default:
      throw("mode type doesn't exist");
      break;
  }
  for(i; i < modePacketSize; i++){
    buffer[i] = 0;
  }
}

void static deserializeModeData(duty_t dataArray[modePacketSize], ModeDataStruct *dataStruct){
  ModeTypes type = static_cast<ModeTypes>(dataArray[1]);
  switch (type)
  {
  case ModeTypes::constantBrightness:
  {
    int i = 0;
    dataStruct->ID = dataArray[i]; i++;  // i = 1
    dataStruct->type = type; i++;        // i = 2
    for(uint8_t c = 0; c < nChannels; c++){
      dataStruct->endColourRatios[c] = dataArray[i];
      dataStruct->startColourRatios[c] = 0;
      i++;
    } // i = 2 + nChannels
    dataStruct->maxBrightness = 0;
    dataStruct->minBrightness = dataArray[i];
    dataStruct->finalMaxBrightness = 0;
    dataStruct->finalMinBrightness = 0;
    uint8_t timeVals[3] = {0,0,0};
    memcpy(dataStruct->time, timeVals, 3);
    return;
  }
  default:
    {
      #ifdef native_env
        throw("mode doesn't exist (or not implemented or whatever)");
      #endif
      return;
    }
  }
}

void static fillDefaultConstantBrightnessStruct(ModeDataStruct *dataStruct, uint8_t numChannels){
  dataStruct->ID = 1;
  dataStruct->type = ModeTypes::constantBrightness;
  dataStruct->maxBrightness = 0;
  dataStruct->minBrightness = 0;
  dataStruct->finalMaxBrightness = 0;
  dataStruct->finalMinBrightness = 0;
  for(uint8_t i = 0; i < numChannels; i++){
    dataStruct->endColourRatios[i] = 255;
    dataStruct->startColourRatios[i] = 0;
    dataStruct->time[i] = 0;
  }
}

/**
 * @brief Get the number of bytes of data for a given mode, including ID and type. if no mode is given, it uses the largest mode which should also be the size of the mode buffer
 * 
 * @param type 
 * @return uint8_t 
 */
static uint8_t getModeDataSize(ModeTypes type = ModeTypes::chirp){
  switch(type){
    case ModeTypes::constantBrightness:
      return 3 + nChannels;
    case ModeTypes::sunrise:
      return 4 + 2*nChannels;
    case ModeTypes::sunset:
      return 4 + nChannels;
    case ModeTypes::pulse:
      return 5 + 2*nChannels;
    case ModeTypes::chirp:
      return 9 + 2*nChannels;
    case ModeTypes::changing:
      return 6 + 2*nChannels;
    default:
      return 0;
  }
}

enum class InteractionSources : uint8_t {
  physical = 1 << 1, // i.e. user pressed a button or turned a nob
  network = 1 << 2,  // i.e. from the app, an iot button or voice assistant
  environmental = 1 << 3 // from the TBD sensor handling  (i.e. lights turn off when noone is in the room)
};

#endif