#include <Arduino.h>
#include "defines.h"
#include "driver/touch_sensor.h"
#include "touch_switch.h"
#include "ac_lights_pwm.h"

static touch_pad_t _touchPin = TOUCH_PAD_NUM1;

void IRAM_ATTR touchInterrupt(){
  if(touchInterruptGetLastStatus(_touchPin)){
    Serial.println("toggling lights state");
    toggle_Lights_State();
  }
}

/*
 * Setup the interrupt for the touch switch.
 * Call after setting PWM, as the pwm adjusts the
 * clock frequency (i think)
 */
void setupTouch(touch_pad_t touchPin){
  Serial.println("setting up touchpad");
  _touchPin = touchPin;
  touch_pad_init();
  touch_pad_config(touchPin);
  
  touch_pad_set_meas_time(150000 / 5, 0xffff);  // limit how quickly the light state cycles by setting the touch measurement to the slowest possible
  touch_pad_set_thresh(touchPin, 50000);
  touchAttachInterrupt(touchPin, touchInterrupt, TOUCH_THRESHOULD);
}

/*
 * Reads the touch value and prints it to serial
 */
void print_Touch(){
  uint touch_val = touchRead(_touchPin);
  Serial.print("touch value: "); Serial.println(touch_val);
}