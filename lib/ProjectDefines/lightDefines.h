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

typedef uint8_t duty_t;   // 255 is absolutely fine. lightsHal can interpolate up if needed
const duty_t LED_LIGHTS_MAX_DUTY= ~((duty_t)0);

const uint8_t nChannels = 1;  // TODO: delete me and replace with templates

struct LightStateStruct {
  duty_t values[nChannels+1] = {};   // [0] is duty cycle, the rest is colour ratios in same order as Channels enum
  bool state = 0;         // i.e. on or off
};

#endif