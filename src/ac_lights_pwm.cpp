/*
 * This file creates two complimentary square waves that
 * create an ac signal, intended to drive LEDs wired in
 * opposite directions. Changing the brightness will
 * change both LEDs equally.
 */

#include <Arduino.h>
#include "driver/mcpwm.h"
#include "defines.h"
#include "ac_lights_pwm.h"

static const char* TAG = "ac_lights_pwm";

bool lights_on = true;
float duty_cycle = 50;

/*
 * Configures and starts two opposite pwms that can
 * be used create a modulated AC wave.
 * @param duty - duty cycle between 0.00% and 100.00%
 */
void set_PWM_Duty(float duty){
  duty_cycle = duty / 2;
  update_Lights();
}

float get_PWM_Duty(){
  return duty_cycle;
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
  lights_on = state;
}

bool get_Lights_State(){
  return lights_on;
}

/*
 * Sets the physical PWM lights to lights_state.
 * Set the state first using [toggle_Lights_state()] of set_Lights_State
 */
void update_Lights(){
  if(lights_on){
    const mcpwm_config_t init_config_2 = {
      .frequency = PWM_FREQ,
      .cmpr_a = duty_cycle,
      .duty_mode = MCPWM_DUTY_MODE_0,
      .counter_mode = MCPWM_UP_COUNTER
    };
    delayMicroseconds(100); // the timers are definitely in sync, or else it it would show on the oscilloscope
    const esp_err_t init_timer_0 = mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &init_config_2);
    ESP_LOGV(TAG, "initialising mcpwm timer 0: %d", init_timer_0);

    const mcpwm_config_t init_config_1 = {
      .frequency = PWM_FREQ,
      .cmpr_a = duty_cycle,
      .duty_mode = MCPWM_DUTY_MODE_0,
      .counter_mode = MCPWM_DOWN_COUNTER
    };
    const esp_err_t init_timer_1 = mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &init_config_1);
    ESP_LOGV(TAG, "initialising mcpwm timer 1: %d", init_timer_1);
  }
  else{
    const esp_err_t stop_timer_0 = mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
    ESP_LOGV(TAG, "stopping mcpwm timer 0: %d", stop_timer_0);
    const esp_err_t stop_timer_1 = mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_1);
    ESP_LOGV(TAG, "stopping mcpwm timer 1: %d", stop_timer_1);
  }
}

void setup_PWM(){
  
  set_PWM_Duty(50);
  const mcpwm_pin_config_t pin_config = {
    .mcpwm0a_out_num = PWM_0,
    .mcpwm1a_out_num = PWM_1,
  };
  const esp_err_t set_pin = mcpwm_set_pin(MCPWM_UNIT_0, &pin_config);
  ESP_LOGV(TAG, "setting pins: %d", set_pin);

  // esp_err_t start = mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
  // Serial.print("starting: "); Serial.println(esp_err_to_name(start));
}