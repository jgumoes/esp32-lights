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
  duty_t getBrightness(){
    return dutyLevel * state;
  };

  /**
   * @brief Sets the dutyLevel and state. doesn't check limits
   * 
   * @param brightness 
   */
  void setBrightness(duty_t brightness){
    dutyLevel = brightness;
    state = brightness > 0;
  }
};

#endif