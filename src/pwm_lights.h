#ifndef __PWM_LIGHTS_H__
#define __PWM_LIGHTS_H__

#include<ModalLights.h>

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