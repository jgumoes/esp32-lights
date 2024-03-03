/*
 * This file creates two complimentary square waves that
 * create an ac signal, intended to drive LEDs wired in
 * opposite directions. Changing the brightness will
 * change both LEDs equally.
 */

#ifndef __LIGHTS_PWM_H__
#define __LIGHTS_PWM_H__

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

void setPowerLevel(float);
float getPowerLevel();
void setDutyCycle(duty_t duty_value);
duty_t getDutyCycleValue();
duty_t getDutyLevel();

void toggle_Lights_State();
void set_Lights_State(bool state);
bool get_Lights_State();
bool areLightsOn();

void setup_PWM(uint8_t pin, uint8_t freq, duty_t initialDuty);
void setup_symmetrical_PWM(uint8_t pwm0, uint8_t pwm1);

#endif