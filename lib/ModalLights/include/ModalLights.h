#ifndef _MODAL_LIGHTS_H_
#define _MODAL_LIGHTS_H_

#include "lights_pwm.h" // TODO: the setters should not be globally available
#include "modes.h"

/**
 * @brief update the state of the lights.
 * should be called in the main loop
 * 
 * @param currentTimestamp in microseconds
 */
void updateLights(uint64_t currentTimestamp){
   setDutyCycle(findBrightness(currentTimestamp));
}
#endif