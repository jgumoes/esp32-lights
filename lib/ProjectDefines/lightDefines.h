/*
 * This file contains defines necessary for the Modal controller
 *
 * It includes defines relating to the hardware PWM
 * (i.e. maximum duty cycle). ModalLights should retain ownership
 * of these defines, because they relate directly to the library
 * and the code that interfaces with the library, but also
 * because this is the only piece of code that should interact
 * with the hardware abstraction layer.
 */

#ifndef __LIGHT_DEFINES_H__
#define __LIGHT_DEFINES_H__

#include <stdint.h>
// #include "ProjectDefines.h"
// #include "PrintDebug.h"

#ifndef interpDivide
  // macro for the dividing portion of interpolation. rounds to nearest int if top is positive, doesn't round if top is negative. bottom is assumed always positive
  // TODO: when negative, 0.5 needs to round down
  // #define interpDivide(top, bottom) (top >= 0 ? (top + (bottom>>1))/bottom : (top)/bottom)
  #define interpDivide(top, bottom) (top >= 0 ? (top + (bottom>>1))/bottom : (top - (bottom>>1)+500000)/bottom)

#endif

#ifndef roundingDivide
  // divides the top by the bottom, rounded to the nearest integer. assumes bottom is always positive
  #define roundingDivide(top, bottom) (top >= 0 ? (top + (bottom>>1))/bottom : (top - (bottom>>1))/bottom)
#endif

typedef uint8_t duty_t;   // 255 is absolutely fine. lightsHal can interpolate up if needed
const duty_t LED_LIGHTS_MAX_DUTY= ~((duty_t)0);

const uint8_t nChannels = 3;  // TODO: delete me and replace with templates

struct LightStateStruct {
private:
  duty_t channelValues[nChannels];

public:
  duty_t values[nChannels+1] = {};   // [0] is duty cycle, the rest is colour ratios in same order as Channels enum
  bool state = 0;         // i.e. on or off
  
  /**
   * @brief adjusts the colour ratios according to brightness, and returns a pointer to the channelValues array
   * 
   * @return uint8_t* start of duty_t[nChannels] array
   */
  uint8_t* getLightValues(){
    // TODO: normalise colour ratios?
    duty_t brightness = this->state * this->values[0];
    for(uint8_t i = 0; i < nChannels; i++){
      channelValues[i] = roundingDivide(this->values[i+1] * brightness, LED_LIGHTS_MAX_DUTY);
    };
    return channelValues;
  }
};

#endif