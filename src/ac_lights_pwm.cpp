/*
 * This file creates two complimentary square waves that
 * create an ac signal, intended to drive LEDs wired in
 * opposite directions. Changing the brightness will
 * change both LEDs equally.
 */

#include <Arduino.h>
#include "driver/ledc.h"
#include "defines.h"
#include "ac_lights_pwm.h"

#define SPEED_MODE LEDC_LOW_SPEED_MODE


bool lights_on = true;
float powerLevel = 100;


/*
 * sets the pwm duty cycle by integer value 
 */
void setDutyCycle(uint duty_value){
  // ledc_set_duty_and_update(SPEED_MODE, LEDC_CHANNEL_0, powerLevel, 0);
  ledc_set_duty(SPEED_MODE, LEDC_CHANNEL_0, duty_value);
  ledc_set_duty(SPEED_MODE, LEDC_CHANNEL_1, MAX_DUTY - duty_value + 1);
  ledc_update_duty(SPEED_MODE, LEDC_CHANNEL_0);
  ledc_update_duty(SPEED_MODE, LEDC_CHANNEL_1);
}

/*
 * Configures and starts two opposite pwms that can
 * be used create a modulated AC wave.
 * @param duty - duty cycle between 0.00% and 100.00%
 */
void setPowerLevel(float power){
  powerLevel = power;
  uint dutyCycle = MAX_DUTY * powerLevel/200;
  setDutyCycle(dutyCycle);
}

float getPowerLevel(){
  return powerLevel;
}

/*
 * Toggles the lights state.
 * i.e. if the lights are on, this sets them to off
 */
void toggle_Lights_State(){
  lights_on = !lights_on;
}

/*
 * Sets the lights state.
 */
void set_Lights_State(bool state){
  if(state == lights_on){
    return;
  }
  lights_on = state;
  if(lights_on){
    setPowerLevel(powerLevel);
  }
  else{
    setDutyCycle(0);
  }
}

bool get_Lights_State(){
  return lights_on;
}


void setup_PWM(uint pwm0, uint pwm1){

  ledc_timer_config_t timer_config_0 = {
    .speed_mode = SPEED_MODE,
    .duty_resolution = LEDC_TIMER_8_BIT,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = 1000,
  };
  esp_err_t timer_err = ledc_timer_config(&timer_config_0);
  Serial.print("timer config: "); Serial.println(esp_err_to_name(timer_err));

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
  esp_err_t channel_0_err= ledc_channel_config(&channel_config_0);
  Serial.print("channel 0 config: "); Serial.println(esp_err_to_name(channel_0_err));

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
  esp_err_t channel_1_err= ledc_channel_config(&channel_config_1);
  Serial.print("channel 1 config: "); Serial.println(esp_err_to_name(channel_1_err));
  
}