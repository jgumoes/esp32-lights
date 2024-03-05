/*
 * This file creates two complimentary square waves that
 * create an ac signal, intended to drive LEDs wired in
 * opposite directions. Changing the brightness will
 * change both LEDs equally.
 */

// #include <Arduino.h>
#include <stdint.h>
#ifndef native_env
  #include "driver/ledc.h"
#else
  #include "ledcMock.h"
#endif
#include "defines.h"
#include "pwm_lights.h"
#define SPEED_MODE LEDC_LOW_SPEED_MODE


volatile bool lights_on = true;
#ifdef LIGHTS_PWM_SYMMETRICAL_OUTPUTS
  volatile uint8_t dutyCycle = AC_MAX_DUTY / 2;
  #define LIGHTS_PWM_PERCENTAGE_DIVIDER 200
#else
  volatile duty_t dutyCycle = LED_LIGHTS_MAX_DUTY;
  #define LIGHTS_PWM_PERCENTAGE_DIVIDER 100
#endif


/*
 * write the duty cycles, without changing dutyCycle
 */
void _writeDutyCycle(duty_t duty_value){
  // ledc_set_duty_and_update(SPEED_MODE, LEDC_CHANNEL_0, powerLevel, 0);
  ledc_set_duty(SPEED_MODE, LEDC_CHANNEL_0, duty_value);
  ledc_update_duty(SPEED_MODE, LEDC_CHANNEL_0);
#ifdef LIGHTS_PWM_SYMMETRICAL_OUTPUTS
  ledc_set_duty(AC_SPEED_MODE, LEDC_CHANNEL_1, AC_MAX_DUTY - duty_value + 1);
  ledc_update_duty(AC_SPEED_MODE, LEDC_CHANNEL_1);
#endif
}

/*
 * sets the pwm duty cycle by integer value between 0 and LED_LIGHTS_MAX_DUTY.
 */
void setDutyCycle(duty_t dutyValue){
  if(dutyValue == 0){
    lights_on = false;
  }
  else{
    lights_on = true;
  }
  dutyCycle = dutyValue;
  _writeDutyCycle(dutyCycle);
}

/**
 * @brief Get the raw Duty Cycle value i.e. returns dutyCycle when state is 'off'
 * 
 * @return duty cycle
 */
duty_t getDutyCycleValue(){
  return dutyCycle;
}

/**
 * @brief Get the current operating duty cycle i.e. 0 when state is 'off'
 * 
 * @return duty_t 
 */
duty_t getDutyLevel(){
  return lights_on * dutyCycle;
}

/**
 * @brief and starts two opposite pwms that can
 * be used create a modulated AC wave.
 * 
 * @param duty - duty cycle between 0.00% and 100.00%
 */
void setPowerLevel(float powerLevel){
  setDutyCycle((LED_LIGHTS_MAX_DUTY * powerLevel)/LIGHTS_PWM_PERCENTAGE_DIVIDER);
}

/**
 * @brief Returns true if lights are on and duty cycle is over 0.
 */
bool areLightsOn(){
  return lights_on  && (dutyCycle > 0);
}

float getPowerLevel(){
  float powerLevel = ((dutyCycle * LIGHTS_PWM_PERCENTAGE_DIVIDER) / LED_LIGHTS_MAX_DUTY) ;
  return powerLevel;
}

/*
 * Toggles the lights state.
 * i.e. if the lights are on, this sets them to off
 */
void toggle_Lights_State(){
  set_Lights_State(!lights_on);
}

/*
 * Sets the lights state.
 */
void set_Lights_State(bool state){
  if(state == lights_on){
    return;
  }
  // Serial.print("setting lights to: "); Serial.println(state);
  lights_on = state;
  _writeDutyCycle(lights_on * dutyCycle);
}

bool get_Lights_State(){
  return lights_on;
}

/**
 * @brief Setup the single-channel PWM.
 * 
 * @param pin output pin
 * @param freg frequency in hertz
 */
void setup_PWM(uint8_t pin, uint8_t freq){
  setup_PWM(pin, freq, 0);
}

/**
 * @brief Setup the single-channel PWM.
 * 
 * @param pin output pin
 * @param freg frequency in hertz
 * @param initialDuty the duty to turn on with from 0 to LED_LIGHTS_MAX_DUTY
 */
void setup_PWM(uint8_t pin, uint8_t freq, duty_t initialDuty){
  if(initialDuty == 0){
    lights_on = false;
  }

  ledc_timer_config_t timer_config_0 = {
    .speed_mode = SPEED_MODE,
    .duty_resolution = LEDC_TIMER_8_BIT,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = freq,
  };
  esp_err_t timer_err = ledc_timer_config(&timer_config_0);
  // Serial.print("timer config: "); Serial.println(esp_err_to_name(timer_err));

  ledc_channel_config_t channel_config_0 = {
    .gpio_num = pin,
    .speed_mode = SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = initialDuty,
    .hpoint = 0,
    .flags{
      .output_invert = 0
    }
  };
  esp_err_t channel_0_err= ledc_channel_config(&channel_config_0);
  // Serial.print("channel 0 config: "); Serial.println(esp_err_to_name(channel_0_err));
}

/**
 * @brief Setup the symmetrical PWM. LIGHTS_PWM_SYMMETRICAL_OUTPUTS must be defined
 * 
 * @param pwm0 output pin 0
 * @param pwm1 output pin 1
 */
void setup_symmetrical_PWM(uint8_t pwm0, uint8_t pwm1){
  ledc_timer_config_t timer_config_0 = {
    .speed_mode = SPEED_MODE,
    .duty_resolution = LEDC_TIMER_8_BIT,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = 1000,
  };
  esp_err_t timer_err = ledc_timer_config(&timer_config_0);
  // Serial.print("timer config: "); Serial.println(esp_err_to_name(timer_err));

  ledc_channel_config_t channel_config_0 = {
    .gpio_num = pwm0,
    .speed_mode = SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = 127,
    .hpoint = 0,
    .flags{
      .output_invert = 0
    }
  };
  esp_err_t channel_0_err = ledc_channel_config(&channel_config_0);
  // Serial.print("channel 0 config: "); Serial.println(esp_err_to_name(channel_0_err));

  ledc_channel_config_t channel_config_1 = {
    .gpio_num = pwm1,
    .speed_mode = SPEED_MODE,
    .channel = LEDC_CHANNEL_1,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = 127,
    .hpoint = 0,
    .flags{
      .output_invert = 1
    }
  };
  esp_err_t channel_1_err = ledc_channel_config(&channel_config_1);
  // Serial.print("channel 1 config: "); Serial.println(esp_err_to_name(channel_1_err));
}