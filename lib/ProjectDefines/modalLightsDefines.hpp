#ifndef __MODAL_LIGHTS_DEFINES_HPP__
#define __MODAL_LIGHTS_DEFINES_HPP__

#include <Arduino.h>

#include "lightDefines.h"

struct ModalConfigsStruct {
  duty_t defaultOnBrightness = 13;  // about 5%
  duty_t minOnBrightness = 1;       // the minimum brightness when state == on
  uint8_t changeoverWindow = 10;  // 10 seconds to change from one mode to the next
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

typedef uint8_t modeUUID;

/**
 * @brief Get the number of bytes of data for a given mode, including ID and type. if no mode is given, it uses the largest mode which should also be the size of the mode buffer
 * 
 * @param type 
 * @return uint8_t 
 */
static uint8_t getModeDataSize(ModeTypes type = ModeTypes::chirp){
  switch(type){
    case ModeTypes::constantBrightness:
      return 2 + nChannels;
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

// size of the header, i.e. modeUUID and ModeTypes
const uint8_t modePacketHeaderSize = 2;
// size of all of the stored mode data and header
const uint8_t modePacketSize = 9 + 2*nChannels;

enum class InteractionSources : uint8_t {
  physical = 1 << 1, // i.e. user pressed a button or turned a nob
  network = 1 << 2,  // i.e. from the app, an iot button or voice assistant
  environmental = 1 << 3 // from the TBD sensor handling  (i.e. lights turn off when noone is in the room)
};

#endif