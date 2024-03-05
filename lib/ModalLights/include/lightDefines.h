/*
 * This file creates two complimentary square waves that
 * create an ac signal, intended to drive LEDs wired in
 * opposite directions. Changing the brightness will
 * change both LEDs equally.
 */

#ifndef __LIGHT_DEFINES_H__
#define __LIGHT_DEFINES_H__

#include <stdint.h>

#ifndef LED_LIGHTS_DUTY_BITs
  #define LED_LIGHTS_DUTY_BITs 12
#endif

#if LED_LIGHTS_DUTY_BITs <= 8
  #define LED_LIGHTS_MAX_DUTY 255
  #define duty_t uint8_t
#elif LED_LIGHTS_DUTY_BITs <= 10
  #define LED_LIGHTS_MAX_DUTY 1023
  typedef uint16_t duty_t;
#else
  #define LED_LIGHTS_MAX_DUTY 255
  typedef uint8_t duty_t;
#endif

struct LightStateStruct {
  duty_t dutyLevel = 0;   // the duty cycle of the lights. can have a value even when off
  bool state = 0;         // i.e. on or off

  /**
   * @brief returns the brightness of the lights
  */
  duty_t brightness(){
    return dutyLevel * state;
  };
};

#endif