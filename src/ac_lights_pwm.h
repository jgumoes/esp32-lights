/*
 * This file creates two complimentary square waves that
 * create an ac signal, intended to drive LEDs wired in
 * opposite directions. Changing the brightness will
 * change both LEDs equally.
 */

#ifndef __LIGHTS_PWM_H__
#define __LIGHTS_PWM_H__

#include <Arduino.h>
#include "driver/mcpwm.h"

void set_PWM_Duty(float);
float get_PWM_Duty();

void toggle_Lights_State();
void set_Lights_State(bool);
bool get_Lights_State();

void update_Lights();

void setup_PWM(uint pwm0, uint pwm1);

#endif