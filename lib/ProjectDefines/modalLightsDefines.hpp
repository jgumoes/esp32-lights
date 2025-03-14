#ifndef __MODAL_LIGHTS_DEFINES_HPP__
#define __MODAL_LIGHTS_DEFINES_HPP__

#include <Arduino.h>

#include "lightDefines.h"

enum class ModeTypes {
  nullMode = 0,             // this ID cancels the active mode, and gets sent to the server if an error occurs
  constantBrightness = 1,   // normal, dimmable lamp
  sunrise = 2,              // increasing brightness that reaches max at a defined time. not dimmable, toggle switches to constant brightness
  sunset = 3,               // brightness dims at a constant rate
  pulse = 4,                // flashing at a constant rate
  chirp = 5                 // flashing of increasing intensity
};

typedef uint8_t modeUUID;
typedef uint8_t channelID;
struct ChannelDataPacket{
  channelID channel = 0;
  duty_t highBrightness = 0;  // max/target brightness
  duty_t lowBrightness = 0;   // min/toggle On brightness
  uint8_t rate = 0;           // seconds between change of 
};

struct ModeDataPacket{
  modeUUID modeID = 0;
  ModeTypes type = ModeTypes::nullMode;
  bool isActive = 0;
  ChannelDataPacket channelData[NUMBER_OF_LIGHTS_CHANNELS];
};

#endif