/*
 * This file creates two complimentary square waves that
 * create an ac signal, intended to drive LEDs wired in
 * opposite directions. Changing the brightness will
 * change both LEDs equally.
 */

#ifndef __LIGHTS_PWM_H__
#define __LIGHTS_PWM_H__

#include <Arduino.h>
#define MAX_DUTY 255

void setPowerLevel(float);
float getPowerLevel();
void setDutyCycle(uint duty_value);

void toggle_Lights_State();
void set_Lights_State(bool);
bool get_Lights_State();
bool isLightsOn();

void update_Lights();

void setup_PWM(uint pwm0, uint pwm1);

#endif